#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

N      = 256
B      = 8
SIGMA  = np.sqrt(2.0)
N_RUNS = 100

# Pinned to the SC26 global RNG seed (mirrors Experiments/common/prng.h's
# SC26_SEED=42). Override via SC26_SEED env var for ablations; keeps every
# illustrative figure deterministic across reruns.
SC26_SEED = int(__import__("os").environ.get("SC26_SEED", "42"))
np.random.seed(SC26_SEED)

idx_exact = np.array([(i + 2) % N for i in range(N)])

def gen_normal():
    raw = np.round((np.arange(N) + 2) + np.random.normal(0, SIGMA, N)).astype(int)
    return raw % N

def replay(idx):
    blocks = idx // B
    mu = len(np.unique(blocks))
    diffs = np.abs(np.diff(blocks.astype(np.int64)))
    return mu, diffs.mean() if len(diffs) else 0.0

mu_exact,  delta_exact  = replay(idx_exact)
mu_s, delta_s = zip(*[replay(gen_normal()) for _ in range(N_RUNS)])
mu_s, delta_s = np.array(mu_s), np.array(delta_s)
idx_normal_one = gen_normal()

ORANGE_DARK  = '#2F72B8'   # exact values — blue
ORANGE_LIGHT = '#C04A10'   # approximation / violin fill — red
ZOOM_LO, ZOOM_HI = 10, 30

# Each panel is square: panel_size x panel_size inches
panel_size = 2.8
fig = plt.figure(figsize=((panel_size * 3 + 1.2)*0.5, (panel_size + 1.4)*0.5))
gs  = fig.add_gridspec(1, 3, width_ratios=[1, 1, 1],
                        left=0.07, right=0.97,
                        top=0.82, bottom=0.26,
                        wspace=0.38)
ax_pat   = fig.add_subplot(gs[0])
ax_mu    = fig.add_subplot(gs[1])
ax_delta = fig.add_subplot(gs[2])

fig.suptitle(
    r'Indirect access $A[\,\mathrm{idx}[i]\,]$ with $\mathrm{idx}[i]=(i{+}2)\;\%\;N$',
    fontsize=9.5)

# ── Left: zoomed access sequence ──────────────────────────────────────
zoom_mask = (np.arange(N) >= ZOOM_LO) & (np.arange(N) <= ZOOM_HI)
iters = np.arange(N)

y_lo = (ZOOM_LO + 2) - 4
y_hi = (ZOOM_HI + 2) + 4

# Force square by matching x and y range manually
x_range = ZOOM_HI - ZOOM_LO
y_range = y_hi - y_lo
mid_y   = (y_lo + y_hi) / 2
if y_range < x_range:
    y_lo = mid_y - x_range / 2
    y_hi = mid_y + x_range / 2
elif x_range < y_range:
    mid_x = (ZOOM_LO + ZOOM_HI) / 2
    ZOOM_LO_p = mid_x - y_range / 2
    ZOOM_HI_p = mid_x + y_range / 2
else:
    ZOOM_LO_p, ZOOM_HI_p = ZOOM_LO, ZOOM_HI

for b in range(N // B):
    yb = b * B
    if y_lo <= yb <= y_hi:
        ax_pat.axhline(yb, color='#d8d8d8', lw=0.5, zorder=1)

sc_n = ax_pat.scatter(iters[zoom_mask], idx_normal_one[zoom_mask],
                      s=18, alpha=0.6, color=ORANGE_LIGHT, edgecolors='none', zorder=2)
sc_e = ax_pat.scatter(iters[zoom_mask], idx_exact[zoom_mask],
                      s=14, alpha=0.95, color=ORANGE_DARK, edgecolors='none', zorder=3)

ax_pat.set_xlim(ZOOM_LO - 0.5, ZOOM_HI + 0.5)
ax_pat.set_ylim(y_lo, y_hi)
ax_pat.set_box_aspect(1.1)
ax_pat.set_xlabel('Iteration $i$', fontsize=9)
ax_pat.set_ylabel('Value of idx[$i$]', fontsize=9)
ax_pat.set_title(f'Access Sequence', fontsize=9)
ax_pat.tick_params(labelsize=8)

# ── Shared legend — centred just below all panels ─────────────────────
leg_labels = [
    r'Approx. value of idx[$i$] $\sim\mathcal{N}(i{+}2,\;\sigma^2{=}2)$',
    r'Replay value of idx[$i$] $= (i{+}2)\;\%\;N$',
]
fig.legend(handles=[sc_n, sc_e], labels=leg_labels,
           loc='lower center', bbox_to_anchor=(0.52, -0.13),
           fontsize=8.2, framealpha=0.9, markerscale=1.2,
           handletextpad=0.2, borderpad=0.1, ncol=1,
           columnspacing=1, labelspacing=0.2)

# ── Helper: violin styling ─────────────────────────────────────────────
def style_violin(vp):
    for body in vp['bodies']:
        body.set_facecolor(ORANGE_LIGHT)
        body.set_alpha(0.45)
        body.set_edgecolor(ORANGE_LIGHT)
    vp['cmedians'].set_color(ORANGE_LIGHT)
    vp['cmedians'].set_linewidth(1.8)

# ── Middle: μ ─────────────────────────────────────────────────────────
vp = ax_mu.violinplot(mu_s, positions=[1], showmedians=True,
                      showextrema=False, widths=0.5)
style_violin(vp)

ax_mu.plot(0.35, mu_exact, 'o', color=ORANGE_DARK, ms=9, zorder=5,
           markeredgecolor='white', markeredgewidth=1.2)
ax_mu.annotate(f'{mu_exact}', (0.35, mu_exact),
               textcoords='offset points', xytext=(-18, 5),
               fontsize=9, color=ORANGE_DARK)
med = np.median(mu_s)
ax_mu.annotate(f'{med:.0f}', (1, med),
               textcoords='offset points', xytext=(5, -12),
               fontsize=9, color=ORANGE_LIGHT)

ax_mu.set_xlim(-0.1, 1.6)
ax_mu.set_xticks([0.35, 1.0])
ax_mu.set_xticklabels(['Exact', '  Approx.'], fontsize=8)
ax_mu.set_ylabel(r'$\mu$', fontsize=9)
ax_mu.set_title(r'$\mu$ (Blocks Touched)', fontsize=9)
ax_mu.set_box_aspect(1.1)
ax_mu.grid(axis='y', alpha=0.2)
ax_mu.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
ax_mu.tick_params(labelsize=8)

# ── Right: Δ ──────────────────────────────────────────────────────────
vp2 = ax_delta.violinplot(delta_s, positions=[1], showmedians=True,
                          showextrema=False, widths=0.5)
style_violin(vp2)

ax_delta.plot(0.35, delta_exact, 'o', color=ORANGE_DARK, ms=9, zorder=5,
              markeredgecolor='white', markeredgewidth=1.2)
ax_delta.annotate(f'{delta_exact:.2f}', (0.55, delta_exact),
                  textcoords='offset points', xytext=(-27, 5),
                  fontsize=9, color=ORANGE_DARK)
med_d = np.median(delta_s)
ax_delta.annotate(f'{med_d:.2f}', (1, med_d),
                  textcoords='offset points', xytext=(7, -15),
                  fontsize=9, color=ORANGE_LIGHT)

ax_delta.set_xlim(-0.1, 1.6)
ax_delta.set_ylim(bottom=0.0)
ax_delta.set_xticks([0.35, 1.0])
ax_delta.set_xticklabels(['Exact', '  Approx.'], fontsize=8)
ax_delta.set_ylabel(r'$\Delta$', fontsize=9)
ax_delta.set_title(r'$\Delta$ (Block Distance)', fontsize=9)
ax_delta.set_box_aspect(1.1)
ax_delta.grid(axis='y', alpha=0.2)
ax_delta.tick_params(labelsize=8)

plt.savefig('stride2.png', dpi=150, bbox_inches='tight')
plt.savefig('stride2.pdf', bbox_inches='tight')
print('Done.')