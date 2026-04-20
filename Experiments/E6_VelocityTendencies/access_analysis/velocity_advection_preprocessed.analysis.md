# Loop and Array Access Analysis: `velocity_tendencies`

| Symbol | Meaning |
|--------|---------|
| `h` | horizontal (nproma) dimension — loop vars `jc`, `je` (treated as equivalent) |
| `v` | vertical (nlev) dimension — loop var `jk` |
| `b` | block dimension — loop var `jb` |
| `S` | **structured** access — subscript is a direct loop variable |
| `U` | **unstructured** access — subscript wraps an indirection array (`icidx`, `ividx`, ...) |
| `C` | **constant** access — subscript is loop-invariant |

## Array Groups

Arrays in the same group have identical access patterns across all loops.

### FULLY STRUCTURED (9 arrays)

Patterns observed:

- `h:S  v:S  b:S`

Arrays:

- `p_diag%w_concorr_c`
- `p_metrics%coeff1_dwdz`
- `p_metrics%coeff2_dwdz`
- `p_metrics%ddqz_z_full_e`
- `p_metrics%ddqz_z_half`
- `p_metrics%ddxn_z_full`
- `p_metrics%ddxt_z_full`
- `p_metrics%wgtfac_c`
- `p_metrics%wgtfac_e`

### FULLY STRUCTURED (9 arrays)

Patterns observed:

- `h:S  b:S`

Arrays:

- `p_patch%cells%area`
- `p_patch%cells%decomp_info%owner_mask`
- `p_patch%edges%area_edge`
- `p_patch%edges%f_e`
- `p_patch%edges%fn_e`
- `p_patch%edges%ft_e`
- `p_patch%edges%inv_dual_edge_length`
- `p_patch%edges%inv_primal_edge_length`
- `p_patch%edges%tangent_orientation`

### FULLY STRUCTURED (7 arrays)

Patterns observed:

- `h:S  -:C  b:S`

Arrays:

- `p_diag%vn_ie_ubc`
- `p_int%c_lin_e`
- `p_int%e_bln_c_s`
- `p_int%geofac_grdiv`
- `p_int%geofac_n2s`
- `p_metrics%coeff_gradekin`
- `p_metrics%wgtfacq_e`

### HAS UNSTRUCTURED (5 arrays)

Patterns observed:

- `h:S  v:S  b:S`
- `h:U  v:S  h:U`

Arrays:

- `p_prog%w`
- `z_ekinh`
- `z_v_grad_w`
- `z_w_con_c_full`
- `z_w_concorr_me`

### FULLY STRUCTURED (5 arrays)

Patterns observed:

- `v:S`

Arrays:

- `levelmask`
- `p_metrics%deepatmo_gradh_ifc`
- `p_metrics%deepatmo_gradh_mc`
- `p_metrics%deepatmo_invr_ifc`
- `p_metrics%deepatmo_invr_mc`

### FULLY STRUCTURED (3 arrays)

Patterns observed:

- `h:S  -:C  b:S`
- `h:S  v:S  b:S`

Arrays:

- `p_diag%vn_ie`
- `p_diag%vt`
- `z_vt_ie`

### FULLY STRUCTURED (3 arrays)

Patterns observed:

- `h:S  v:S  b:S  -:C`

Arrays:

- `p_diag%ddt_vn_apc_pc`
- `p_diag%ddt_vn_cor_pc`
- `p_diag%ddt_w_adv_pc`

### HAS UNSTRUCTURED (2 arrays)

Patterns observed:

- `h:S  -:C  b:S`
- `h:S  v:S  b:S`
- `h:U  v:S  h:U`

Arrays:

- `p_prog%vn`
- `z_kin_hor_e`

### HAS UNSTRUCTURED (2 arrays)

Patterns observed:

- `h:U  v:S  h:U`

Arrays:

- `z_w_v`
- `zeta`

### FULLY STRUCTURED (2 arrays)

Patterns observed:

- `h:S  v:S`

Arrays:

- `cfl_clipping`
- `z_w_concorr_mc`

### FULLY STRUCTURED (1 arrays)

Patterns observed:

- `-:C  h:S  b:S`

Arrays:

- `p_int%rbf_vec_coeff_e`

### FULLY STRUCTURED (1 arrays)

Patterns observed:

- `h:S  -:C`
- `h:S  v:S`

Arrays:

- `z_w_con_c`

### FULLY STRUCTURED (1 arrays)

Patterns observed:

- `-:C  v:S`
- `b:S  v:S`

Arrays:

- `levmask`

## Per-Array Access Summary

> `S+U` marks arrays accessed both structured and unstructured (layout tradeoff)

| Array | Role:S/U Patterns | Conflict |
|-------|-------------------|----------|
| `cfl_clipping` | `h:S  v:S` |  |
| `levelmask` | `v:S` |  |
| `levmask` | `-:C  v:S` \| `b:S  v:S` |  |
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  b:S  -:C` |  |
| `p_diag%ddt_vn_cor_pc` | `h:S  v:S  b:S  -:C` |  |
| `p_diag%ddt_w_adv_pc` | `h:S  v:S  b:S  -:C` |  |
| `p_diag%vn_ie` | `h:S  -:C  b:S` \| `h:S  v:S  b:S` |  |
| `p_diag%vn_ie_ubc` | `h:S  -:C  b:S` |  |
| `p_diag%vt` | `h:S  -:C  b:S` \| `h:S  v:S  b:S` |  |
| `p_diag%w_concorr_c` | `h:S  v:S  b:S` |  |
| `p_int%c_lin_e` | `h:S  -:C  b:S` |  |
| `p_int%e_bln_c_s` | `h:S  -:C  b:S` |  |
| `p_int%geofac_grdiv` | `h:S  -:C  b:S` |  |
| `p_int%geofac_n2s` | `h:S  -:C  b:S` |  |
| `p_int%rbf_vec_coeff_e` | `-:C  h:S  b:S` |  |
| `p_metrics%coeff1_dwdz` | `h:S  v:S  b:S` |  |
| `p_metrics%coeff2_dwdz` | `h:S  v:S  b:S` |  |
| `p_metrics%coeff_gradekin` | `h:S  -:C  b:S` |  |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S  b:S` |  |
| `p_metrics%ddqz_z_half` | `h:S  v:S  b:S` |  |
| `p_metrics%ddxn_z_full` | `h:S  v:S  b:S` |  |
| `p_metrics%ddxt_z_full` | `h:S  v:S  b:S` |  |
| `p_metrics%deepatmo_gradh_ifc` | `v:S` |  |
| `p_metrics%deepatmo_gradh_mc` | `v:S` |  |
| `p_metrics%deepatmo_invr_ifc` | `v:S` |  |
| `p_metrics%deepatmo_invr_mc` | `v:S` |  |
| `p_metrics%wgtfac_c` | `h:S  v:S  b:S` |  |
| `p_metrics%wgtfac_e` | `h:S  v:S  b:S` |  |
| `p_metrics%wgtfacq_e` | `h:S  -:C  b:S` |  |
| `p_patch%cells%area` | `h:S  b:S` |  |
| `p_patch%cells%decomp_info%owner_mask` | `h:S  b:S` |  |
| `p_patch%edges%area_edge` | `h:S  b:S` |  |
| `p_patch%edges%f_e` | `h:S  b:S` |  |
| `p_patch%edges%fn_e` | `h:S  b:S` |  |
| `p_patch%edges%ft_e` | `h:S  b:S` |  |
| `p_patch%edges%inv_dual_edge_length` | `h:S  b:S` |  |
| `p_patch%edges%inv_primal_edge_length` | `h:S  b:S` |  |
| `p_patch%edges%tangent_orientation` | `h:S  b:S` |  |
| `p_prog%vn` | `h:S  -:C  b:S` \| `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `p_prog%w` | `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_ekinh` | `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_kin_hor_e` | `h:S  -:C  b:S` \| `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_v_grad_w` | `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_vt_ie` | `h:S  -:C  b:S` \| `h:S  v:S  b:S` |  |
| `z_w_con_c` | `h:S  -:C` \| `h:S  v:S` |  |
| `z_w_con_c_full` | `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_w_concorr_mc` | `h:S  v:S` |  |
| `z_w_concorr_me` | `h:S  v:S  b:S` \| `h:U  v:S  h:U` |  |
| `z_w_v` | `h:U  v:S  h:U` |  |
| `zeta` | `h:U  v:S  h:U` |  |

## Loop Shape Counts

| Count | Shape | Ranges | Behavior |
|------:|-------|--------|----------|
| 10 | `v.h` | full_vert, full_horiz |  |
| 9 | `v.h` | partial_vert, full_horiz |  |
| 3 | `b.h` | full_block, full_horiz |  |
| 1 | `b.v` | full_block, partial_vert |  |
| 1 | `v.h` | partial_vert, full_horiz | reduction |
| 1 | `v` | partial_vert |  |

## Loop Patterns (collapsed, block-stripped)

> **Collapsed**: if ANY array in the nest is `U` for a role, the whole nest is `U` for that role.

> Block dimension stripped from pattern key.

| Count | Shape | Ranges | Behavior | Collapsed S/U |
|------:|-------|--------|----------|---------------|
| 6 | `v.h` | full_vert, full_horiz |  | `h:U  v:S` |
| 5 | `v.h` | partial_vert, full_horiz |  | `h:S  v:S` |
| 4 | `v.h` | full_vert, full_horiz |  | `h:S  v:S` |
| 4 | `v.h` | partial_vert, full_horiz |  | `h:U  v:S` |
| 3 | `h` | full_horiz |  | `h:S` |
| 2 | `v` | partial_vert |  | `v:S` |
| 1 | `v.h` | partial_vert, full_horiz | reduction | `h:S  v:S` |

## Detailed Loop Nests

25 nests found.

### Nest 1: `b.h` (full_block, full_horiz)

Collapsed: `h:S  b:S`

```fortran
! inside DO jb = i_startblk:i_endblk
DO je=i_startidx,i_endidx
  p_diag%vn_ie(je, 1, jb) = p_prog%vn(je, 1, jb)
  z_vt_ie(je, 1, jb) = p_diag%vt(je, 1, jb)
  z_kin_hor_e(je, 1, jb) = 0.5_wp*(p_prog%vn(je, 1, jb)**2 + p_diag%vt(je, 1, jb)**2)
  p_diag%vn_ie(je, nlevp1, jb) = p_metrics%wgtfacq_e(je, 1, jb)*p_prog%vn(je, nlev, jb) + p_metrics%wgtfacq_e(je, 2, jb) &
  & *p_prog%vn(je, nlev - 1, jb) + p_metrics%wgtfacq_e(je, 3, jb)*p_prog%vn(je, nlev - 2, jb)
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%vn_ie` | `h:S  -:C  b:S` |
| `p_prog%vn` | `h:S  -:C  b:S` |
| `z_vt_ie` | `h:S  -:C  b:S` |
| `p_diag%vt` | `h:S  -:C  b:S` |
| `z_kin_hor_e` | `h:S  -:C  b:S` |
| `p_metrics%wgtfacq_e` | `h:S  -:C  b:S` |

</details>

### Nest 2: `b.h` (full_block, full_horiz)

Collapsed: `h:S  b:S`

```fortran
! inside DO jb = i_startblk:i_endblk
DO je=i_startidx,i_endidx
  p_diag%vn_ie(je, 1, jb) = p_diag%vn_ie_ubc(je, 1, jb) + dt_linintp_ubc*p_diag%vn_ie_ubc(je, 2, jb)
  z_vt_ie(je, 1, jb) = p_diag%vt(je, 1, jb)
  z_kin_hor_e(je, 1, jb) = 0.5_wp*(p_prog%vn(je, 1, jb)**2 + p_diag%vt(je, 1, jb)**2)
  p_diag%vn_ie(je, nlevp1, jb) = p_metrics%wgtfacq_e(je, 1, jb)*p_prog%vn(je, nlev, jb) + p_metrics%wgtfacq_e(je, 2, jb) &
  & *p_prog%vn(je, nlev - 1, jb) + p_metrics%wgtfacq_e(je, 3, jb)*p_prog%vn(je, nlev - 2, jb)
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%vn_ie` | `h:S  -:C  b:S` |
| `p_diag%vn_ie_ubc` | `h:S  -:C  b:S` |
| `z_vt_ie` | `h:S  -:C  b:S` |
| `p_diag%vt` | `h:S  -:C  b:S` |
| `z_kin_hor_e` | `h:S  -:C  b:S` |
| `p_prog%vn` | `h:S  -:C  b:S` |
| `p_metrics%wgtfacq_e` | `h:S  -:C  b:S` |

</details>

### Nest 3: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%vt(je, jk, jb) = p_int%rbf_vec_coeff_e(1, je, jb)*p_prog%vn(iqidx(je, jb, 1), jk, iqblk(je, jb, 1)) +  &
    & p_int%rbf_vec_coeff_e(2, je, jb)*p_prog%vn(iqidx(je, jb, 2), jk, iqblk(je, jb, 2)) + p_int%rbf_vec_coeff_e(3, je, jb) &
    & *p_prog%vn(iqidx(je, jb, 3), jk, iqblk(je, jb, 3)) + p_int%rbf_vec_coeff_e(4, je, jb)*p_prog%vn(iqidx(je, jb, 4), jk,  &
    & iqblk(je, jb, 4))
    IF (jk >= 2) THEN
      p_diag%vn_ie(je, jk, jb) =  &
      & p_metrics%wgtfac_e(je, jk, jb)*p_prog%vn(je, jk, jb) + (1._wp - p_metrics%wgtfac_e(je, jk, jb))*p_prog%vn(je, jk - 1, jb)
      z_kin_hor_e(je, jk, jb) = 0.5_wp*(p_prog%vn(je, jk, jb)**2 + p_diag%vt(je, jk, jb)**2)
    END IF
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_int%rbf_vec_coeff_e` | `-:C  h:S  b:S` |
| `p_prog%vn` | `h:U  v:S  h:U` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` |
| `p_metrics%wgtfac_e` | `h:S  v:S  b:S` |
| `p_prog%vn` | `h:S  v:S  b:S` |
| `z_kin_hor_e` | `h:S  v:S  b:S` |

</details>

### Nest 4: `v.h` (partial_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=2,nlev
  DO je=i_startidx,i_endidx
    z_vt_ie(je, jk, jb) =  &
    & p_metrics%wgtfac_e(je, jk, jb)*p_diag%vt(je, jk, jb) + (1._wp - p_metrics%wgtfac_e(je, jk, jb))*p_diag%vt(je, jk - 1, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_vt_ie` | `h:S  v:S  b:S` |
| `p_metrics%wgtfac_e` | `h:S  v:S  b:S` |
| `p_diag%vt` | `h:S  v:S  b:S` |

</details>

### Nest 5: `v.h` (partial_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=nflatlev_jg,nlev
  DO je=i_startidx,i_endidx
    z_w_concorr_me(je, jk, jb) =  &
    & p_prog%vn(je, jk, jb)*p_metrics%ddxn_z_full(je, jk, jb) + p_diag%vt(je, jk, jb)*p_metrics%ddxt_z_full(je, jk, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_concorr_me` | `h:S  v:S  b:S` |
| `p_prog%vn` | `h:S  v:S  b:S` |
| `p_metrics%ddxn_z_full` | `h:S  v:S  b:S` |
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_metrics%ddxt_z_full` | `h:S  v:S  b:S` |

</details>

### Nest 6: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    z_v_grad_w(je, jk, jb) = p_diag%vn_ie(je, jk, jb)*p_patch%edges%inv_dual_edge_length(je, jb)*(p_prog%w(icidx(je, jb, 1), jk,  &
    & icblk(je, jb, 1)) - p_prog%w(icidx(je, jb, 2), jk, icblk(je, jb, 2))) + z_vt_ie(je, jk, jb) &
    & *p_patch%edges%inv_primal_edge_length(je, jb)*p_patch%edges%tangent_orientation(je, jb)*(z_w_v(ividx(je, jb, 1), jk,  &
    & ivblk(je, jb, 1)) - z_w_v(ividx(je, jb, 2), jk, ivblk(je, jb, 2)))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_v_grad_w` | `h:S  v:S  b:S` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` |
| `p_patch%edges%inv_dual_edge_length` | `h:S  b:S` |
| `p_prog%w` | `h:U  v:S  h:U` |
| `z_vt_ie` | `h:S  v:S  b:S` |
| `p_patch%edges%inv_primal_edge_length` | `h:S  b:S` |
| `p_patch%edges%tangent_orientation` | `h:S  b:S` |
| `z_w_v` | `h:U  v:S  h:U` |

</details>

### Nest 7: `v.h` (full_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    z_v_grad_w(je, jk, jb) = z_v_grad_w(je, jk, jb)*p_metrics%deepatmo_gradh_ifc(jk) + p_diag%vn_ie(je, jk, jb)*(p_diag%vn_ie(je, &
    &  jk, jb)*p_metrics%deepatmo_invr_ifc(jk) - p_patch%edges%ft_e(je, jb)) + z_vt_ie(je, jk, jb)*(z_vt_ie(je, jk, jb) &
    & *p_metrics%deepatmo_invr_ifc(jk) + p_patch%edges%fn_e(je, jb))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_v_grad_w` | `h:S  v:S  b:S` |
| `p_metrics%deepatmo_gradh_ifc` | `v:S` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` |
| `p_metrics%deepatmo_invr_ifc` | `v:S` |
| `p_patch%edges%ft_e` | `h:S  b:S` |
| `z_vt_ie` | `h:S  v:S  b:S` |
| `p_patch%edges%fn_e` | `h:S  b:S` |

</details>

### Nest 8: `b.h` (full_block, full_horiz)

Collapsed: `h:S`

```fortran
! inside DO jb = i_startblk:i_endblk
DO jc=i_startidx,i_endidx
  z_w_con_c(jc, nlevp1) = 0.0_vp
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_con_c` | `h:S  -:C` |

</details>

### Nest 9: `b.v` (full_block, partial_vert)

Collapsed: `v:S  b:S`

```fortran
! inside DO jb = i_startblk:i_endblk
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  levmask(jb, jk) = .false.
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `levmask` | `b:S  v:S` |

</details>

### Nest 10: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO jc=i_startidx,i_endidx
    z_ekinh(jc, jk, jb) = p_int%e_bln_c_s(jc, 1, jb)*z_kin_hor_e(ieidx(jc, jb, 1), jk, ieblk(jc, jb, 1)) + p_int%e_bln_c_s(jc, 2, &
    &  jb)*z_kin_hor_e(ieidx(jc, jb, 2), jk, ieblk(jc, jb, 2)) + p_int%e_bln_c_s(jc, 3, jb)*z_kin_hor_e(ieidx(jc, jb, 3), jk,  &
    & ieblk(jc, jb, 3))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_ekinh` | `h:S  v:S  b:S` |
| `p_int%e_bln_c_s` | `h:S  -:C  b:S` |
| `z_kin_hor_e` | `h:U  v:S  h:U` |

</details>

### Nest 11: `v.h` (partial_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=nflatlev_jg,nlev
  DO jc=i_startidx,i_endidx
    z_w_concorr_mc(jc, jk) = p_int%e_bln_c_s(jc, 1, jb)*z_w_concorr_me(ieidx(jc, jb, 1), jk, ieblk(jc, jb, 1)) +  &
    & p_int%e_bln_c_s(jc, 2, jb)*z_w_concorr_me(ieidx(jc, jb, 2), jk, ieblk(jc, jb, 2)) + p_int%e_bln_c_s(jc, 3, jb) &
    & *z_w_concorr_me(ieidx(jc, jb, 3), jk, ieblk(jc, jb, 3))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_concorr_mc` | `h:S  v:S` |
| `p_int%e_bln_c_s` | `h:S  -:C  b:S` |
| `z_w_concorr_me` | `h:U  v:S  h:U` |

</details>

### Nest 12: `v.h` (partial_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=nflatlev_jg + 1,nlev
  DO jc=i_startidx,i_endidx
    p_diag%w_concorr_c(jc, jk, jb) =  &
    & p_metrics%wgtfac_c(jc, jk, jb)*z_w_concorr_mc(jc, jk) + (1._vp - p_metrics%wgtfac_c(jc, jk, jb))*z_w_concorr_mc(jc, jk - 1)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%w_concorr_c` | `h:S  v:S  b:S` |
| `p_metrics%wgtfac_c` | `h:S  v:S  b:S` |
| `z_w_concorr_mc` | `h:S  v:S` |

</details>

### Nest 13: `v.h` (full_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=1,nlev
  DO jc=i_startidx,i_endidx
    z_w_con_c(jc, jk) = p_prog%w(jc, jk, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_con_c` | `h:S  v:S` |
| `p_prog%w` | `h:S  v:S  b:S` |

</details>

### Nest 14: `v.h` (partial_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=nlev,nflatlev_jg + 1,-1
  DO jc=i_startidx,i_endidx
    z_w_con_c(jc, jk) = z_w_con_c(jc, jk) - p_diag%w_concorr_c(jc, jk, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_con_c` | `h:S  v:S` |
| `p_diag%w_concorr_c` | `h:S  v:S  b:S` |

</details>

### Nest 15: `v.h` (partial_vert, full_horiz) `reduction`

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  DO jc=i_startidx,i_endidx
    cfl_clipping(jc, jk) = ABS(z_w_con_c(jc, jk)) > cfl_w_limit*p_metrics%ddqz_z_half(jc, jk, jb)
    IF (cfl_clipping(jc, jk)) THEN
      levmask(jb, jk) = .true.
      vcfl = z_w_con_c(jc, jk)*dtime / p_metrics%ddqz_z_half(jc, jk, jb)
      maxvcfl = MAX(maxvcfl, ABS(vcfl))
      IF (vcfl < -0.85_vp) THEN
        z_w_con_c(jc, jk) = -0.85_vp*p_metrics%ddqz_z_half(jc, jk, jb) / dtime
      ELSE IF (vcfl > 0.85_vp) THEN
        z_w_con_c(jc, jk) = 0.85_vp*p_metrics%ddqz_z_half(jc, jk, jb) / dtime
      END IF
    END IF
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `cfl_clipping` | `h:S  v:S` |
| `z_w_con_c` | `h:S  v:S` |
| `p_metrics%ddqz_z_half` | `h:S  v:S  b:S` |
| `levmask` | `b:S  v:S` |

</details>

### Nest 16: `v.h` (full_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=1,nlev
  DO jc=i_startidx,i_endidx
    z_w_con_c_full(jc, jk, jb) = 0.5_vp*(z_w_con_c(jc, jk) + z_w_con_c(jc, jk + 1))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `z_w_con_c_full` | `h:S  v:S  b:S` |
| `z_w_con_c` | `h:S  v:S` |

</details>

### Nest 17: `v.h` (partial_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=2,nlev
  DO jc=i_startidx_2,i_endidx_2
    p_diag%ddt_w_adv_pc(jc, jk, jb, ntnd) = -z_w_con_c(jc, jk)*(p_prog%w(jc, jk - 1, jb)*p_metrics%coeff1_dwdz(jc, jk, jb) -  &
    & p_prog%w(jc, jk + 1, jb)*p_metrics%coeff2_dwdz(jc, jk, jb) + p_prog%w(jc, jk, jb)*(p_metrics%coeff2_dwdz(jc, jk, jb) -  &
    & p_metrics%coeff1_dwdz(jc, jk, jb)))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_w_adv_pc` | `h:S  v:S  b:S  -:C` |
| `z_w_con_c` | `h:S  v:S` |
| `p_prog%w` | `h:S  v:S  b:S` |
| `p_metrics%coeff1_dwdz` | `h:S  v:S  b:S` |
| `p_metrics%coeff2_dwdz` | `h:S  v:S  b:S` |

</details>

### Nest 18: `v.h` (partial_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=2,nlev
  DO jc=i_startidx_2,i_endidx_2
    p_diag%ddt_w_adv_pc(jc, jk, jb, ntnd) = p_diag%ddt_w_adv_pc(jc, jk, jb, ntnd) + p_int%e_bln_c_s(jc, 1, jb) &
    & *z_v_grad_w(ieidx(jc, jb, 1), jk, ieblk(jc, jb, 1)) + p_int%e_bln_c_s(jc, 2, jb)*z_v_grad_w(ieidx(jc, jb, 2), jk, ieblk(jc, &
    &  jb, 2)) + p_int%e_bln_c_s(jc, 3, jb)*z_v_grad_w(ieidx(jc, jb, 3), jk, ieblk(jc, jb, 3))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_w_adv_pc` | `h:S  v:S  b:S  -:C` |
| `p_int%e_bln_c_s` | `h:S  -:C  b:S` |
| `z_v_grad_w` | `h:U  v:S  h:U` |

</details>

### Nest 19: `v.h` (partial_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  IF (levmask(jb, jk)) THEN
    DO jc=i_startidx_2,i_endidx_2
      IF (cfl_clipping(jc, jk) .and. p_patch%cells%decomp_info%owner_mask(jc, jb)) THEN
        difcoef = scalfac_exdiff*MIN(0.85_wp - cfl_w_limit*dtime, ABS(z_w_con_c(jc, jk))*dtime / p_metrics%ddqz_z_half(jc, jk, jb &
        & ) - cfl_w_limit*dtime)
        p_diag%ddt_w_adv_pc(jc, jk, jb, ntnd) = p_diag%ddt_w_adv_pc(jc, jk, jb, ntnd) + difcoef*p_patch%cells%area(jc, jb) &
        & *(p_prog%w(jc, jk, jb)*p_int%geofac_n2s(jc, 1, jb) + p_prog%w(incidx(jc, jb, 1), jk, incblk(jc, jb, 1)) &
        & *p_int%geofac_n2s(jc, 2, jb) + p_prog%w(incidx(jc, jb, 2), jk, incblk(jc, jb, 2))*p_int%geofac_n2s(jc, 3, jb) +  &
        & p_prog%w(incidx(jc, jb, 3), jk, incblk(jc, jb, 3))*p_int%geofac_n2s(jc, 4, jb))
      END IF
    END DO
  END IF
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `cfl_clipping` | `h:S  v:S` |
| `p_patch%cells%decomp_info%owner_mask` | `h:S  b:S` |
| `z_w_con_c` | `h:S  v:S` |
| `p_metrics%ddqz_z_half` | `h:S  v:S  b:S` |
| `p_diag%ddt_w_adv_pc` | `h:S  v:S  b:S  -:C` |
| `p_patch%cells%area` | `h:S  b:S` |
| `p_prog%w` | `h:S  v:S  b:S` |
| `p_int%geofac_n2s` | `h:S  -:C  b:S` |
| `p_prog%w` | `h:U  v:S  h:U` |

</details>

### Nest 20: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = -(z_kin_hor_e(je, jk, jb)*(p_metrics%coeff_gradekin(je, 1, jb) -  &
    & p_metrics%coeff_gradekin(je, 2, jb)) + p_metrics%coeff_gradekin(je, 2, jb)*z_ekinh(icidx(je, jb, 2), jk, icblk(je, jb, 2))  &
    & - p_metrics%coeff_gradekin(je, 1, jb)*z_ekinh(icidx(je, jb, 1), jk, icblk(je, jb, 1)) + p_diag%vt(je, jk, jb) &
    & *(p_patch%edges%f_e(je, jb) + 0.5_vp*(zeta(ividx(je, jb, 1), jk, ivblk(je, jb, 1)) + zeta(ividx(je, jb, 2), jk, ivblk(je,  &
    & jb, 2)))) + (p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1), jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb) &
    & *z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2)))*(p_diag%vn_ie(je, jk, jb) - p_diag%vn_ie(je, jk + 1, jb)) /  &
    & p_metrics%ddqz_z_full_e(je, jk, jb))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  b:S  -:C` |
| `z_kin_hor_e` | `h:S  v:S  b:S` |
| `p_metrics%coeff_gradekin` | `h:S  -:C  b:S` |
| `z_ekinh` | `h:U  v:S  h:U` |
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_patch%edges%f_e` | `h:S  b:S` |
| `zeta` | `h:U  v:S  h:U` |
| `p_int%c_lin_e` | `h:S  -:C  b:S` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S  b:S` |

</details>

### Nest 21: `v.h` (full_vert, full_horiz)

Collapsed: `h:S  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_cor_pc(je, jk, jb, ntnd) = -p_diag%vt(je, jk, jb)*p_patch%edges%f_e(je, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_vn_cor_pc` | `h:S  v:S  b:S  -:C` |
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_patch%edges%f_e` | `h:S  b:S` |

</details>

### Nest 22: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = -((z_kin_hor_e(je, jk, jb)*(p_metrics%coeff_gradekin(je, 1, jb) -  &
    & p_metrics%coeff_gradekin(je, 2, jb)) + p_metrics%coeff_gradekin(je, 2, jb)*z_ekinh(icidx(je, jb, 2), jk, icblk(je, jb, 2))  &
    & - p_metrics%coeff_gradekin(je, 1, jb)*z_ekinh(icidx(je, jb, 1), jk, icblk(je, jb, 1)))*p_metrics%deepatmo_gradh_mc(jk) +  &
    & p_diag%vt(je, jk, jb)*(p_patch%edges%f_e(je, jb) + 0.5_vp*(zeta(ividx(je, jb, 1), jk, ivblk(je, jb, 1)) + zeta(ividx(je,  &
    & jb, 2), jk, ivblk(je, jb, 2)))*p_metrics%deepatmo_gradh_mc(jk)) + (p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1) &
    & , jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb)*z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2))) &
    & *((p_diag%vn_ie(je, jk, jb) - p_diag%vn_ie(je, jk + 1, jb)) / p_metrics%ddqz_z_full_e(je, jk, jb) + p_prog%vn(je, jk, jb) &
    & *p_metrics%deepatmo_invr_mc(jk) - p_patch%edges%ft_e(je, jb)))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  b:S  -:C` |
| `z_kin_hor_e` | `h:S  v:S  b:S` |
| `p_metrics%coeff_gradekin` | `h:S  -:C  b:S` |
| `z_ekinh` | `h:U  v:S  h:U` |
| `p_metrics%deepatmo_gradh_mc` | `v:S` |
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_patch%edges%f_e` | `h:S  b:S` |
| `zeta` | `h:U  v:S  h:U` |
| `p_int%c_lin_e` | `h:S  -:C  b:S` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S  b:S` |
| `p_prog%vn` | `h:S  v:S  b:S` |
| `p_metrics%deepatmo_invr_mc` | `v:S` |
| `p_patch%edges%ft_e` | `h:S  b:S` |

</details>

### Nest 23: `v.h` (full_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_cor_pc(je, jk, jb, ntnd) = -(p_diag%vt(je, jk, jb)*p_patch%edges%f_e(je, jb) + (p_int%c_lin_e(je, 1, jb) &
    & *z_w_con_c_full(icidx(je, jb, 1), jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb)*z_w_con_c_full(icidx(je, jb, 2), jk,  &
    & icblk(je, jb, 2)))*(-p_patch%edges%ft_e(je, jb)))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_diag%ddt_vn_cor_pc` | `h:S  v:S  b:S  -:C` |
| `p_diag%vt` | `h:S  v:S  b:S` |
| `p_patch%edges%f_e` | `h:S  b:S` |
| `p_int%c_lin_e` | `h:S  -:C  b:S` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_patch%edges%ft_e` | `h:S  b:S` |

</details>

### Nest 24: `v.h` (partial_vert, full_horiz)

Collapsed: `h:U  v:S  b:S`

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 4
  IF (levelmask(jk) .or. levelmask(jk + 1)) THEN
    DO je=i_startidx,i_endidx
      w_con_e = p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1), jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb) &
      & *z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2))
      IF (ABS(w_con_e) > cfl_w_limit*p_metrics%ddqz_z_full_e(je, jk, jb)) THEN
        difcoef = scalfac_exdiff*MIN(0.85_wp - cfl_w_limit*dtime, ABS(w_con_e)*dtime / p_metrics%ddqz_z_full_e(je, jk, jb) -  &
        & cfl_w_limit*dtime)
        p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) + difcoef*p_patch%edges%area_edge(je, jb) &
        & *(p_int%geofac_grdiv(je, 1, jb)*p_prog%vn(je, jk, jb) + p_int%geofac_grdiv(je, 2, jb)*p_prog%vn(iqidx(je, jb, 1), jk,  &
        & iqblk(je, jb, 1)) + p_int%geofac_grdiv(je, 3, jb)*p_prog%vn(iqidx(je, jb, 2), jk, iqblk(je, jb, 2)) +  &
        & p_int%geofac_grdiv(je, 4, jb)*p_prog%vn(iqidx(je, jb, 3), jk, iqblk(je, jb, 3)) + p_int%geofac_grdiv(je, 5, jb) &
        & *p_prog%vn(iqidx(je, jb, 4), jk, iqblk(je, jb, 4)) + p_patch%edges%tangent_orientation(je, jb) &
        & *p_patch%edges%inv_primal_edge_length(je, jb)*(zeta(ividx(je, jb, 2), jk, ivblk(je, jb, 2)) - zeta(ividx(je, jb, 1),  &
        & jk, ivblk(je, jb, 1))))
      END IF
    END DO
  END IF
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `p_int%c_lin_e` | `h:S  -:C  b:S` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S  b:S` |
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  b:S  -:C` |
| `p_patch%edges%area_edge` | `h:S  b:S` |
| `p_int%geofac_grdiv` | `h:S  -:C  b:S` |
| `p_prog%vn` | `h:S  v:S  b:S` |
| `p_prog%vn` | `h:U  v:S  h:U` |
| `p_patch%edges%tangent_orientation` | `h:S  b:S` |
| `p_patch%edges%inv_primal_edge_length` | `h:S  b:S` |
| `zeta` | `h:U  v:S  h:U` |

</details>

### Nest 25: `v` (partial_vert)

Collapsed: `v:S`

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  levelmask(jk) = ANY(levmask(i_startblk:i_endblk, jk))
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U |
|-------|----------|
| `levelmask` | `v:S` |
| `levmask` | `-:C  v:S` |

</details>


---

# Single-Block View (`nblks=1`)

Block dimension stripped: `b.h` becomes `h`, `b:S`/`b:U` removed from all signatures.

## Loop Patterns (single-block, collapsed)

| Count | Shape | Ranges | Behavior | Collapsed S/U |
|------:|-------|--------|----------|---------------|
| 6 | `v.h` | full_vert, full_horiz |  | `h:U  v:S` |
| 5 | `v.h` | partial_vert, full_horiz |  | `h:S  v:S` |
| 4 | `v.h` | full_vert, full_horiz |  | `h:S  v:S` |
| 4 | `v.h` | partial_vert, full_horiz |  | `h:U  v:S` |
| 3 | `h` | full_horiz |  | `h:S` |
| 2 | `v` | partial_vert |  | `v:S` |
| 1 | `v.h` | partial_vert, full_horiz | reduction | `h:S  v:S` |

## Example Nest per Pattern Type

One representative nest from each pattern (the one touching the most arrays).

### `v.h` full_vert, full_horiz — collapsed `h:U  v:S` (6x)

15 unique arrays.

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = -((z_kin_hor_e(je, jk, jb)*(p_metrics%coeff_gradekin(je, 1, jb) -  &
    & p_metrics%coeff_gradekin(je, 2, jb)) + p_metrics%coeff_gradekin(je, 2, jb)*z_ekinh(icidx(je, jb, 2), jk, icblk(je, jb, 2))  &
    & - p_metrics%coeff_gradekin(je, 1, jb)*z_ekinh(icidx(je, jb, 1), jk, icblk(je, jb, 1)))*p_metrics%deepatmo_gradh_mc(jk) +  &
    & p_diag%vt(je, jk, jb)*(p_patch%edges%f_e(je, jb) + 0.5_vp*(zeta(ividx(je, jb, 1), jk, ivblk(je, jb, 1)) + zeta(ividx(je,  &
    & jb, 2), jk, ivblk(je, jb, 2)))*p_metrics%deepatmo_gradh_mc(jk)) + (p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1) &
    & , jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb)*z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2))) &
    & *((p_diag%vn_ie(je, jk, jb) - p_diag%vn_ie(je, jk + 1, jb)) / p_metrics%ddqz_z_full_e(je, jk, jb) + p_prog%vn(je, jk, jb) &
    & *p_metrics%deepatmo_invr_mc(jk) - p_patch%edges%ft_e(je, jb)))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  -:C` |
| `z_kin_hor_e` | `h:S  v:S` |
| `p_metrics%coeff_gradekin` | `h:S  -:C` |
| `z_ekinh` | `h:U  v:S  h:U` |
| `p_metrics%deepatmo_gradh_mc` | `v:S` |
| `p_diag%vt` | `h:S  v:S` |
| `p_patch%edges%f_e` | `h:S` |
| `zeta` | `h:U  v:S  h:U` |
| `p_int%c_lin_e` | `h:S  -:C` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_diag%vn_ie` | `h:S  v:S` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S` |
| `p_prog%vn` | `h:S  v:S` |
| `p_metrics%deepatmo_invr_mc` | `v:S` |
| `p_patch%edges%ft_e` | `h:S` |

</details>

### `v.h` partial_vert, full_horiz — collapsed `h:S  v:S` (5x)

5 unique arrays.

```fortran
DO jk=nflatlev_jg,nlev
  DO je=i_startidx,i_endidx
    z_w_concorr_me(je, jk, jb) =  &
    & p_prog%vn(je, jk, jb)*p_metrics%ddxn_z_full(je, jk, jb) + p_diag%vt(je, jk, jb)*p_metrics%ddxt_z_full(je, jk, jb)
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `z_w_concorr_me` | `h:S  v:S` |
| `p_prog%vn` | `h:S  v:S` |
| `p_metrics%ddxn_z_full` | `h:S  v:S` |
| `p_diag%vt` | `h:S  v:S` |
| `p_metrics%ddxt_z_full` | `h:S  v:S` |

</details>

### `v.h` full_vert, full_horiz — collapsed `h:S  v:S` (4x)

7 unique arrays.

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    z_v_grad_w(je, jk, jb) = z_v_grad_w(je, jk, jb)*p_metrics%deepatmo_gradh_ifc(jk) + p_diag%vn_ie(je, jk, jb)*(p_diag%vn_ie(je, &
    &  jk, jb)*p_metrics%deepatmo_invr_ifc(jk) - p_patch%edges%ft_e(je, jb)) + z_vt_ie(je, jk, jb)*(z_vt_ie(je, jk, jb) &
    & *p_metrics%deepatmo_invr_ifc(jk) + p_patch%edges%fn_e(je, jb))
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `z_v_grad_w` | `h:S  v:S` |
| `p_metrics%deepatmo_gradh_ifc` | `v:S` |
| `p_diag%vn_ie` | `h:S  v:S` |
| `p_metrics%deepatmo_invr_ifc` | `v:S` |
| `p_patch%edges%ft_e` | `h:S` |
| `z_vt_ie` | `h:S  v:S` |
| `p_patch%edges%fn_e` | `h:S` |

</details>

### `v.h` partial_vert, full_horiz — collapsed `h:U  v:S` (4x)

10 unique arrays.

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 4
  IF (levelmask(jk) .or. levelmask(jk + 1)) THEN
    DO je=i_startidx,i_endidx
      w_con_e = p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1), jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb) &
      & *z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2))
      IF (ABS(w_con_e) > cfl_w_limit*p_metrics%ddqz_z_full_e(je, jk, jb)) THEN
        difcoef = scalfac_exdiff*MIN(0.85_wp - cfl_w_limit*dtime, ABS(w_con_e)*dtime / p_metrics%ddqz_z_full_e(je, jk, jb) -  &
        & cfl_w_limit*dtime)
        p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) + difcoef*p_patch%edges%area_edge(je, jb) &
        & *(p_int%geofac_grdiv(je, 1, jb)*p_prog%vn(je, jk, jb) + p_int%geofac_grdiv(je, 2, jb)*p_prog%vn(iqidx(je, jb, 1), jk,  &
        & iqblk(je, jb, 1)) + p_int%geofac_grdiv(je, 3, jb)*p_prog%vn(iqidx(je, jb, 2), jk, iqblk(je, jb, 2)) +  &
        & p_int%geofac_grdiv(je, 4, jb)*p_prog%vn(iqidx(je, jb, 3), jk, iqblk(je, jb, 3)) + p_int%geofac_grdiv(je, 5, jb) &
        & *p_prog%vn(iqidx(je, jb, 4), jk, iqblk(je, jb, 4)) + p_patch%edges%tangent_orientation(je, jb) &
        & *p_patch%edges%inv_primal_edge_length(je, jb)*(zeta(ividx(je, jb, 2), jk, ivblk(je, jb, 2)) - zeta(ividx(je, jb, 1),  &
        & jk, ivblk(je, jb, 1))))
      END IF
    END DO
  END IF
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `p_int%c_lin_e` | `h:S  -:C` |
| `z_w_con_c_full` | `h:U  v:S  h:U` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S` |
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  -:C` |
| `p_patch%edges%area_edge` | `h:S` |
| `p_int%geofac_grdiv` | `h:S  -:C` |
| `p_prog%vn` | `h:S  v:S` |
| `p_prog%vn` | `h:U  v:S  h:U` |
| `p_patch%edges%tangent_orientation` | `h:S` |
| `p_patch%edges%inv_primal_edge_length` | `h:S` |
| `zeta` | `h:U  v:S  h:U` |

</details>

### `h` full_horiz — collapsed `h:S` (3x)

7 unique arrays.

```fortran
! inside DO jb = i_startblk:i_endblk
DO je=i_startidx,i_endidx
  p_diag%vn_ie(je, 1, jb) = p_diag%vn_ie_ubc(je, 1, jb) + dt_linintp_ubc*p_diag%vn_ie_ubc(je, 2, jb)
  z_vt_ie(je, 1, jb) = p_diag%vt(je, 1, jb)
  z_kin_hor_e(je, 1, jb) = 0.5_wp*(p_prog%vn(je, 1, jb)**2 + p_diag%vt(je, 1, jb)**2)
  p_diag%vn_ie(je, nlevp1, jb) = p_metrics%wgtfacq_e(je, 1, jb)*p_prog%vn(je, nlev, jb) + p_metrics%wgtfacq_e(je, 2, jb) &
  & *p_prog%vn(je, nlev - 1, jb) + p_metrics%wgtfacq_e(je, 3, jb)*p_prog%vn(je, nlev - 2, jb)
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `p_diag%vn_ie` | `h:S  -:C` |
| `p_diag%vn_ie_ubc` | `h:S  -:C` |
| `z_vt_ie` | `h:S  -:C` |
| `p_diag%vt` | `h:S  -:C` |
| `z_kin_hor_e` | `h:S  -:C` |
| `p_prog%vn` | `h:S  -:C` |
| `p_metrics%wgtfacq_e` | `h:S  -:C` |

</details>

### `v` partial_vert — collapsed `v:S` (2x)

2 unique arrays.

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  levelmask(jk) = ANY(levmask(i_startblk:i_endblk, jk))
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `levelmask` | `v:S` |
| `levmask` | `-:C  v:S` |

</details>

### `v.h` partial_vert, full_horiz `reduction` — collapsed `h:S  v:S` (1x)

4 unique arrays.

```fortran
DO jk=MAX(3, nrdmax_jg - 2),nlev - 3
  DO jc=i_startidx,i_endidx
    cfl_clipping(jc, jk) = ABS(z_w_con_c(jc, jk)) > cfl_w_limit*p_metrics%ddqz_z_half(jc, jk, jb)
    IF (cfl_clipping(jc, jk)) THEN
      levmask(jb, jk) = .true.
      vcfl = z_w_con_c(jc, jk)*dtime / p_metrics%ddqz_z_half(jc, jk, jb)
      maxvcfl = MAX(maxvcfl, ABS(vcfl))
      IF (vcfl < -0.85_vp) THEN
        z_w_con_c(jc, jk) = -0.85_vp*p_metrics%ddqz_z_half(jc, jk, jb) / dtime
      ELSE IF (vcfl > 0.85_vp) THEN
        z_w_con_c(jc, jk) = 0.85_vp*p_metrics%ddqz_z_half(jc, jk, jb) / dtime
      END IF
    END IF
  END DO
END DO
```

<details><summary>Array access table</summary>

| Array | Role:S/U (no block) |
|-------|---------------------|
| `cfl_clipping` | `h:S  v:S` |
| `z_w_con_c` | `h:S  v:S` |
| `p_metrics%ddqz_z_half` | `h:S  v:S` |
| `levmask` | `v:S` |

</details>

## Widest Nest (most unique array references)

**15 unique arrays** accessed in a single nest.

- Shape: `v.h` (single-block: `v.h`)
- Collapsed: `h:U  v:S`

```fortran
DO jk=1,nlev
  DO je=i_startidx,i_endidx
    p_diag%ddt_vn_apc_pc(je, jk, jb, ntnd) = -((z_kin_hor_e(je, jk, jb)*(p_metrics%coeff_gradekin(je, 1, jb) -  &
    & p_metrics%coeff_gradekin(je, 2, jb)) + p_metrics%coeff_gradekin(je, 2, jb)*z_ekinh(icidx(je, jb, 2), jk, icblk(je, jb, 2))  &
    & - p_metrics%coeff_gradekin(je, 1, jb)*z_ekinh(icidx(je, jb, 1), jk, icblk(je, jb, 1)))*p_metrics%deepatmo_gradh_mc(jk) +  &
    & p_diag%vt(je, jk, jb)*(p_patch%edges%f_e(je, jb) + 0.5_vp*(zeta(ividx(je, jb, 1), jk, ivblk(je, jb, 1)) + zeta(ividx(je,  &
    & jb, 2), jk, ivblk(je, jb, 2)))*p_metrics%deepatmo_gradh_mc(jk)) + (p_int%c_lin_e(je, 1, jb)*z_w_con_c_full(icidx(je, jb, 1) &
    & , jk, icblk(je, jb, 1)) + p_int%c_lin_e(je, 2, jb)*z_w_con_c_full(icidx(je, jb, 2), jk, icblk(je, jb, 2))) &
    & *((p_diag%vn_ie(je, jk, jb) - p_diag%vn_ie(je, jk + 1, jb)) / p_metrics%ddqz_z_full_e(je, jk, jb) + p_prog%vn(je, jk, jb) &
    & *p_metrics%deepatmo_invr_mc(jk) - p_patch%edges%ft_e(je, jb)))
  END DO
END DO
```

| Array | Role:S/U (full) | Role:S/U (no block) |
|-------|-----------------|---------------------|
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  b:S  -:C` | `h:S  v:S  -:C` |
| `z_kin_hor_e` | `h:S  v:S  b:S` | `h:S  v:S` |
| `p_metrics%coeff_gradekin` | `h:S  -:C  b:S` | `h:S  -:C` |
| `z_ekinh` | `h:U  v:S  h:U` | `h:U  v:S  h:U` |
| `p_metrics%deepatmo_gradh_mc` | `v:S` | `v:S` |
| `p_diag%vt` | `h:S  v:S  b:S` | `h:S  v:S` |
| `p_patch%edges%f_e` | `h:S  b:S` | `h:S` |
| `zeta` | `h:U  v:S  h:U` | `h:U  v:S  h:U` |
| `p_int%c_lin_e` | `h:S  -:C  b:S` | `h:S  -:C` |
| `z_w_con_c_full` | `h:U  v:S  h:U` | `h:U  v:S  h:U` |
| `p_diag%vn_ie` | `h:S  v:S  b:S` | `h:S  v:S` |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S  b:S` | `h:S  v:S` |
| `p_prog%vn` | `h:S  v:S  b:S` | `h:S  v:S` |
| `p_metrics%deepatmo_invr_mc` | `v:S` | `v:S` |
| `p_patch%edges%ft_e` | `h:S  b:S` | `h:S` |

## Array Groups (single-block)

### FULLY STRUCTURED (11 arrays)

Patterns: `h:S  v:S`

- `cfl_clipping`
- `p_diag%w_concorr_c`
- `p_metrics%coeff1_dwdz`
- `p_metrics%coeff2_dwdz`
- `p_metrics%ddqz_z_full_e`
- `p_metrics%ddqz_z_half`
- `p_metrics%ddxn_z_full`
- `p_metrics%ddxt_z_full`
- `p_metrics%wgtfac_c`
- `p_metrics%wgtfac_e`
- `z_w_concorr_mc`

### FULLY STRUCTURED (9 arrays)

Patterns: `h:S`

- `p_patch%cells%area`
- `p_patch%cells%decomp_info%owner_mask`
- `p_patch%edges%area_edge`
- `p_patch%edges%f_e`
- `p_patch%edges%fn_e`
- `p_patch%edges%ft_e`
- `p_patch%edges%inv_dual_edge_length`
- `p_patch%edges%inv_primal_edge_length`
- `p_patch%edges%tangent_orientation`

### FULLY STRUCTURED (7 arrays)

Patterns: `h:S  -:C`

- `p_diag%vn_ie_ubc`
- `p_int%c_lin_e`
- `p_int%e_bln_c_s`
- `p_int%geofac_grdiv`
- `p_int%geofac_n2s`
- `p_metrics%coeff_gradekin`
- `p_metrics%wgtfacq_e`

### HAS UNSTRUCTURED (5 arrays)

Patterns: `h:S  v:S`, `h:U  v:S  h:U`

- `p_prog%w`
- `z_ekinh`
- `z_v_grad_w`
- `z_w_con_c_full`
- `z_w_concorr_me`

### FULLY STRUCTURED (5 arrays)

Patterns: `v:S`

- `levelmask`
- `p_metrics%deepatmo_gradh_ifc`
- `p_metrics%deepatmo_gradh_mc`
- `p_metrics%deepatmo_invr_ifc`
- `p_metrics%deepatmo_invr_mc`

### FULLY STRUCTURED (4 arrays)

Patterns: `h:S  -:C`, `h:S  v:S`

- `p_diag%vn_ie`
- `p_diag%vt`
- `z_vt_ie`
- `z_w_con_c`

### FULLY STRUCTURED (3 arrays)

Patterns: `h:S  v:S  -:C`

- `p_diag%ddt_vn_apc_pc`
- `p_diag%ddt_vn_cor_pc`
- `p_diag%ddt_w_adv_pc`

### HAS UNSTRUCTURED (2 arrays)

Patterns: `h:S  -:C`, `h:S  v:S`, `h:U  v:S  h:U`

- `p_prog%vn`
- `z_kin_hor_e`

### HAS UNSTRUCTURED (2 arrays)

Patterns: `h:U  v:S  h:U`

- `z_w_v`
- `zeta`

### FULLY STRUCTURED (1 arrays)

Patterns: `-:C  h:S`

- `p_int%rbf_vec_coeff_e`

### FULLY STRUCTURED (1 arrays)

Patterns: `-:C  v:S`, `v:S`

- `levmask`

## Per-Array Summary (single-block)

| Array | Role:S/U Patterns | Conflict |
|-------|-------------------|----------|
| `cfl_clipping` | `h:S  v:S` |  |
| `levelmask` | `v:S` |  |
| `levmask` | `-:C  v:S` \| `v:S` |  |
| `p_diag%ddt_vn_apc_pc` | `h:S  v:S  -:C` |  |
| `p_diag%ddt_vn_cor_pc` | `h:S  v:S  -:C` |  |
| `p_diag%ddt_w_adv_pc` | `h:S  v:S  -:C` |  |
| `p_diag%vn_ie` | `h:S  -:C` \| `h:S  v:S` |  |
| `p_diag%vn_ie_ubc` | `h:S  -:C` |  |
| `p_diag%vt` | `h:S  -:C` \| `h:S  v:S` |  |
| `p_diag%w_concorr_c` | `h:S  v:S` |  |
| `p_int%c_lin_e` | `h:S  -:C` |  |
| `p_int%e_bln_c_s` | `h:S  -:C` |  |
| `p_int%geofac_grdiv` | `h:S  -:C` |  |
| `p_int%geofac_n2s` | `h:S  -:C` |  |
| `p_int%rbf_vec_coeff_e` | `-:C  h:S` |  |
| `p_metrics%coeff1_dwdz` | `h:S  v:S` |  |
| `p_metrics%coeff2_dwdz` | `h:S  v:S` |  |
| `p_metrics%coeff_gradekin` | `h:S  -:C` |  |
| `p_metrics%ddqz_z_full_e` | `h:S  v:S` |  |
| `p_metrics%ddqz_z_half` | `h:S  v:S` |  |
| `p_metrics%ddxn_z_full` | `h:S  v:S` |  |
| `p_metrics%ddxt_z_full` | `h:S  v:S` |  |
| `p_metrics%deepatmo_gradh_ifc` | `v:S` |  |
| `p_metrics%deepatmo_gradh_mc` | `v:S` |  |
| `p_metrics%deepatmo_invr_ifc` | `v:S` |  |
| `p_metrics%deepatmo_invr_mc` | `v:S` |  |
| `p_metrics%wgtfac_c` | `h:S  v:S` |  |
| `p_metrics%wgtfac_e` | `h:S  v:S` |  |
| `p_metrics%wgtfacq_e` | `h:S  -:C` |  |
| `p_patch%cells%area` | `h:S` |  |
| `p_patch%cells%decomp_info%owner_mask` | `h:S` |  |
| `p_patch%edges%area_edge` | `h:S` |  |
| `p_patch%edges%f_e` | `h:S` |  |
| `p_patch%edges%fn_e` | `h:S` |  |
| `p_patch%edges%ft_e` | `h:S` |  |
| `p_patch%edges%inv_dual_edge_length` | `h:S` |  |
| `p_patch%edges%inv_primal_edge_length` | `h:S` |  |
| `p_patch%edges%tangent_orientation` | `h:S` |  |
| `p_prog%vn` | `h:S  -:C` \| `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `p_prog%w` | `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_ekinh` | `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_kin_hor_e` | `h:S  -:C` \| `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_v_grad_w` | `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_vt_ie` | `h:S  -:C` \| `h:S  v:S` |  |
| `z_w_con_c` | `h:S  -:C` \| `h:S  v:S` |  |
| `z_w_con_c_full` | `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_w_concorr_mc` | `h:S  v:S` |  |
| `z_w_concorr_me` | `h:S  v:S` \| `h:U  v:S  h:U` |  |
| `z_w_v` | `h:U  v:S  h:U` |  |
| `zeta` | `h:U  v:S  h:U` |  |

---

# Set Analysis: `velocity_tendencies`

50 unique arrays across 25 loop nests.

## 1. Array Classification

Each array is classified by its participation across structured (S) and unstructured (U) nests.

| Class | Meaning |
|-------|---------|
| `S-only` | Appears exclusively in nests with no indirect access |
| `U-only` | Appears exclusively in nests that have indirect access (rare) |
| `bridge-U` | The array itself is accessed via indirection in ≥1 nest **and** appears in S-nests — layout conflict |
| `bridge-S` | Accessed only with structured subscripts, but co-occurs in a U-nest alongside indirect arrays |

### S-only (13 arrays)

| Array | Nests (count) | U-access nests |
|-------|---------------|----------------|
| `levelmask` | 25 (1) | — |
| `levmask` | 9, 15, 25 (3) | — |
| `p_diag%vn_ie_ubc` | 2 (1) | — |
| `p_diag%w_concorr_c` | 12, 14 (2) | — |
| `p_metrics%coeff1_dwdz` | 17 (1) | — |
| `p_metrics%coeff2_dwdz` | 17 (1) | — |
| `p_metrics%ddxn_z_full` | 5 (1) | — |
| `p_metrics%ddxt_z_full` | 5 (1) | — |
| `p_metrics%deepatmo_gradh_ifc` | 7 (1) | — |
| `p_metrics%deepatmo_invr_ifc` | 7 (1) | — |
| `p_metrics%wgtfac_c` | 12 (1) | — |
| `p_metrics%wgtfacq_e` | 1, 2 (2) | — |
| `p_patch%edges%fn_e` | 7 (1) | — |

### bridge-S (12 arrays)

| Array | Nests (count) | U-access nests |
|-------|---------------|----------------|
| `cfl_clipping` | 15, 19 (2) | — |
| `p_diag%ddt_vn_cor_pc` | 21, 23 (2) | — |
| `p_diag%ddt_w_adv_pc` | 17, 18, 19 (3) | — |
| `p_diag%vn_ie` | 1, 2, 3, 6, 7, 20, 22 (7) | — |
| `p_diag%vt` | 1, 2, 3, 4, 5, 20, 21, 22, 23 (9) | — |
| `p_metrics%ddqz_z_half` | 15, 19 (2) | — |
| `p_metrics%wgtfac_e` | 3, 4 (2) | — |
| `p_patch%edges%f_e` | 20, 21, 22, 23 (4) | — |
| `p_patch%edges%ft_e` | 7, 22, 23 (3) | — |
| `z_vt_ie` | 1, 2, 4, 6, 7 (5) | — |
| `z_w_con_c` | 8, 13, 14, 15, 16, 17, 19 (7) | — |
| `z_w_concorr_mc` | 11, 12 (2) | — |

### bridge-U (9 arrays)

| Array | Nests (count) | U-access nests |
|-------|---------------|----------------|
| `p_prog%vn` | 1, 2, 3, 5, 22, 24 (6) | 3, 24 |
| `p_prog%w` | 6, 13, 17, 19 (4) | 6, 19 |
| `z_ekinh` | 10, 20, 22 (3) | 20, 22 |
| `z_kin_hor_e` | 1, 2, 3, 10, 20, 22 (6) | 10 |
| `z_v_grad_w` | 6, 7, 18 (3) | 18 |
| `z_w_con_c_full` | 16, 20, 22, 23, 24 (5) | 20, 22, 23, 24 |
| `z_w_concorr_me` | 5, 11 (2) | 11 |
| `z_w_v` | 6 (1) | 6 |
| `zeta` | 20, 22, 24 (3) | 20, 22, 24 |

### U-only (16 arrays)

| Array | Nests (count) | U-access nests |
|-------|---------------|----------------|
| `p_diag%ddt_vn_apc_pc` | 20, 22, 24 (3) | — |
| `p_int%c_lin_e` | 20, 22, 23, 24 (4) | — |
| `p_int%e_bln_c_s` | 10, 11, 18 (3) | — |
| `p_int%geofac_grdiv` | 24 (1) | — |
| `p_int%geofac_n2s` | 19 (1) | — |
| `p_int%rbf_vec_coeff_e` | 3 (1) | — |
| `p_metrics%coeff_gradekin` | 20, 22 (2) | — |
| `p_metrics%ddqz_z_full_e` | 20, 22, 24 (3) | — |
| `p_metrics%deepatmo_gradh_mc` | 22 (1) | — |
| `p_metrics%deepatmo_invr_mc` | 22 (1) | — |
| `p_patch%cells%area` | 19 (1) | — |
| `p_patch%cells%decomp_info%owner_mask` | 19 (1) | — |
| `p_patch%edges%area_edge` | 24 (1) | — |
| `p_patch%edges%inv_dual_edge_length` | 6 (1) | — |
| `p_patch%edges%inv_primal_edge_length` | 6, 24 (2) | — |
| `p_patch%edges%tangent_orientation` | 6, 24 (2) | — |

**Summary**: 13 S-only, 12 bridge-S, 9 bridge-U, 16 U-only.

## 2. Per-Nest Working Sets

| Nest | Shape | Collapsed | Behavior | #Arrays | Arrays |
|-----:|-------|-----------|----------|--------:|--------|
| 1 | `b.h` S | `h:S  b:S` |  | 6 | `p_diag%vn_ie`, `p_diag%vt`, `p_metrics%wgtfacq_e`, `p_prog%vn`, `z_kin_hor_e`, `z_vt_ie` |
| 2 | `b.h` S | `h:S  b:S` |  | 7 | `p_diag%vn_ie`, `p_diag%vn_ie_ubc`, `p_diag%vt`, `p_metrics%wgtfacq_e`, `p_prog%vn`, `z_kin_hor_e`, `z_vt_ie` |
| 3 | `v.h` **U** | `h:U  v:S  b:S` |  | 6 | `p_diag%vn_ie`, `p_diag%vt`, `p_int%rbf_vec_coeff_e`, `p_metrics%wgtfac_e`, `p_prog%vn`, `z_kin_hor_e` |
| 4 | `v.h` S | `h:S  v:S  b:S` |  | 3 | `p_diag%vt`, `p_metrics%wgtfac_e`, `z_vt_ie` |
| 5 | `v.h` S | `h:S  v:S  b:S` |  | 5 | `p_diag%vt`, `p_metrics%ddxn_z_full`, `p_metrics%ddxt_z_full`, `p_prog%vn`, `z_w_concorr_me` |
| 6 | `v.h` **U** | `h:U  v:S  b:S` |  | 8 | `p_diag%vn_ie`, `p_patch%edges%inv_dual_edge_length`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%w`, `z_v_grad_w`, `z_vt_ie`, `z_w_v` |
| 7 | `v.h` S | `h:S  v:S  b:S` |  | 7 | `p_diag%vn_ie`, `p_metrics%deepatmo_gradh_ifc`, `p_metrics%deepatmo_invr_ifc`, `p_patch%edges%fn_e`, `p_patch%edges%ft_e`, `z_v_grad_w`, `z_vt_ie` |
| 8 | `b.h` S | `h:S` |  | 1 | `z_w_con_c` |
| 9 | `b.v` S | `v:S  b:S` |  | 1 | `levmask` |
| 10 | `v.h` **U** | `h:U  v:S  b:S` |  | 3 | `p_int%e_bln_c_s`, `z_ekinh`, `z_kin_hor_e` |
| 11 | `v.h` **U** | `h:U  v:S  b:S` |  | 3 | `p_int%e_bln_c_s`, `z_w_concorr_mc`, `z_w_concorr_me` |
| 12 | `v.h` S | `h:S  v:S  b:S` |  | 3 | `p_diag%w_concorr_c`, `p_metrics%wgtfac_c`, `z_w_concorr_mc` |
| 13 | `v.h` S | `h:S  v:S  b:S` |  | 2 | `p_prog%w`, `z_w_con_c` |
| 14 | `v.h` S | `h:S  v:S  b:S` |  | 2 | `p_diag%w_concorr_c`, `z_w_con_c` |
| 15 | `v.h` S | `h:S  v:S  b:S` | reduction | 4 | `cfl_clipping`, `levmask`, `p_metrics%ddqz_z_half`, `z_w_con_c` |
| 16 | `v.h` S | `h:S  v:S  b:S` |  | 2 | `z_w_con_c`, `z_w_con_c_full` |
| 17 | `v.h` S | `h:S  v:S  b:S` |  | 5 | `p_diag%ddt_w_adv_pc`, `p_metrics%coeff1_dwdz`, `p_metrics%coeff2_dwdz`, `p_prog%w`, `z_w_con_c` |
| 18 | `v.h` **U** | `h:U  v:S  b:S` |  | 3 | `p_diag%ddt_w_adv_pc`, `p_int%e_bln_c_s`, `z_v_grad_w` |
| 19 | `v.h` **U** | `h:U  v:S  b:S` |  | 8 | `cfl_clipping`, `p_diag%ddt_w_adv_pc`, `p_int%geofac_n2s`, `p_metrics%ddqz_z_half`, `p_patch%cells%area`, `p_patch%cells%decomp_info%owner_mask`, `p_prog%w`, `z_w_con_c` |
| 20 | `v.h` **U** | `h:U  v:S  b:S` |  | 11 | `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_patch%edges%f_e`, `z_ekinh`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta` |
| 21 | `v.h` S | `h:S  v:S  b:S` |  | 3 | `p_diag%ddt_vn_cor_pc`, `p_diag%vt`, `p_patch%edges%f_e` |
| 22 | `v.h` **U** | `h:U  v:S  b:S` |  | 15 | `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `p_prog%vn`, `z_ekinh`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta` |
| 23 | `v.h` **U** | `h:U  v:S  b:S` |  | 6 | `p_diag%ddt_vn_cor_pc`, `p_diag%vt`, `p_int%c_lin_e`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `z_w_con_c_full` |
| 24 | `v.h` **U** | `h:U  v:S  b:S` |  | 10 | `p_diag%ddt_vn_apc_pc`, `p_int%c_lin_e`, `p_int%geofac_grdiv`, `p_metrics%ddqz_z_full_e`, `p_patch%edges%area_edge`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%vn`, `z_w_con_c_full`, `zeta` |
| 25 | `v` S | `v:S` |  | 2 | `levelmask`, `levmask` |

## 3. Dimension Groups

Arrays grouped by their dimension signature (block dimension stripped). Only arrays with identical shapes can share an AoS/AoSoA record.

### Signature `(h, v)` — 15 arrays

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `cfl_clipping` | bridge-S | S | 2 |
| `p_diag%vn_ie` | bridge-S | S | 7 |
| `p_diag%vt` | bridge-S | S | 9 |
| `p_diag%w_concorr_c` | S-only | S | 2 |
| `p_metrics%coeff1_dwdz` | S-only | S | 1 |
| `p_metrics%coeff2_dwdz` | S-only | S | 1 |
| `p_metrics%ddqz_z_full_e` | U-only | S | 3 |
| `p_metrics%ddqz_z_half` | bridge-S | S | 2 |
| `p_metrics%ddxn_z_full` | S-only | S | 1 |
| `p_metrics%ddxt_z_full` | S-only | S | 1 |
| `p_metrics%wgtfac_c` | S-only | S | 1 |
| `p_metrics%wgtfac_e` | bridge-S | S | 2 |
| `z_vt_ie` | bridge-S | S | 5 |
| `z_w_con_c` | bridge-S | S | 7 |
| `z_w_concorr_mc` | bridge-S | S | 2 |

### Signature `(h, v, extra)` — 12 arrays ⚠ bridge-U

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `p_diag%ddt_vn_apc_pc` | U-only | S | 3 |
| `p_diag%ddt_vn_cor_pc` | bridge-S | S | 2 |
| `p_diag%ddt_w_adv_pc` | bridge-S | S | 3 |
| `p_prog%vn` | bridge-U | U | 6 |
| `p_prog%w` | bridge-U | U | 4 |
| `z_ekinh` | bridge-U | U | 3 |
| `z_kin_hor_e` | bridge-U | U | 6 |
| `z_v_grad_w` | bridge-U | U | 3 |
| `z_w_con_c_full` | bridge-U | U | 5 |
| `z_w_concorr_me` | bridge-U | U | 2 |
| `z_w_v` | bridge-U | U | 1 |
| `zeta` | bridge-U | U | 3 |

### Signature `(h)` — 9 arrays

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `p_patch%cells%area` | U-only | S | 1 |
| `p_patch%cells%decomp_info%owner_mask` | U-only | S | 1 |
| `p_patch%edges%area_edge` | U-only | S | 1 |
| `p_patch%edges%f_e` | bridge-S | S | 4 |
| `p_patch%edges%fn_e` | S-only | S | 1 |
| `p_patch%edges%ft_e` | bridge-S | S | 3 |
| `p_patch%edges%inv_dual_edge_length` | U-only | S | 1 |
| `p_patch%edges%inv_primal_edge_length` | U-only | S | 2 |
| `p_patch%edges%tangent_orientation` | U-only | S | 2 |

### Signature `(h, extra)` — 8 arrays

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `p_diag%vn_ie_ubc` | S-only | S | 1 |
| `p_int%c_lin_e` | U-only | S | 4 |
| `p_int%e_bln_c_s` | U-only | S | 3 |
| `p_int%geofac_grdiv` | U-only | S | 1 |
| `p_int%geofac_n2s` | U-only | S | 1 |
| `p_int%rbf_vec_coeff_e` | U-only | S | 1 |
| `p_metrics%coeff_gradekin` | U-only | S | 2 |
| `p_metrics%wgtfacq_e` | S-only | S | 2 |

### Signature `(v)` — 5 arrays

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `levelmask` | S-only | S | 1 |
| `p_metrics%deepatmo_gradh_ifc` | S-only | S | 1 |
| `p_metrics%deepatmo_gradh_mc` | U-only | S | 1 |
| `p_metrics%deepatmo_invr_ifc` | S-only | S | 1 |
| `p_metrics%deepatmo_invr_mc` | U-only | S | 1 |

### Signature `(v, extra)` — 1 arrays

| Array | Class | S/U | Nests |
|-------|-------|-----|------:|
| `levmask` | S-only | S | 3 |

## 4. Array Co-occurrence (top pairs)

Pairs of arrays that appear together in the most nests.

| Array A | Array B | Co-occurrence | Jaccard |
|---------|---------|:------------:|:-------:|
| `p_diag%vn_ie` | `p_diag%vt` | 5 | 0.45 ⚠ |
| `p_diag%vn_ie` | `z_kin_hor_e` | 5 | 0.62 ⚠ |
| `p_diag%vt` | `p_prog%vn` | 5 | 0.50 ⚠ |
| `p_diag%vt` | `z_kin_hor_e` | 5 | 0.50 ⚠ |
| `p_diag%vn_ie` | `p_prog%vn` | 4 | 0.44 ⚠ |
| `p_diag%vn_ie` | `z_vt_ie` | 4 | 0.50 ⚠ |
| `p_prog%vn` | `z_kin_hor_e` | 4 | 0.50 ⚠ |
| `p_diag%vt` | `p_patch%edges%f_e` | 4 | 0.44 ⚠ |
| `p_int%c_lin_e` | `z_w_con_c_full` | 4 | 0.80 ⚠ |
| `p_diag%vt` | `z_vt_ie` | 3 | 0.27 ⚠ |
| `z_ekinh` | `z_kin_hor_e` | 3 | 0.50 ⚠ |
| `p_prog%w` | `z_w_con_c` | 3 | 0.38 ⚠ |
| `p_diag%ddt_vn_apc_pc` | `p_int%c_lin_e` | 3 | 0.75 |
| `p_diag%ddt_vn_apc_pc` | `p_metrics%ddqz_z_full_e` | 3 | 1.00 |
| `p_diag%ddt_vn_apc_pc` | `z_w_con_c_full` | 3 | 0.60 ⚠ |
| `p_diag%ddt_vn_apc_pc` | `zeta` | 3 | 1.00 ⚠ |
| `p_diag%vt` | `p_int%c_lin_e` | 3 | 0.30 ⚠ |
| `p_diag%vt` | `z_w_con_c_full` | 3 | 0.27 ⚠ |
| `p_int%c_lin_e` | `p_metrics%ddqz_z_full_e` | 3 | 0.75 |
| `p_int%c_lin_e` | `p_patch%edges%f_e` | 3 | 0.60 ⚠ |
| `p_int%c_lin_e` | `zeta` | 3 | 0.75 ⚠ |
| `p_metrics%ddqz_z_full_e` | `z_w_con_c_full` | 3 | 0.60 ⚠ |
| `p_metrics%ddqz_z_full_e` | `zeta` | 3 | 1.00 ⚠ |
| `p_patch%edges%f_e` | `z_w_con_c_full` | 3 | 0.50 ⚠ |
| `z_w_con_c_full` | `zeta` | 3 | 0.60 ⚠ |
| `p_diag%vn_ie` | `p_metrics%wgtfacq_e` | 2 | 0.29 ⚠ |
| `p_diag%vt` | `p_metrics%wgtfacq_e` | 2 | 0.22 ⚠ |
| `p_metrics%wgtfacq_e` | `p_prog%vn` | 2 | 0.33 ⚠ |
| `p_metrics%wgtfacq_e` | `z_kin_hor_e` | 2 | 0.33 ⚠ |
| `p_metrics%wgtfacq_e` | `z_vt_ie` | 2 | 0.40 ⚠ |
| `p_prog%vn` | `z_vt_ie` | 2 | 0.22 ⚠ |
| `z_kin_hor_e` | `z_vt_ie` | 2 | 0.22 ⚠ |
| `p_diag%vt` | `p_metrics%wgtfac_e` | 2 | 0.22 ⚠ |
| `p_diag%vn_ie` | `z_v_grad_w` | 2 | 0.25 ⚠ |
| `p_patch%edges%inv_primal_edge_length` | `p_patch%edges%tangent_orientation` | 2 | 1.00 |
| `z_v_grad_w` | `z_vt_ie` | 2 | 0.33 ⚠ |
| `p_diag%vn_ie` | `p_patch%edges%ft_e` | 2 | 0.25 ⚠ |
| `cfl_clipping` | `p_metrics%ddqz_z_half` | 2 | 1.00 ⚠ |
| `cfl_clipping` | `z_w_con_c` | 2 | 0.29 ⚠ |
| `p_metrics%ddqz_z_half` | `z_w_con_c` | 2 | 0.29 ⚠ |

> ⚠ = at least one array is a bridge (S+U conflict)

## 5. Identical-Membership Clusters (Jaccard = 1.0)

Arrays that appear in exactly the same set of nests.

### Cluster: nests {20, 22, 24} — ⚠ HAS BRIDGE-U

- `p_diag%ddt_vn_apc_pc` (U-only)
- `p_metrics%ddqz_z_full_e` (U-only)
- `zeta` (bridge-U)

### Cluster: nests {19} — ⚠ HAS BRIDGE-U

- `p_int%geofac_n2s` (U-only)
- `p_patch%cells%area` (U-only)
- `p_patch%cells%decomp_info%owner_mask` (U-only)

### Cluster: nests {7} — fully structured

- `p_metrics%deepatmo_gradh_ifc` (S-only)
- `p_metrics%deepatmo_invr_ifc` (S-only)
- `p_patch%edges%fn_e` (S-only)

### Cluster: nests {15, 19} — fully structured

- `cfl_clipping` (bridge-S)
- `p_metrics%ddqz_z_half` (bridge-S)

### Cluster: nests {24} — ⚠ HAS BRIDGE-U

- `p_int%geofac_grdiv` (U-only)
- `p_patch%edges%area_edge` (U-only)

### Cluster: nests {17} — fully structured

- `p_metrics%coeff1_dwdz` (S-only)
- `p_metrics%coeff2_dwdz` (S-only)

### Cluster: nests {5} — fully structured

- `p_metrics%ddxn_z_full` (S-only)
- `p_metrics%ddxt_z_full` (S-only)

### Cluster: nests {22} — ⚠ HAS BRIDGE-U

- `p_metrics%deepatmo_gradh_mc` (U-only)
- `p_metrics%deepatmo_invr_mc` (U-only)

### Cluster: nests {6} — ⚠ HAS BRIDGE-U

- `p_patch%edges%inv_dual_edge_length` (U-only)
- `z_w_v` (bridge-U)

### Cluster: nests {6, 24} — ⚠ HAS BRIDGE-U

- `p_patch%edges%inv_primal_edge_length` (U-only)
- `p_patch%edges%tangent_orientation` (U-only)

## 6. Nest Overlap (Jaccard on array working sets)

Nest pairs ranked by array-set similarity.

| Nest A | Nest B | Jaccard | |A| | |B| | |A∩B| | Shared arrays |
|-------:|-------:|:-------:|----:|----:|------:|---------------|
| 1 (b.h S) | 2 (b.h S) | 0.86 | 6 | 7 | 6 | `p_diag%vn_ie`, `p_diag%vt`, `p_metrics%wgtfacq_e`, `p_prog%vn`, `z_kin_hor_e`, `z_vt_ie` |
| 20 (v.h U) | 22 (v.h U) | 0.73 | 11 | 15 | 11 | `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_patch%edges%f_e`, `z_ekinh`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta` |
| 1 (b.h S) | 3 (v.h U) | 0.50 | 6 | 6 | 4 | `p_diag%vn_ie`, `p_diag%vt`, `p_prog%vn`, `z_kin_hor_e` |
| 8 (b.h S) | 13 (v.h S) | 0.50 | 1 | 2 | 1 | `z_w_con_c` |
| 8 (b.h S) | 14 (v.h S) | 0.50 | 1 | 2 | 1 | `z_w_con_c` |
| 8 (b.h S) | 16 (v.h S) | 0.50 | 1 | 2 | 1 | `z_w_con_c` |
| 9 (b.v S) | 25 (v S) | 0.50 | 1 | 2 | 1 | `levmask` |
| 21 (v.h S) | 23 (v.h U) | 0.50 | 3 | 6 | 3 | `p_diag%ddt_vn_cor_pc`, `p_diag%vt`, `p_patch%edges%f_e` |
| 2 (b.h S) | 3 (v.h U) | 0.44 | 7 | 6 | 4 | `p_diag%vn_ie`, `p_diag%vt`, `p_prog%vn`, `z_kin_hor_e` |
| 13 (v.h S) | 17 (v.h S) | 0.40 | 2 | 5 | 2 | `p_prog%w`, `z_w_con_c` |
| 13 (v.h S) | 14 (v.h S) | 0.33 | 2 | 2 | 1 | `z_w_con_c` |
| 13 (v.h S) | 16 (v.h S) | 0.33 | 2 | 2 | 1 | `z_w_con_c` |
| 14 (v.h S) | 16 (v.h S) | 0.33 | 2 | 2 | 1 | `z_w_con_c` |
| 15 (v.h S) | 19 (v.h U) | 0.33 | 4 | 8 | 3 | `cfl_clipping`, `p_metrics%ddqz_z_half`, `z_w_con_c` |
| 22 (v.h U) | 24 (v.h U) | 0.32 | 15 | 10 | 6 | `p_diag%ddt_vn_apc_pc`, `p_int%c_lin_e`, `p_metrics%ddqz_z_full_e`, `p_prog%vn`, `z_w_con_c_full`, `zeta` |
| 20 (v.h U) | 24 (v.h U) | 0.31 | 11 | 10 | 5 | `p_diag%ddt_vn_apc_pc`, `p_int%c_lin_e`, `p_metrics%ddqz_z_full_e`, `z_w_con_c_full`, `zeta` |
| 22 (v.h U) | 23 (v.h U) | 0.31 | 15 | 6 | 5 | `p_diag%vt`, `p_int%c_lin_e`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `z_w_con_c_full` |
| 20 (v.h U) | 23 (v.h U) | 0.31 | 11 | 6 | 4 | `p_diag%vt`, `p_int%c_lin_e`, `p_patch%edges%f_e`, `z_w_con_c_full` |
| 17 (v.h S) | 19 (v.h U) | 0.30 | 5 | 8 | 3 | `p_diag%ddt_w_adv_pc`, `p_prog%w`, `z_w_con_c` |
| 1 (b.h S) | 4 (v.h S) | 0.29 | 6 | 3 | 2 | `p_diag%vt`, `z_vt_ie` |
| 3 (v.h U) | 4 (v.h S) | 0.29 | 6 | 3 | 2 | `p_diag%vt`, `p_metrics%wgtfac_e` |
| 2 (b.h S) | 4 (v.h S) | 0.25 | 7 | 3 | 2 | `p_diag%vt`, `z_vt_ie` |
| 6 (v.h U) | 7 (v.h S) | 0.25 | 8 | 7 | 3 | `p_diag%vn_ie`, `z_v_grad_w`, `z_vt_ie` |
| 8 (b.h S) | 15 (v.h S) | 0.25 | 1 | 4 | 1 | `z_w_con_c` |
| 9 (b.v S) | 15 (v.h S) | 0.25 | 1 | 4 | 1 | `levmask` |
| 12 (v.h S) | 14 (v.h S) | 0.25 | 3 | 2 | 1 | `p_diag%w_concorr_c` |
| 13 (v.h S) | 19 (v.h U) | 0.25 | 2 | 8 | 2 | `p_prog%w`, `z_w_con_c` |
| 1 (b.h S) | 22 (v.h U) | 0.24 | 6 | 15 | 4 | `p_diag%vn_ie`, `p_diag%vt`, `p_prog%vn`, `z_kin_hor_e` |
| 3 (v.h U) | 22 (v.h U) | 0.24 | 6 | 15 | 4 | `p_diag%vn_ie`, `p_diag%vt`, `p_prog%vn`, `z_kin_hor_e` |
| 1 (b.h S) | 5 (v.h S) | 0.22 | 6 | 5 | 2 | `p_diag%vt`, `p_prog%vn` |

## 7. Producer → Consumer Dataflow

Array-level data dependencies between nests: nest A writes an array that nest B reads.

| Producer | Consumer | Dataflow arrays |
|:--------:|:--------:|-----------------|
| N1 (b.h S) | N6 (v.h U) | `p_diag%vn_ie`, `z_vt_ie` |
| N1 (b.h S) | N7 (v.h S) | `p_diag%vn_ie`, `z_vt_ie` |
| N1 (b.h S) | N10 (v.h U) | `z_kin_hor_e` |
| N1 (b.h S) | N20 (v.h U) | `p_diag%vn_ie`, `z_kin_hor_e` |
| N1 (b.h S) | N22 (v.h U) | `p_diag%vn_ie`, `z_kin_hor_e` |
| N2 (b.h S) | N6 (v.h U) | `p_diag%vn_ie`, `z_vt_ie` |
| N2 (b.h S) | N7 (v.h S) | `p_diag%vn_ie`, `z_vt_ie` |
| N2 (b.h S) | N10 (v.h U) | `z_kin_hor_e` |
| N2 (b.h S) | N20 (v.h U) | `p_diag%vn_ie`, `z_kin_hor_e` |
| N2 (b.h S) | N22 (v.h U) | `p_diag%vn_ie`, `z_kin_hor_e` |
| N3 (v.h U) | N1 (b.h S) | `p_diag%vt` |
| N3 (v.h U) | N2 (b.h S) | `p_diag%vt` |
| N3 (v.h U) | N4 (v.h S) | `p_diag%vt` |
| N3 (v.h U) | N5 (v.h S) | `p_diag%vt` |
| N3 (v.h U) | N6 (v.h U) | `p_diag%vn_ie` |
| N3 (v.h U) | N7 (v.h S) | `p_diag%vn_ie` |
| N3 (v.h U) | N10 (v.h U) | `z_kin_hor_e` |
| N3 (v.h U) | N20 (v.h U) | `p_diag%vn_ie`, `p_diag%vt`, `z_kin_hor_e` |
| N3 (v.h U) | N21 (v.h S) | `p_diag%vt` |
| N3 (v.h U) | N22 (v.h U) | `p_diag%vn_ie`, `p_diag%vt`, `z_kin_hor_e` |
| N3 (v.h U) | N23 (v.h U) | `p_diag%vt` |
| N4 (v.h S) | N6 (v.h U) | `z_vt_ie` |
| N4 (v.h S) | N7 (v.h S) | `z_vt_ie` |
| N5 (v.h S) | N11 (v.h U) | `z_w_concorr_me` |
| N6 (v.h U) | N7 (v.h S) | `z_v_grad_w` |
| N6 (v.h U) | N18 (v.h U) | `z_v_grad_w` |
| N7 (v.h S) | N18 (v.h U) | `z_v_grad_w` |
| N8 (b.h S) | N14 (v.h S) | `z_w_con_c` |
| N8 (b.h S) | N15 (v.h S) | `z_w_con_c` |
| N8 (b.h S) | N16 (v.h S) | `z_w_con_c` |
| N8 (b.h S) | N17 (v.h S) | `z_w_con_c` |
| N8 (b.h S) | N19 (v.h U) | `z_w_con_c` |
| N9 (b.v S) | N25 (v S) | `levmask` |
| N10 (v.h U) | N20 (v.h U) | `z_ekinh` |
| N10 (v.h U) | N22 (v.h U) | `z_ekinh` |
| N11 (v.h U) | N12 (v.h S) | `z_w_concorr_mc` |
| N12 (v.h S) | N14 (v.h S) | `p_diag%w_concorr_c` |
| N13 (v.h S) | N14 (v.h S) | `z_w_con_c` |
| N13 (v.h S) | N15 (v.h S) | `z_w_con_c` |
| N13 (v.h S) | N16 (v.h S) | `z_w_con_c` |
| N13 (v.h S) | N17 (v.h S) | `z_w_con_c` |
| N13 (v.h S) | N19 (v.h U) | `z_w_con_c` |
| N14 (v.h S) | N15 (v.h S) | `z_w_con_c` |
| N14 (v.h S) | N16 (v.h S) | `z_w_con_c` |
| N14 (v.h S) | N17 (v.h S) | `z_w_con_c` |
| N14 (v.h S) | N19 (v.h U) | `z_w_con_c` |
| N15 (v.h S) | N14 (v.h S) | `z_w_con_c` |
| N15 (v.h S) | N16 (v.h S) | `z_w_con_c` |
| N15 (v.h S) | N17 (v.h S) | `z_w_con_c` |
| N15 (v.h S) | N19 (v.h U) | `z_w_con_c` |
| N15 (v.h S) | N25 (v S) | `levmask` |
| N16 (v.h S) | N20 (v.h U) | `z_w_con_c_full` |
| N16 (v.h S) | N22 (v.h U) | `z_w_con_c_full` |
| N16 (v.h S) | N23 (v.h U) | `z_w_con_c_full` |
| N16 (v.h S) | N24 (v.h U) | `z_w_con_c_full` |
| N17 (v.h S) | N18 (v.h U) | `p_diag%ddt_w_adv_pc` |
| N17 (v.h S) | N19 (v.h U) | `p_diag%ddt_w_adv_pc` |
| N18 (v.h U) | N19 (v.h U) | `p_diag%ddt_w_adv_pc` |
| N19 (v.h U) | N18 (v.h U) | `p_diag%ddt_w_adv_pc` |
| N20 (v.h U) | N24 (v.h U) | `p_diag%ddt_vn_apc_pc` |
| N22 (v.h U) | N24 (v.h U) | `p_diag%ddt_vn_apc_pc` |

## 8. Bridge-U Deep Dive

These arrays are accessed with direct structured subscripts in some nests and via indirection in others.

### `p_prog%vn`

- **Structured nests:** 1, 2, 5, 22 (4 nests)
- **Unstructured nests:** 3, 24 (2 nests)
- **Co-accessed in U-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_int%geofac_grdiv`, `p_int%rbf_vec_coeff_e`, `p_metrics%ddqz_z_full_e`, `p_metrics%wgtfac_e`, `p_patch%edges%area_edge`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta`
- **Co-accessed in S-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vn_ie_ubc`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%ddxn_z_full`, `p_metrics%ddxt_z_full`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_metrics%wgtfacq_e`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `z_ekinh`, `z_kin_hor_e`, `z_vt_ie`, `z_w_con_c_full`, `z_w_concorr_me`, `zeta`
- **Shared co-access (S∩U):** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%ddqz_z_full_e`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta`

### `p_prog%w`

- **Structured nests:** 13, 17 (2 nests)
- **Unstructured nests:** 6, 19 (2 nests)
- **Co-accessed in U-nests:** `cfl_clipping`, `p_diag%ddt_w_adv_pc`, `p_diag%vn_ie`, `p_int%geofac_n2s`, `p_metrics%ddqz_z_half`, `p_patch%cells%area`, `p_patch%cells%decomp_info%owner_mask`, `p_patch%edges%inv_dual_edge_length`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `z_v_grad_w`, `z_vt_ie`, `z_w_con_c`, `z_w_v`
- **Co-accessed in S-nests:** `p_diag%ddt_w_adv_pc`, `p_metrics%coeff1_dwdz`, `p_metrics%coeff2_dwdz`, `z_w_con_c`
- **Shared co-access (S∩U):** `p_diag%ddt_w_adv_pc`, `z_w_con_c`

### `z_ekinh`

- **Structured nests:** 10 (1 nests)
- **Unstructured nests:** 20, 22 (2 nests)
- **Co-accessed in U-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `p_prog%vn`, `z_kin_hor_e`, `z_w_con_c_full`, `zeta`
- **Co-accessed in S-nests:** `p_int%e_bln_c_s`, `z_kin_hor_e`
- **Shared co-access (S∩U):** `z_kin_hor_e`

### `z_kin_hor_e`

- **Structured nests:** 1, 2, 3, 20, 22 (5 nests)
- **Unstructured nests:** 10 (1 nests)
- **Co-accessed in U-nests:** `p_int%e_bln_c_s`, `z_ekinh`
- **Co-accessed in S-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vn_ie_ubc`, `p_diag%vt`, `p_int%c_lin_e`, `p_int%rbf_vec_coeff_e`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_metrics%wgtfac_e`, `p_metrics%wgtfacq_e`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `p_prog%vn`, `z_ekinh`, `z_vt_ie`, `z_w_con_c_full`, `zeta`
- **Shared co-access (S∩U):** `z_ekinh`

### `z_v_grad_w`

- **Structured nests:** 6, 7 (2 nests)
- **Unstructured nests:** 18 (1 nests)
- **Co-accessed in U-nests:** `p_diag%ddt_w_adv_pc`, `p_int%e_bln_c_s`
- **Co-accessed in S-nests:** `p_diag%vn_ie`, `p_metrics%deepatmo_gradh_ifc`, `p_metrics%deepatmo_invr_ifc`, `p_patch%edges%fn_e`, `p_patch%edges%ft_e`, `p_patch%edges%inv_dual_edge_length`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%w`, `z_vt_ie`, `z_w_v`

### `z_w_con_c_full`

- **Structured nests:** 16 (1 nests)
- **Unstructured nests:** 20, 22, 23, 24 (4 nests)
- **Co-accessed in U-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%ddt_vn_cor_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_int%geofac_grdiv`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_patch%edges%area_edge`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%vn`, `z_ekinh`, `z_kin_hor_e`, `zeta`
- **Co-accessed in S-nests:** `z_w_con_c`

### `z_w_concorr_me`

- **Structured nests:** 5 (1 nests)
- **Unstructured nests:** 11 (1 nests)
- **Co-accessed in U-nests:** `p_int%e_bln_c_s`, `z_w_concorr_mc`
- **Co-accessed in S-nests:** `p_diag%vt`, `p_metrics%ddxn_z_full`, `p_metrics%ddxt_z_full`, `p_prog%vn`

### `z_w_v`

- **Structured nests:**  (0 nests)
- **Unstructured nests:** 6 (1 nests)
- **Co-accessed in U-nests:** `p_diag%vn_ie`, `p_patch%edges%inv_dual_edge_length`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%w`, `z_v_grad_w`, `z_vt_ie`
- **Co-accessed in S-nests:** 

### `zeta`

- **Structured nests:**  (0 nests)
- **Unstructured nests:** 20, 22, 24 (3 nests)
- **Co-accessed in U-nests:** `p_diag%ddt_vn_apc_pc`, `p_diag%vn_ie`, `p_diag%vt`, `p_int%c_lin_e`, `p_int%geofac_grdiv`, `p_metrics%coeff_gradekin`, `p_metrics%ddqz_z_full_e`, `p_metrics%deepatmo_gradh_mc`, `p_metrics%deepatmo_invr_mc`, `p_patch%edges%area_edge`, `p_patch%edges%f_e`, `p_patch%edges%ft_e`, `p_patch%edges%inv_primal_edge_length`, `p_patch%edges%tangent_orientation`, `p_prog%vn`, `z_ekinh`, `z_kin_hor_e`, `z_w_con_c_full`
- **Co-accessed in S-nests:** 

## 9. Layout Groups (collapsed, block-stripped)

Arrays grouped by **(dimension signature, worst-case S/U per role)**. Block dimension stripped. Within a group, all arrays have the same shape and the same structured/unstructured classification per dimension.

This collapses the fine-grained "exact access pattern" groups into a small number of layout-equivalent classes.

**7 layout groups** for 50 arrays.

### STRUCTURED — (h, v) h:S v:S — FULLY STRUCTURED (15 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `cfl_clipping` | bridge-S | 2 |
| `p_diag%vn_ie` | bridge-S | 7 |
| `p_diag%vt` | bridge-S | 9 |
| `p_diag%w_concorr_c` | S-only | 2 |
| `p_metrics%coeff1_dwdz` | S-only | 1 |
| `p_metrics%coeff2_dwdz` | S-only | 1 |
| `p_metrics%ddqz_z_full_e` | U-only | 3 |
| `p_metrics%ddqz_z_half` | bridge-S | 2 |
| `p_metrics%ddxn_z_full` | S-only | 1 |
| `p_metrics%ddxt_z_full` | S-only | 1 |
| `p_metrics%wgtfac_c` | S-only | 1 |
| `p_metrics%wgtfac_e` | bridge-S | 2 |
| `z_vt_ie` | bridge-S | 5 |
| `z_w_con_c` | bridge-S | 7 |
| `z_w_concorr_mc` | bridge-S | 2 |

### STRUCTURED — (h) h:S — FULLY STRUCTURED (9 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `p_patch%cells%area` | U-only | 1 |
| `p_patch%cells%decomp_info%owner_mask` | U-only | 1 |
| `p_patch%edges%area_edge` | U-only | 1 |
| `p_patch%edges%f_e` | bridge-S | 4 |
| `p_patch%edges%fn_e` | S-only | 1 |
| `p_patch%edges%ft_e` | bridge-S | 3 |
| `p_patch%edges%inv_dual_edge_length` | U-only | 1 |
| `p_patch%edges%inv_primal_edge_length` | U-only | 2 |
| `p_patch%edges%tangent_orientation` | U-only | 2 |

### ⚠ UNSTRUCTURED — (h, v, extra) h:U v:S extra:C — HAS UNSTRUCTURED (9 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `p_prog%vn` | bridge-U | 6 |
| `p_prog%w` | bridge-U | 4 |
| `z_ekinh` | bridge-U | 3 |
| `z_kin_hor_e` | bridge-U | 6 |
| `z_v_grad_w` | bridge-U | 3 |
| `z_w_con_c_full` | bridge-U | 5 |
| `z_w_concorr_me` | bridge-U | 2 |
| `z_w_v` | bridge-U | 1 |
| `zeta` | bridge-U | 3 |

### STRUCTURED — (h, extra) h:S extra:C — FULLY STRUCTURED (8 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `p_diag%vn_ie_ubc` | S-only | 1 |
| `p_int%c_lin_e` | U-only | 4 |
| `p_int%e_bln_c_s` | U-only | 3 |
| `p_int%geofac_grdiv` | U-only | 1 |
| `p_int%geofac_n2s` | U-only | 1 |
| `p_int%rbf_vec_coeff_e` | U-only | 1 |
| `p_metrics%coeff_gradekin` | U-only | 2 |
| `p_metrics%wgtfacq_e` | S-only | 2 |

### STRUCTURED — (v) v:S — FULLY STRUCTURED (5 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `levelmask` | S-only | 1 |
| `p_metrics%deepatmo_gradh_ifc` | S-only | 1 |
| `p_metrics%deepatmo_gradh_mc` | U-only | 1 |
| `p_metrics%deepatmo_invr_ifc` | S-only | 1 |
| `p_metrics%deepatmo_invr_mc` | U-only | 1 |

### STRUCTURED — (h, v, extra) h:S v:S extra:C — FULLY STRUCTURED (3 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `p_diag%ddt_vn_apc_pc` | U-only | 3 |
| `p_diag%ddt_vn_cor_pc` | bridge-S | 2 |
| `p_diag%ddt_w_adv_pc` | bridge-S | 3 |

### STRUCTURED — (v, extra) v:S extra:C — FULLY STRUCTURED (1 arrays)

| Array | Fine-grained class | Nests |
|-------|--------------------|------:|
| `levmask` | S-only | 3 |

### Summary

| # | Dims | S/U | Tag | Arrays |
|--:|------|-----|-----|-------:|
| 1 | (h, v) | h:S v:S | S | 15 |
| 2 | (h) | h:S | S | 9 |
| 3 | (h, v, extra) | h:U v:S extra:C | U | 9 |
| 4 | (h, extra) | h:S extra:C | S | 8 |
| 5 | (v) | v:S | S | 5 |
| 6 | (h, v, extra) | h:S v:S extra:C | S | 3 |
| 7 | (v, extra) | v:S extra:C | S | 1 |

## 10. Layout Conclusions

**SoA → AoS/AoSoA conversion is not indicated.** The widest loop nest references 15 unique arrays. Since no nest exceeds the typical register-pressure threshold of ~16 concurrent streams, SoA (the current Fortran column-major layout) is not bandwidth-limited by TLB or prefetcher capacity.

**Co-access analysis:** only 16 array pair(s) have Jaccard = 1.0 (always accessed together):
- `cfl_clipping` and `p_metrics%ddqz_z_half`
- `p_diag%ddt_vn_apc_pc` and `p_metrics%ddqz_z_full_e`
- `p_diag%ddt_vn_apc_pc` and `zeta`
- `p_int%geofac_grdiv` and `p_patch%edges%area_edge`
- `p_int%geofac_n2s` and `p_patch%cells%area`
- `p_int%geofac_n2s` and `p_patch%cells%decomp_info%owner_mask`
- `p_metrics%coeff1_dwdz` and `p_metrics%coeff2_dwdz`
- `p_metrics%ddqz_z_full_e` and `zeta`
- `p_metrics%ddxn_z_full` and `p_metrics%ddxt_z_full`
- `p_metrics%deepatmo_gradh_ifc` and `p_metrics%deepatmo_invr_ifc`
- `p_metrics%deepatmo_gradh_ifc` and `p_patch%edges%fn_e`
- `p_metrics%deepatmo_gradh_mc` and `p_metrics%deepatmo_invr_mc`
- `p_metrics%deepatmo_invr_ifc` and `p_patch%edges%fn_e`
- `p_patch%cells%area` and `p_patch%cells%decomp_info%owner_mask`
- `p_patch%edges%inv_dual_edge_length` and `z_w_v`
- `p_patch%edges%inv_primal_edge_length` and `p_patch%edges%tangent_orientation`

These are the only candidates where AoS interleaving could improve spatial locality. All other pairs have Jaccard < 1.0, meaning they appear in different subsets of nests — interleaving them would waste bandwidth in nests that use only one member.

**Blocking along the vertical dimension (nlev) is not beneficial.** The vertical dimension is small and constant (typically nlev = 90), so the full vertical column already fits in cache. Combined with indirect (unstructured) access patterns on the horizontal dimension, vertical blocking would add overhead without reducing cache misses.

