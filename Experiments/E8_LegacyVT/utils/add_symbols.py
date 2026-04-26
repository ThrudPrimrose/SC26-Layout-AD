import dace

_FORCE_USE_STATE = "sym_force_use"

_SYMLIST = [
    "__f2dace_A_z_kin_hor_e_d_0_s", "__f2dace_A_z_kin_hor_e_d_1_s",
    "__f2dace_A_z_kin_hor_e_d_2_s", "__f2dace_A_z_vt_ie_d_0_s",
    "__f2dace_A_z_vt_ie_d_1_s",     "__f2dace_A_z_vt_ie_d_2_s",
    "__f2dace_A_z_w_concorr_me_d_0_s", "__f2dace_A_z_w_concorr_me_d_1_s",
    "__f2dace_A_z_w_concorr_me_d_2_s", "__f2dace_OA_z_kin_hor_e_d_0_s",
    "__f2dace_OA_z_kin_hor_e_d_1_s", "__f2dace_OA_z_kin_hor_e_d_2_s",
    "__f2dace_OA_z_vt_ie_d_0_s",    "__f2dace_OA_z_vt_ie_d_1_s",
    "__f2dace_OA_z_vt_ie_d_2_s",    "__f2dace_OA_z_w_concorr_me_d_0_s",
    "__f2dace_OA_z_w_concorr_me_d_1_s", "__f2dace_OA_z_w_concorr_me_d_2_s",
    "dt_linintp_ubc", "dtime", "istep", "ldeepatmo", "lvn_only", "ntnd",
]


def add_symbols(sdfg: dace.SDFG):
    if sdfg.start_block is not None and sdfg.start_block.label == _FORCE_USE_STATE:
        return

    new_start = sdfg.add_state_before(sdfg.start_block, _FORCE_USE_STATE, True)
    sname, _ = sdfg.add_scalar(
        "dummy_symbol_sum", dtype=dace.float64,
        transient=True, storage=dace.StorageType.Register,
    )
    inputs = {sym for sym in _SYMLIST if sym in sdfg.arrays}
    tstr = (
        "_out = "
        + " + ".join(f"_in_{inp}" for inp in inputs)
        + " + "
        + " + ".join(sym for sym in _SYMLIST if sym not in inputs)
    )
    new_tasklet = new_start.add_tasklet(
        _FORCE_USE_STATE, {"_in_" + s for s in inputs}, {"_out"}, tstr,
        side_effects=True,
    )
    for sym in _SYMLIST:
        if sym in sdfg.arrays:
            an = new_start.add_access(sym)
            new_start.add_edge(an, None, new_tasklet, "_in_" + sym,
                               dace.Memlet(f"{sym}[0]"))
        elif sym not in sdfg.symbols:
            sdfg.add_symbol(sym, dace.int32, False)
    new_start.add_edge(new_tasklet, "_out", new_start.add_access(sname), None,
                       dace.Memlet(f"{sname}[0]"))
