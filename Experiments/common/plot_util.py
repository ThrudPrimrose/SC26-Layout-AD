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
