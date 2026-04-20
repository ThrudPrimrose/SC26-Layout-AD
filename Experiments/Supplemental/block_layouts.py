import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

nrows, ncols = 4, 4
B = 2  # elements per cache block
n_elems = nrows * ncols
n_blocks = n_elems // B  # 8

# 4 repeating gray shades
gray4 = ['#777777', '#999999', '#bbbbbb', '#dddddd']

def block_shade(block_id):
    return gray4[block_id % 4]

# ── Build block-ID grids ───────────────────────────────────────────────
block_grid_rowmajor = np.zeros((nrows, ncols), dtype=int)
for r in range(nrows):
    for c in range(ncols):
        addr = r * ncols + c
        block_grid_rowmajor[r, c] = addr // B

block_grid_tiled = np.zeros((nrows, ncols), dtype=int)
for r in range(nrows):
    for c in range(ncols):
        tr, tc = r // 2, c // 2
        lr, lc = r % 2, c % 2
        tile_base = (tr * (ncols // 2) + tc) * 4
        local_addr = lr * 2 + lc
        block_grid_tiled[r, c] = (tile_base + local_addr) // B


def draw_layout(block_grid, ribbon_label, outname):
    fig, ax = plt.subplots(1, 1, figsize=(2.5, 2.5))

    # Ribbon defines the reference width
    ribbon_w = 4.0
    ribbon_x0 = 0.0

    # Grid is 0.85x the ribbon width, centered above it
    grid_w = 0.85 * ribbon_w
    cell = grid_w / ncols
    inner = 0.82 * cell
    offset = (cell - inner) / 2
    grid_x0 = (ribbon_w - grid_w) / 2

    # --- Gray background ---
    for r in range(nrows):
        for c in range(ncols):
            y = (nrows - 1 - r) * cell
            rect = mpatches.FancyBboxPatch(
                (grid_x0 + c * cell, y), cell, cell,
                boxstyle="square,pad=0",
                facecolor='#ececec', edgecolor='none', linewidth=0
            )
            ax.add_patch(rect)

    # --- Grid lines (dashed) ---
    if "rowmajor" in outname:
        for c_inner in [0, 4]:
            x = grid_x0 + c_inner * cell
            ax.plot([x, x], [0, nrows * cell],
                    ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))
        for r_inner in [0, 1, 2, 3, 4]:
            y = r_inner * cell
            ax.plot([grid_x0, grid_x0 + ncols * cell], [y, y],
                    ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))
    else:
        for c_inner in [0, 2, 4]:
            x = grid_x0 + c_inner * cell
            ax.plot([x, x], [0, nrows * cell],
                    ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))
        for r_inner in [0, 2, 4]:
            y = r_inner * cell
            ax.plot([grid_x0, grid_x0 + ncols * cell], [y, y],
                    ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))

    # --- Inner cells: block ID + gray shade ---
    for r in range(nrows):
        for c in range(ncols):
            bid = block_grid[r, c]
            color = block_shade(bid)

            y = (nrows - 1 - r) * cell + offset
            x = grid_x0 + c * cell + offset
            rect = mpatches.FancyBboxPatch(
                (x, y), inner, inner,
                boxstyle="round,pad=0.02,rounding_size=0.08",
                facecolor=color, edgecolor='none', linewidth=0
            )
            ax.add_patch(rect)

            tc = 'white' if (bid % 4) < 2 else '#222'
            ax.text(grid_x0 + c * cell + cell / 2,
                    (nrows - 1 - r) * cell + cell / 2,
                    str(bid), ha='center', va='center',
                    fontsize=11, color=tc)

    # --- Memory ribbon ---
    ribbon_y = -0.9
    ribbon_h = 0.5
    block_w = ribbon_w / n_blocks

    ax.text(ribbon_w / 2, ribbon_y + ribbon_h + 0.25,
            ribbon_label, ha='center', va='center',
            fontsize=7, color='#555')

    for bid in range(n_blocks):
        x = ribbon_x0 + bid * block_w
        rect = mpatches.FancyBboxPatch(
            (x + 0.02, ribbon_y + 0.03), block_w - 0.04, ribbon_h - 0.06,
            boxstyle="round,pad=0.01,rounding_size=0.04",
            facecolor=block_shade(bid), edgecolor='#bbb', linewidth=0.3
        )
        ax.add_patch(rect)

        tc = 'white' if (bid % 4) < 2 else '#222'
        ax.text(x + block_w / 2, ribbon_y + ribbon_h / 2,
                str(bid), ha='center', va='center',
                fontsize=7, color=tc)

    # Block boundary lines
    for b in range(n_blocks + 1):
        x = ribbon_x0 + b * block_w
        ax.plot([x, x], [ribbon_y - 0.02, ribbon_y + ribbon_h + 0.02],
                ls='--', color='#aaaaaa', lw=0.8)

    # Block labels
    for b in range(n_blocks):
        x = ribbon_x0 + b * block_w + block_w / 2
        ax.text(x, ribbon_y - 0.18, f'B$_{b}$',
                ha='center', va='center', fontsize=6, color='#444')

    ax.set_xlim(-0.15, ribbon_w + 0.15)
    ax.set_ylim(ribbon_y - 0.45, nrows * cell + 0.15)
    ax.set_aspect('equal')
    ax.axis('off')

    plt.subplots_adjust(left=0.02, right=0.98, top=0.98, bottom=0.02)
    plt.savefig(f'{outname}.pdf', bbox_inches='tight', dpi=300)
    plt.savefig(f'{outname}.png', bbox_inches='tight', dpi=300)
    plt.close(fig)
    print(f"Done: {outname}")


draw_layout(block_grid_rowmajor, 'Row-Major Layout', 'block_rowmajor')
draw_layout(block_grid_tiled,    '2×2 Blocked Layout',  'block_tiled')