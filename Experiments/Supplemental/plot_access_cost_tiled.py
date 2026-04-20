import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

fig, ax = plt.subplots(1, 1, figsize=(1.8, 1.8))
ax.text(0.12+0.08, 1.01, r'$\Delta$', transform=ax.transAxes,
        ha='center', va='bottom', fontsize=10, color='#444')
ax.text(0.19+0.08, 1.04, '(Block Distance)', transform=ax.transAxes,
        ha='left', va='bottom', fontsize=5, color='#888')
nrows, ncols = 4, 4

# Ribbon defines the reference width
ribbon_w = 4.0
ribbon_x0 = 0.0
n_elems = nrows * ncols
elem_w = ribbon_w / n_elems

# Grid is 0.85x the ribbon width, centered above it
grid_w = 0.85 * ribbon_w
cell = grid_w / ncols
inner = 0.82 * cell
offset = (cell - inner) / 2
grid_x0 = (ribbon_w - grid_w) / 2

# --- Draw gray background cells ---
for r in range(nrows):
    for c in range(ncols):
        y = (nrows - 1 - r) * cell
        rect = mpatches.FancyBboxPatch(
            (grid_x0 + c * cell, y), cell, cell,
            boxstyle="square,pad=0",
            facecolor='#ececec', edgecolor='none', linewidth=0
        )
        ax.add_patch(rect)

# --- Inner (dashed) grid lines at 2x2 tile boundaries ---
for c_inner in [0, 2, 4]:
    x = grid_x0 + c_inner * cell
    ax.plot([x, x], [0, nrows * cell],
            ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))

for r_inner in [0, 2, 4]:
    y = r_inner * cell
    ax.plot([grid_x0, grid_x0 + ncols * cell], [y, y],
            ls='--', color='#aaaaaa', lw=0.5, dashes=(3, 3))

# --- Data: all 1s for tiled layout (no stride-2 within tile) ---
values = [
    [1, 0, 1, 0],
    [1, 0, 1, 0],
    [1, 0, 1, 0],
    [1, 0, 1, 0],
]

tile_colors = {
    (0, 0): '#e8706a',
    (0, 1): '#f0a050',
    (1, 0): '#5b9bd5',
    (1, 1): '#70c070',
}

inner_opacities = {
    (0, 0): 0.95,
    (0, 1): 0.55,
    (1, 0): 0.40,
    (1, 1): 0.25,
}

white = np.array([1.0, 1.0, 1.0])

for r in range(nrows):
    for c in range(ncols):
        tile_r, tile_c = r // 2, c // 2
        local_r, local_c = r % 2, c % 2
        base_hex = tile_colors[(tile_r, tile_c)]
        base = np.array([int(base_hex[i:i+2], 16) / 255 for i in (1, 3, 5)])
        a = inner_opacities[(local_r, local_c)]
        color = a * base + (1 - a) * white

        y = (nrows - 1 - r) * cell + offset
        x = grid_x0 + c * cell + offset

        rect = mpatches.FancyBboxPatch(
            (x, y), inner, inner,
            boxstyle="round,pad=0.02,rounding_size=0.08",
            facecolor=color, edgecolor='none', linewidth=0
        )
        ax.add_patch(rect)

        val = values[r][c]
        ax.text(grid_x0 + c * cell + cell / 2, (nrows - 1 - r) * cell + cell / 2,
                str(val),
                ha='center', va='center', fontsize=9,
                color='#222' if val > 0 else '#444')

# --- Arrows ---
access_order = []
for ti in range(2):
    for tj in range(2):
        for ii in range(2 * ti, 2 * ti + 2):
            for jj in range(2 * tj, 2 * tj + 2):
                access_order.append((ii, jj))

def cell_center(r, c):
    return (grid_x0 + c * cell + cell / 2, (nrows - 1 - r) * cell + cell / 2)

def cell_box(r, c):
    x0 = grid_x0 + c * cell + offset
    y0 = (nrows - 1 - r) * cell + offset
    return (x0, y0, x0 + inner, y0 + inner)

def edge_point(cx, cy, dx, dy, box):
    xmin, ymin, xmax, ymax = box
    margin = 0.04
    xmin += margin; ymin += margin; xmax -= margin; ymax -= margin
    t_candidates = []
    if dx != 0:
        for edge_x in [xmax, xmin]:
            t = (edge_x - cx) / dx
            if t > 0:
                hit_y = cy + dy * t
                if ymin <= hit_y <= ymax:
                    t_candidates.append(t)
    if dy != 0:
        for edge_y in [ymax, ymin]:
            t = (edge_y - cy) / dy
            if t > 0:
                hit_x = cx + dx * t
                if xmin <= hit_x <= xmax:
                    t_candidates.append(t)
    if not t_candidates:
        norm = (dx**2 + dy**2) ** 0.5
        return (cx + dx / norm * 0.35, cy + dy / norm * 0.35)
    t_min = min(t_candidates)
    return (cx + dx * t_min, cy + dy * t_min)

for idx in range(len(access_order) - 1):
    r0, c0 = access_order[idx]
    r1, c1 = access_order[idx + 1]
    cx0, cy0 = cell_center(r0, c0)
    cx1, cy1 = cell_center(r1, c1)
    dx = cx1 - cx0
    dy = cy1 - cy0
    tile0 = (r0 // 2, c0 // 2)
    tile1 = (r1 // 2, c1 // 2)
    if tile0 != tile1:
        margin = 0.04
        box0 = cell_box(r0, c0)
        box1 = cell_box(r1, c1)
        sx = box0[2] - margin if dx > 0 else box0[0] + margin
        sy = box0[3] - margin if dy > 0 else box0[1] + margin
        ex = box1[0] + margin if dx > 0 else box1[2] - margin
        ey = box1[1] + margin if dy > 0 else box1[3] - margin
        p0 = (sx, sy)
        p1 = (ex, ey)
    else:
        p0 = edge_point(cx0, cy0, dx, dy, cell_box(r0, c0))
        p1 = edge_point(cx1, cy1, -dx, -dy, cell_box(r1, c1))
    tile_r, tile_c = r0 // 2, c0 // 2
    base_hex = tile_colors[(tile_r, tile_c)]
    arrow = mpatches.FancyArrowPatch(
        p0, p1, arrowstyle='->', mutation_scale=7,
        color=base_hex, lw=0.8, alpha=0.7,
        shrinkA=0, shrinkB=0, zorder=5
    )
    ax.add_patch(arrow)

# --- Memory ribbon: 2x2 blocked layout ---
ribbon_y = -1.0
ribbon_h = 0.5

ax.text(ribbon_w / 2, ribbon_y + ribbon_h + 0.25,
        '2×2 Blocked Layout', ha='center', va='center',
        fontsize=7, color='#555')

blocked_order = []
for tile_r in range(nrows // 2):
    for tile_c in range(ncols // 2):
        for lr in range(2):
            for lc in range(2):
                blocked_order.append((tile_r * 2 + lr, tile_c * 2 + lc))

for addr, (r, c) in enumerate(blocked_order):
    tile_r, tile_c = r // 2, c // 2
    local_r, local_c = r % 2, c % 2
    base_hex = tile_colors[(tile_r, tile_c)]
    base = np.array([int(base_hex[i:i+2], 16) / 255 for i in (1, 3, 5)])
    a = inner_opacities[(local_r, local_c)]
    color = a * base + (1 - a) * white
    x = ribbon_x0 + addr * elem_w
    rect = mpatches.FancyBboxPatch(
        (x + 0.01, ribbon_y + 0.03), elem_w - 0.02, ribbon_h - 0.06,
        boxstyle="round,pad=0.01,rounding_size=0.03",
        facecolor=color, edgecolor='#bbb', linewidth=0.45
    )
    ax.add_patch(rect)

for b in range(0, n_elems + 1, 2):
    x = ribbon_x0 + b * elem_w
    ax.plot([x, x], [ribbon_y - 0.02, ribbon_y + ribbon_h + 0.02],
            ls='--', color='#aaaaaa', lw=1.0)

for b in range(n_elems // 2):
    x = ribbon_x0 + (b * 2 + 1) * elem_w
    ax.text(x, ribbon_y - 0.18, f'B$_{b}$',
            ha='center', va='center', fontsize=6, color='#444')

ax.set_xlim(-0.15, ribbon_w + 0.15)
ax.set_ylim(ribbon_y - 0.45, nrows * cell + 0.15)
ax.set_aspect('equal')
ax.axis('off')

plt.tight_layout()
plt.savefig('block_access_cost_tiled.pdf', bbox_inches='tight', dpi=300)
plt.savefig('block_access_cost_tiled.png', bbox_inches='tight', dpi=300)
print("Done: block_access_cost_tiled")