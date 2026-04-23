"""
plot_util.py -- canonical plotting helpers shared across every experiment.

Policy decisions (applied globally):

  1. **No outlier removal.** Every sample drawn from a timed benchmark
     is preserved exactly as collected. `remove_outliers()` is kept as
     a pass-through function only so legacy call sites do not need to
     be deleted; it returns its input unchanged. Outliers in HPC
     measurements usually carry information (OS jitter, cache-miss
     regimes, GPU clock transitions) and muting them biases the
     distribution shape.

  2. **Identical violin-plot sampling across all experiments.** The
     violin KDE uses the same bandwidth estimator and the same number
     of density points in every figure. The kernel density estimate is
     built from every collected sample (no subsampling).

  3. **No whiskers / no extrema markers.** Violin plots are drawn with
     `showextrema=False` so min/max are not highlighted; each figure
     may overlay its own per-group median / 95% CI on top.

Import from any per-experiment plot_paper.py:

    import sys, os
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "common"))
    from plot_util import remove_outliers, canonical_violin, \
                          VIOLIN_BW_METHOD, VIOLIN_POINTS
"""

# Canonical violin-plot parameters. DO NOT change per-experiment; consistency
# of the density estimate across figures is the whole point.
#
# VIOLIN_BW_METHOD -- passed to matplotlib's violinplot(bw_method=...).
#     'scott' is the default in numpy's gaussian_kde and the most common
#     rule-of-thumb bandwidth. Stating it explicitly prevents silent
#     changes across matplotlib releases.
#
# VIOLIN_POINTS -- number of points at which the density is evaluated.
#     200 gives a visibly smooth shape without overfitting to n=100
#     samples.
VIOLIN_BW_METHOD = "scott"
VIOLIN_POINTS = 200


def remove_outliers(vals, *args, **kwargs):
    """Pass-through. Previously did IQR-based trimming; now preserves
    every sample so the violin plot shows the true distribution.

    Kept as a function (not just removed) so legacy plot scripts that
    wrap every call through this symbol keep compiling without edits.
    """
    return vals


def canonical_violin(ax, datasets, positions, widths=0.9):
    """Draw a violin plot with the repo-wide canonical settings.

    Parameters
    ----------
    ax : matplotlib Axes
    datasets : sequence of 1-D numpy arrays (one violin each)
    positions : sequence of x positions, same length as datasets
    widths : violin width (per figure; does not affect the density)

    Returns the matplotlib PolyCollection dict from violinplot() so
    the caller can still restyle face/edge colors per figure.
    """
    return ax.violinplot(
        datasets,
        positions=positions,
        showmedians=False,
        showextrema=False,
        widths=widths,
        bw_method=VIOLIN_BW_METHOD,
        points=VIOLIN_POINTS,
    )


# ──────────────────────────────────────────────────────────────────────────
#  Data-discovery helpers
#
#  Every plot_paper.py across the repo should load its inputs through these
#  helpers instead of hard-coding `results/daint/...` / `results/beverin/...`
#  path strings. The convention is:
#
#    <experiment_dir>/results/<platform>/<csv_name>
#
#  where <platform> is one of `PLATFORMS` below. Each helper returns an
#  empty / missing marker rather than raising when a CSV is absent, so a
#  single-platform reproduction run still plots cleanly.
# ──────────────────────────────────────────────────────────────────────────

PLATFORMS = ("daint", "beverin")


def experiment_dir(caller_file):
    """Return the directory containing the calling plot_paper.py.

    Usage from a plot script:
        EXP_DIR = experiment_dir(__file__)
    """
    import os
    return os.path.dirname(os.path.abspath(caller_file))


def results_root(experiment_dir_path):
    """`<experiment>/results` -- where every bench writes its CSVs."""
    import os
    return os.path.join(experiment_dir_path, "results")


def results_path(experiment_dir_path, platform, csv_name):
    """Compose the full CSV path for one (platform, csv_name) pair.

    Example: results_path(EXP_DIR, "daint", "numa_cpu.csv")
             -> "<EXP_DIR>/results/daint/numa_cpu.csv"
    """
    import os
    return os.path.join(results_root(experiment_dir_path), platform, csv_name)


def discover_csvs(experiment_dir_path, pattern="*.csv"):
    """List every CSV under <experiment>/results/<platform>/, grouped by
    platform. Missing platforms are silently skipped.

    Returns a dict: { platform: [csv_path, ...] }.
    """
    import glob, os
    found = {}
    for plat in PLATFORMS:
        plat_dir = os.path.join(results_root(experiment_dir_path), plat)
        if not os.path.isdir(plat_dir):
            continue
        found[plat] = sorted(glob.glob(os.path.join(plat_dir, pattern)))
    return found


def load_csv(experiment_dir_path, csv_name, platform=None):
    """Load one CSV as a pandas DataFrame. Adds a `platform` column so
    that per-platform DataFrames can be concatenated downstream.

    If `platform` is None, concatenates across every present platform.
    Returns an empty DataFrame if nothing is found (never raises).
    """
    import os
    import pandas as pd

    if platform is not None:
        path = results_path(experiment_dir_path, platform, csv_name)
        if not os.path.isfile(path):
            return pd.DataFrame()
        df = pd.read_csv(path)
        df["platform"] = platform
        return df

    frames = []
    for plat in PLATFORMS:
        path = results_path(experiment_dir_path, plat, csv_name)
        if not os.path.isfile(path):
            continue
        df = pd.read_csv(path)
        df["platform"] = plat
        frames.append(df)
    if not frames:
        return pd.DataFrame()
    return pd.concat(frames, ignore_index=True)


def fig_output_path(experiment_dir_path, stem, ext="pdf"):
    """Canonical figure output location: `<experiment_dir>/<stem>.<ext>`.
    Keeps figures next to their source plot_paper.py. Always returns an
    absolute path.
    """
    import os
    return os.path.join(experiment_dir_path, f"{stem}.{ext}")


# ── Canonical STREAM peak table ──────────────────────────────────────────────
# Every per-experiment plot that normalizes bandwidth ("% of STREAM Triad
# peak") reads these numbers. The backing file is Experiments/common/
# stream_peak.json, which E0_NUMA/plot_paper.py overwrites with measured
# peaks when the bench has been run. The JSON schema is:
#
#   { "peaks_tbs":       {<label>: <TB/s>, ...},
#     "platform_labels": {<platform>: {"cpu": <label>, "gpu": <label>}} }
#
# Labels match the strings the existing per-experiment plot scripts already
# use (e.g. "MI300A Zen CPU", "GH200 Grace CPU"), so swapping in
# `STREAM_PEAK = load_stream_peaks()` is a drop-in replacement for the
# hardcoded dicts in E2/E4/loopnest_1/etc.

def _stream_peak_json_path():
    """Locate stream_peak.json with cwd-awareness.

    Walks up from the current working directory looking for a sibling
    `common/stream_peak.json`. This lets scripts that `cd` into a
    PaperSnapshot/<exp>/ directory pick up PaperSnapshot/common/
    stream_peak.json instead of Experiments/common/stream_peak.json.

    Falls back to the JSON next to this module (Experiments/common/) so
    nothing breaks when there is no cwd-walkable override.
    """
    import os
    d = os.path.abspath(os.getcwd())
    for _ in range(8):
        cand = os.path.join(d, "common", "stream_peak.json")
        if os.path.isfile(cand):
            return cand
        parent = os.path.dirname(d)
        if parent == d:
            break
        d = parent
    return os.path.join(os.path.dirname(__file__), "stream_peak.json")


def load_stream_peaks():
    """Return {<label>: <peak_TB/s>} from common/stream_peak.json.

    This is the flat dict that every per-experiment plot_paper.py can use as
    a drop-in for its local `STREAM_PEAK = {...}` hardcode. Raises if the
    file is missing or malformed -- callers should treat this as an
    invariant of the repo.
    """
    import json
    with open(_stream_peak_json_path()) as f:
        data = json.load(f)
    return dict(data["peaks_tbs"])


def stream_peak_platform_labels():
    """Return {<platform>: {"cpu": <label>, "gpu": <label>}} -- the mapping
    from results-directory name (beverin / daint) to the per-device label
    used as a key in load_stream_peaks()."""
    import json
    with open(_stream_peak_json_path()) as f:
        data = json.load(f)
    return dict(data["platform_labels"])
