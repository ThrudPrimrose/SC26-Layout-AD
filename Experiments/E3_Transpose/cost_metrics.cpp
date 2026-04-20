/*  transpose_metrics.cpp — Compute μ (avg new-block count) and Δ (avg block distance)
 *  for the matrix transpose kernel: out[c*N+r] = in[r*N+c]
 *  under row-major and blocked(SB) layouts, with various schedules.
 *
 *  Compile: g++ -O3 -fopenmp -o transpose_metrics transpose_metrics.cpp
 *  Usage:   ./transpose_metrics <N> <B> [SB_list] [csv_out]
 *           N  = matrix dimension
 *           B  = cache-line size in floats (e.g. 16 for 64-byte lines of fp32)
 *           SB_list = comma-separated block sizes (default: 8,16,32,64,128)
 *           csv_out = output CSV file (default: transpose_metrics.csv)
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <omp.h>

/* ── Layout address functions ─────────────────────────────────────── */
static inline long rm_addr(int r, int c, int N) {
    return (long)r * N + c;
}
static inline long blk_addr(int r, int c, int N, int SB) {
    int NB = N / SB;
    return (long)(r / SB * NB + c / SB) * SB * SB + (r % SB) * SB + c % SB;
}

/* ── Metric accumulator (processes one (r,c) at a time) ───────────── */
struct MetricState {
    long T = 0;
    double sum_N = 0;
    double sum_rho_bar = 0;
    long prev_in = -1, prev_out = -1;
    bool first = true;
};

static inline void metric_step(MetricState& s, long bi, long bo) {
    s.T++;
    if (s.first) {
        s.sum_N += (bi == bo) ? 1 : 2;
        s.sum_rho_bar += 1.0;
        s.prev_in = bi; s.prev_out = bo;
        s.first = false;
        return;
    }
    int n_new = 0;
    double rho_sum = 0.0;
    auto check = [&](long b) {
        if (b != s.prev_in && b != s.prev_out) {
            long d = std::min(std::abs(b - s.prev_in), std::abs(b - s.prev_out));
            rho_sum += (double)d;
            n_new++;
        }
    };
    check(bi);
    if (bo != bi) check(bo);
    s.sum_N += n_new;
    if (n_new > 0) s.sum_rho_bar += rho_sum / n_new;
    s.prev_in = bi; s.prev_out = bo;
}

struct Metrics { double mu, delta; };
static Metrics finish(const MetricState& s) {
    if (s.T == 0) return {0,0};
    return { s.sum_N / s.T, s.sum_rho_bar / s.T };
}

/* ── Config ───────────────────────────────────────────────────────── */
struct Config {
    std::string name, layout, schedule;
    int sb, sched_type, sched_param;
    bool blk_in, blk_out;
};

static Metrics run_config(const Config& cfg, int N, int B) {
    int SB = cfg.sb > 0 ? cfg.sb : 1;
    auto iaddr = [&](int r, int c) -> long {
        return (cfg.blk_in ? blk_addr(r, c, N, SB) : rm_addr(r, c, N)) / B;
    };
    auto oaddr = [&](int r, int c) -> long {
        return (cfg.blk_out ? blk_addr(c, r, N, SB) : rm_addr(c, r, N)) / B;
    };

    MetricState s;
    switch (cfg.sched_type) {
    case 0: // naive
        for (int r = 0; r < N; r++)
            for (int c = 0; c < N; c++)
                metric_step(s, iaddr(r,c), oaddr(r,c));
        break;
    case 1: { // tiled
        int TB = cfg.sched_param, NT = (N+TB-1)/TB;
        for (int tr = 0; tr < NT; tr++)
            for (int tc = 0; tc < NT; tc++)
                for (int lr = 0; lr < TB && tr*TB+lr < N; lr++)
                    for (int lc = 0; lc < TB && tc*TB+lc < N; lc++)
                        metric_step(s, iaddr(tr*TB+lr, tc*TB+lc), oaddr(tr*TB+lr, tc*TB+lc));
        break;
    }
    case 2: { // blk_aligned
        int NB = N/SB;
        for (int br = 0; br < NB; br++)
            for (int bc = 0; bc < NB; bc++)
                for (int lr = 0; lr < SB; lr++)
                    for (int lc = 0; lc < SB; lc++)
                        metric_step(s, iaddr(br*SB+lr, bc*SB+lc), oaddr(br*SB+lr, bc*SB+lc));
        break;
    }
    }
    return finish(s);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <N> <B_floats> [SB_list] [csv_out]\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]), B = atoi(argv[2]);
    std::vector<int> SBs;
    if (argc > 3) {
        std::istringstream ss(argv[3]); std::string tok;
        while (std::getline(ss, tok, ',')) SBs.push_back(atoi(tok.c_str()));
    } else SBs = {8, 16, 32, 64, 128};
    const char* csv_path = (argc > 4) ? argv[4] : "transpose_metrics.csv";

    std::vector<int> valid;
    for (int sb : SBs) if (N % sb == 0) valid.push_back(sb);

    printf("N=%d  B=%d elements (%d bytes fp32)\n\n", N, B, B*4);

    std::vector<Config> configs;
    configs.push_back({"rm / naive", "row_major", "naive", 0, 0, 0, false, false});
    for (int sb : valid) {
        char nm[64];
        snprintf(nm,64,"rm / tiled(%d)",sb);
        configs.push_back({nm,"row_major","tiled",sb,1,sb,false,false});
    }
    for (int sb : valid) {
        char nm[64];
        snprintf(nm,64,"rm / blk_sched(%d)",sb);
        configs.push_back({nm,"row_major","blk_aligned",sb,2,sb,false,false});
    }
    for (int sb : valid) {
        char nm[64];
        snprintf(nm,64,"blk(%d) / blk_sched(%d)",sb,sb);
        configs.push_back({nm,"blocked","blk_aligned",sb,2,sb,true,true});
    }
    for (int sb : valid) {
        char nm[64];
        snprintf(nm,64,"blk(%d) / naive",sb);
        configs.push_back({nm,"blocked","naive",sb,0,0,true,true});
    }

    int nc = (int)configs.size();
    std::vector<Metrics> results(nc);

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < nc; i++)
        results[i] = run_config(configs[i], N, B);

    printf("%-30s  %10s  %10s  %12s\n", "Configuration", "mu", "delta", "mu*delta");
    printf("%-30s  %10s  %10s  %12s\n",
           "------------------------------","----------","----------","------------");
    for (int i = 0; i < nc; i++)
        printf("%-30s  %10.4f  %10.4f  %12.4f\n",
               configs[i].name.c_str(), results[i].mu, results[i].delta,
               results[i].mu * results[i].delta);

    FILE* f = fopen(csv_path, "w");
    if (f) {
        fprintf(f, "layout,schedule,SB,N,B,mu,delta,mu_delta\n");
        for (int i = 0; i < nc; i++)
            fprintf(f, "%s,%s,%d,%d,%d,%.6f,%.6f,%.6f\n",
                    configs[i].layout.c_str(), configs[i].schedule.c_str(),
                    configs[i].sb, N, B,
                    results[i].mu, results[i].delta, results[i].mu * results[i].delta);
        fclose(f);
        printf("\nCSV written to %s\n", csv_path);
    }
    return 0;
}