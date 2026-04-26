/* DaCe AUTO-GENERATED FILE. DO NOT MODIFY */
#include <dace/dace.h>
#include "../../include/hash.h"

struct velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t {

};

void __program_velocity_no_nproma_if_prop_lvn_only_0_istep_1_internal(velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t*__state, int * __restrict__ __CG_global_data__m_nflatlev, int * __restrict__ __CG_global_data__m_nrdmax, double * __restrict__ __CG_p_diag__m_ddt_vn_apc_pc, double * __restrict__ __CG_p_diag__m_ddt_vn_cor_pc, double * __restrict__ __CG_p_diag__m_ddt_w_adv_pc, double * __restrict__ __CG_p_diag__m_max_vcfl_dyn, double * __restrict__ __CG_p_diag__m_vn_ie, double * __restrict__ __CG_p_diag__m_vn_ie_ubc, double * __restrict__ __CG_p_diag__m_vt, double * __restrict__ __CG_p_diag__m_w_concorr_c, double * __restrict__ __CG_p_int__m_c_lin_e, double * __restrict__ __CG_p_int__m_cells_aw_verts, double * __restrict__ __CG_p_int__m_e_bln_c_s, double * __restrict__ __CG_p_int__m_geofac_grdiv, double * __restrict__ __CG_p_int__m_geofac_n2s, double * __restrict__ __CG_p_int__m_geofac_rot, double * __restrict__ __CG_p_int__m_rbf_vec_coeff_e, double * __restrict__ __CG_p_metrics__m_coeff1_dwdz, double * __restrict__ __CG_p_metrics__m_coeff2_dwdz, double * __restrict__ __CG_p_metrics__m_coeff_gradekin, double * __restrict__ __CG_p_metrics__m_ddqz_z_full_e, double * __restrict__ __CG_p_metrics__m_ddqz_z_half, double * __restrict__ __CG_p_metrics__m_ddxn_z_full, double * __restrict__ __CG_p_metrics__m_ddxt_z_full, double * __restrict__ __CG_p_metrics__m_deepatmo_gradh_ifc, double * __restrict__ __CG_p_metrics__m_deepatmo_gradh_mc, double * __restrict__ __CG_p_metrics__m_deepatmo_invr_ifc, double * __restrict__ __CG_p_metrics__m_deepatmo_invr_mc, double * __restrict__ __CG_p_metrics__m_wgtfac_c, double * __restrict__ __CG_p_metrics__m_wgtfac_e, double * __restrict__ __CG_p_metrics__m_wgtfacq_e, int * __restrict__ __CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask, double * __restrict__ __CG_p_patch__CG_cells__m_area, int * __restrict__ __CG_p_patch__CG_cells__m_edge_blk, int * __restrict__ __CG_p_patch__CG_cells__m_edge_idx, int * __restrict__ __CG_p_patch__CG_cells__m_end_block, int * __restrict__ __CG_p_patch__CG_cells__m_end_index, int * __restrict__ __CG_p_patch__CG_cells__m_neighbor_blk, int * __restrict__ __CG_p_patch__CG_cells__m_neighbor_idx, int * __restrict__ __CG_p_patch__CG_cells__m_start_block, int * __restrict__ __CG_p_patch__CG_cells__m_start_index, double * __restrict__ __CG_p_patch__CG_edges__m_area_edge, int * __restrict__ __CG_p_patch__CG_edges__m_cell_blk, int * __restrict__ __CG_p_patch__CG_edges__m_cell_idx, int * __restrict__ __CG_p_patch__CG_edges__m_end_block, int * __restrict__ __CG_p_patch__CG_edges__m_end_index, double * __restrict__ __CG_p_patch__CG_edges__m_f_e, double * __restrict__ __CG_p_patch__CG_edges__m_fn_e, double * __restrict__ __CG_p_patch__CG_edges__m_ft_e, double * __restrict__ __CG_p_patch__CG_edges__m_inv_dual_edge_length, double * __restrict__ __CG_p_patch__CG_edges__m_inv_primal_edge_length, int * __restrict__ __CG_p_patch__CG_edges__m_quad_blk, int * __restrict__ __CG_p_patch__CG_edges__m_quad_idx, int * __restrict__ __CG_p_patch__CG_edges__m_start_block, int * __restrict__ __CG_p_patch__CG_edges__m_start_index, double * __restrict__ __CG_p_patch__CG_edges__m_tangent_orientation, int * __restrict__ __CG_p_patch__CG_edges__m_vertex_blk, int * __restrict__ __CG_p_patch__CG_edges__m_vertex_idx, int * __restrict__ __CG_p_patch__CG_verts__m_cell_blk, int * __restrict__ __CG_p_patch__CG_verts__m_cell_idx, int * __restrict__ __CG_p_patch__CG_verts__m_edge_blk, int * __restrict__ __CG_p_patch__CG_verts__m_edge_idx, int * __restrict__ __CG_p_patch__CG_verts__m_end_block, int * __restrict__ __CG_p_patch__CG_verts__m_end_index, int * __restrict__ __CG_p_patch__CG_verts__m_start_block, int * __restrict__ __CG_p_patch__CG_verts__m_start_index, double * __restrict__ __CG_p_prog__m_vn, double * __restrict__ __CG_p_prog__m_w, double * __restrict__ z_kin_hor_e, double * __restrict__ z_vt_ie, double * __restrict__ z_w_concorr_me, int A_z_kin_hor_e_d_0, int A_z_kin_hor_e_d_1, int A_z_kin_hor_e_d_2, int A_z_vt_ie_d_0, int A_z_vt_ie_d_1, int A_z_vt_ie_d_2, int A_z_w_concorr_me_d_0, int A_z_w_concorr_me_d_1, int A_z_w_concorr_me_d_2, int OA_z_kin_hor_e_d_0, int OA_z_kin_hor_e_d_1, int OA_z_kin_hor_e_d_2, int OA_z_vt_ie_d_0, int OA_z_vt_ie_d_1, int OA_z_vt_ie_d_2, int OA_z_w_concorr_me_d_0, int OA_z_w_concorr_me_d_1, int OA_z_w_concorr_me_d_2, int SA_area_d_0_cells_p_patch_2, int SA_area_d_1_cells_p_patch_2, int SA_cell_blk_d_1_edges_p_patch_4, int SA_cell_blk_d_1_verts_p_patch_5, int SA_cell_blk_d_2_edges_p_patch_4, int SA_cell_blk_d_2_verts_p_patch_5, int SA_cell_idx_d_1_edges_p_patch_4, int SA_cell_idx_d_1_verts_p_patch_5, int SA_cell_idx_d_2_edges_p_patch_4, int SA_cell_idx_d_2_verts_p_patch_5, int SA_edge_blk_d_2_cells_p_patch_2, int SA_edge_blk_d_2_verts_p_patch_5, int SA_edge_idx_d_2_cells_p_patch_2, int SA_edge_idx_d_2_verts_p_patch_5, int SA_end_block_d_0_cells_p_patch_2, int SA_end_block_d_0_edges_p_patch_4, int SA_end_block_d_0_verts_p_patch_5, int SA_end_index_d_0_cells_p_patch_2, int SA_end_index_d_0_edges_p_patch_4, int SA_end_index_d_0_verts_p_patch_5, int SA_neighbor_blk_d_2_cells_p_patch_2, int SA_neighbor_idx_d_2_cells_p_patch_2, int SA_quad_blk_d_2_edges_p_patch_4, int SA_quad_idx_d_2_edges_p_patch_4, int SA_start_block_d_0_cells_p_patch_2, int SA_start_block_d_0_edges_p_patch_4, int SA_start_block_d_0_verts_p_patch_5, int SA_start_index_d_0_cells_p_patch_2, int SA_start_index_d_0_edges_p_patch_4, int SA_start_index_d_0_verts_p_patch_5, int SA_vertex_blk_d_2_edges_p_patch_4, int SA_vertex_idx_d_2_edges_p_patch_4, int SOA_area_d_0_cells_p_patch_2, int SOA_area_d_1_cells_p_patch_2, int SOA_end_block_d_0_cells_p_patch_2, int SOA_end_block_d_0_edges_p_patch_4, int SOA_end_block_d_0_verts_p_patch_5, int SOA_end_index_d_0_cells_p_patch_2, int SOA_end_index_d_0_edges_p_patch_4, int SOA_end_index_d_0_verts_p_patch_5, int SOA_start_block_d_0_cells_p_patch_2, int SOA_start_block_d_0_edges_p_patch_4, int SOA_start_block_d_0_verts_p_patch_5, int SOA_start_index_d_0_cells_p_patch_2, int SOA_start_index_d_0_edges_p_patch_4, int SOA_start_index_d_0_verts_p_patch_5, int __CG_global_data__m_i_am_accel_node, int __CG_global_data__m_lextra_diffu, int __CG_global_data__m_lvert_nest, int __CG_global_data__m_nproma, int __CG_global_data__m_timer_intp, int __CG_global_data__m_timer_solve_nh_veltend, int __CG_global_data__m_timers_level, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SA_vn_ie_d_0, int __CG_p_diag__m_SA_vn_ie_d_1, int __CG_p_diag__m_SA_vn_ie_d_2, int __CG_p_diag__m_SA_vn_ie_ubc_d_0, int __CG_p_diag__m_SA_vn_ie_ubc_d_1, int __CG_p_diag__m_SA_vn_ie_ubc_d_2, int __CG_p_diag__m_SA_vt_d_0, int __CG_p_diag__m_SA_vt_d_1, int __CG_p_diag__m_SA_vt_d_2, int __CG_p_diag__m_SA_w_concorr_c_d_0, int __CG_p_diag__m_SA_w_concorr_c_d_1, int __CG_p_diag__m_SA_w_concorr_c_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SOA_vn_ie_d_0, int __CG_p_diag__m_SOA_vn_ie_d_1, int __CG_p_diag__m_SOA_vn_ie_d_2, int __CG_p_diag__m_SOA_vn_ie_ubc_d_0, int __CG_p_diag__m_SOA_vn_ie_ubc_d_1, int __CG_p_diag__m_SOA_vn_ie_ubc_d_2, int __CG_p_diag__m_SOA_vt_d_0, int __CG_p_diag__m_SOA_vt_d_1, int __CG_p_diag__m_SOA_vt_d_2, int __CG_p_diag__m_SOA_w_concorr_c_d_0, int __CG_p_diag__m_SOA_w_concorr_c_d_1, int __CG_p_diag__m_SOA_w_concorr_c_d_2, int __CG_p_diag__m_ddt_vn_adv_is_associated, int __CG_p_diag__m_ddt_vn_cor_is_associated, int __CG_p_int__m_SA_c_lin_e_d_0, int __CG_p_int__m_SA_c_lin_e_d_1, int __CG_p_int__m_SA_c_lin_e_d_2, int __CG_p_int__m_SA_cells_aw_verts_d_0, int __CG_p_int__m_SA_cells_aw_verts_d_1, int __CG_p_int__m_SA_cells_aw_verts_d_2, int __CG_p_int__m_SA_e_bln_c_s_d_0, int __CG_p_int__m_SA_e_bln_c_s_d_1, int __CG_p_int__m_SA_e_bln_c_s_d_2, int __CG_p_int__m_SA_geofac_grdiv_d_0, int __CG_p_int__m_SA_geofac_grdiv_d_1, int __CG_p_int__m_SA_geofac_grdiv_d_2, int __CG_p_int__m_SA_geofac_n2s_d_0, int __CG_p_int__m_SA_geofac_n2s_d_1, int __CG_p_int__m_SA_geofac_n2s_d_2, int __CG_p_int__m_SA_geofac_rot_d_0, int __CG_p_int__m_SA_geofac_rot_d_1, int __CG_p_int__m_SA_geofac_rot_d_2, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_2, int __CG_p_int__m_SOA_c_lin_e_d_0, int __CG_p_int__m_SOA_c_lin_e_d_1, int __CG_p_int__m_SOA_c_lin_e_d_2, int __CG_p_int__m_SOA_cells_aw_verts_d_0, int __CG_p_int__m_SOA_cells_aw_verts_d_1, int __CG_p_int__m_SOA_cells_aw_verts_d_2, int __CG_p_int__m_SOA_e_bln_c_s_d_0, int __CG_p_int__m_SOA_e_bln_c_s_d_1, int __CG_p_int__m_SOA_e_bln_c_s_d_2, int __CG_p_int__m_SOA_geofac_grdiv_d_0, int __CG_p_int__m_SOA_geofac_grdiv_d_1, int __CG_p_int__m_SOA_geofac_grdiv_d_2, int __CG_p_int__m_SOA_geofac_n2s_d_0, int __CG_p_int__m_SOA_geofac_n2s_d_1, int __CG_p_int__m_SOA_geofac_n2s_d_2, int __CG_p_int__m_SOA_geofac_rot_d_0, int __CG_p_int__m_SOA_geofac_rot_d_1, int __CG_p_int__m_SOA_geofac_rot_d_2, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2, int __CG_p_metrics__m_SA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SA_coeff_gradekin_d_0, int __CG_p_metrics__m_SA_coeff_gradekin_d_1, int __CG_p_metrics__m_SA_coeff_gradekin_d_2, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SA_ddqz_z_half_d_0, int __CG_p_metrics__m_SA_ddqz_z_half_d_1, int __CG_p_metrics__m_SA_ddqz_z_half_d_2, int __CG_p_metrics__m_SA_ddxn_z_full_d_0, int __CG_p_metrics__m_SA_ddxn_z_full_d_1, int __CG_p_metrics__m_SA_ddxn_z_full_d_2, int __CG_p_metrics__m_SA_ddxt_z_full_d_0, int __CG_p_metrics__m_SA_ddxt_z_full_d_1, int __CG_p_metrics__m_SA_ddxt_z_full_d_2, int __CG_p_metrics__m_SA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_1, int __CG_p_metrics__m_SA_wgtfac_c_d_2, int __CG_p_metrics__m_SA_wgtfac_e_d_0, int __CG_p_metrics__m_SA_wgtfac_e_d_1, int __CG_p_metrics__m_SA_wgtfac_e_d_2, int __CG_p_metrics__m_SA_wgtfacq_e_d_0, int __CG_p_metrics__m_SA_wgtfacq_e_d_1, int __CG_p_metrics__m_SA_wgtfacq_e_d_2, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff_gradekin_d_0, int __CG_p_metrics__m_SOA_coeff_gradekin_d_1, int __CG_p_metrics__m_SOA_coeff_gradekin_d_2, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SOA_ddqz_z_half_d_0, int __CG_p_metrics__m_SOA_ddqz_z_half_d_1, int __CG_p_metrics__m_SOA_ddqz_z_half_d_2, int __CG_p_metrics__m_SOA_ddxn_z_full_d_0, int __CG_p_metrics__m_SOA_ddxn_z_full_d_1, int __CG_p_metrics__m_SOA_ddxn_z_full_d_2, int __CG_p_metrics__m_SOA_ddxt_z_full_d_0, int __CG_p_metrics__m_SOA_ddxt_z_full_d_1, int __CG_p_metrics__m_SOA_ddxt_z_full_d_2, int __CG_p_metrics__m_SOA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_1, int __CG_p_metrics__m_SOA_wgtfac_c_d_2, int __CG_p_metrics__m_SOA_wgtfac_e_d_0, int __CG_p_metrics__m_SOA_wgtfac_e_d_1, int __CG_p_metrics__m_SOA_wgtfac_e_d_2, int __CG_p_metrics__m_SOA_wgtfacq_e_d_0, int __CG_p_metrics__m_SOA_wgtfacq_e_d_1, int __CG_p_metrics__m_SOA_wgtfacq_e_d_2, int __CG_p_patch__m_id, int __CG_p_patch__m_nblks_c, int __CG_p_patch__m_nblks_e, int __CG_p_patch__m_nblks_v, int __CG_p_patch__m_nlev, int __CG_p_patch__m_nlevp1, int __CG_p_patch__m_nshift, int __CG_p_prog__m_SA_vn_d_0, int __CG_p_prog__m_SA_vn_d_1, int __CG_p_prog__m_SA_vn_d_2, int __CG_p_prog__m_SA_w_d_0, int __CG_p_prog__m_SA_w_d_1, int __CG_p_prog__m_SA_w_d_2, int __CG_p_prog__m_SOA_vn_d_0, int __CG_p_prog__m_SOA_vn_d_1, int __CG_p_prog__m_SOA_vn_d_2, int __CG_p_prog__m_SOA_w_d_0, int __CG_p_prog__m_SOA_w_d_1, int __CG_p_prog__m_SOA_w_d_2, double dt_linintp_ubc, double dtime, int ntnd)
{
    double *z_w_concorr_mc;
    z_w_concorr_mc = new double DACE_ALIGN(64)[((__CG_global_data__m_nproma * (__CG_p_patch__m_nlev - 1)) + __CG_global_data__m_nproma)];
    double *z_w_con_c;
    z_w_con_c = new double DACE_ALIGN(64)[((__CG_global_data__m_nproma * (__CG_p_patch__m_nlevp1 - 1)) + __CG_global_data__m_nproma)];
    double *z_w_con_c_full;
    z_w_con_c_full = new double DACE_ALIGN(64)[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (__CG_p_patch__m_nblks_c - 1)) + (__CG_global_data__m_nproma * (__CG_p_patch__m_nlev - 1))) + __CG_global_data__m_nproma)];
    double *z_v_grad_w;
    z_v_grad_w = new double DACE_ALIGN(64)[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (__CG_p_patch__m_nblks_e - 1)) + (__CG_global_data__m_nproma * (__CG_p_patch__m_nlev - 1))) + __CG_global_data__m_nproma)];
    double *z_w_v;
    z_w_v = new double DACE_ALIGN(64)[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlevp1) * (__CG_p_patch__m_nblks_v - 1)) + (__CG_global_data__m_nproma * (__CG_p_patch__m_nlevp1 - 1))) + __CG_global_data__m_nproma)];
    double *zeta;
    zeta = new double DACE_ALIGN(64)[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (__CG_p_patch__m_nblks_v - 1)) + (__CG_global_data__m_nproma * (__CG_p_patch__m_nlev - 1))) + __CG_global_data__m_nproma)];
    double *z_ekinh;
    z_ekinh = new double DACE_ALIGN(64)[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (__CG_p_patch__m_nblks_c - 1)) + (__CG_global_data__m_nproma * (__CG_p_patch__m_nlev - 1))) + __CG_global_data__m_nproma)];
    int nlev_var_154;
    int nlevp1_var_155;
    int jg;
    double cfl_w_limit;
    double vcfl;
    double maxvcfl;
    double *vcflmax;
    vcflmax = new double DACE_ALIGN(64)[__CG_p_patch__m_nblks_c];
    double w_con_e;
    double scalfac_exdiff;
    double difcoef;
    int nrdmax_jg;
    int nflatlev_jg;
    int *levmask;
    levmask = new int DACE_ALIGN(64)[((__CG_p_patch__m_nblks_c * (__CG_p_patch__m_nlev - 1)) + __CG_p_patch__m_nblks_c)];
    int *levelmask;
    levelmask = new int DACE_ALIGN(64)[__CG_p_patch__m_nlev];
    int *cfl_clipping;
    cfl_clipping = new int DACE_ALIGN(64)[((__CG_global_data__m_nproma * (__CG_p_patch__m_nlevp1 - 1)) + __CG_global_data__m_nproma)];
    double tmp_call_19;
    int _if_cond_9;
    int _if_cond_18;
    int _if_cond_29;
    double _if_cond_33;
    int tmp_index_162;
    int tmp_index_163;
    int i_startblk_var_142_0;
    int i_endblk_var_143_0;
    int i_startidx_in_var_144_0;
    int i_endidx_in_var_145_0;
    int i_startblk_var_120_0;
    int i_endblk_var_121_0;
    int i_startblk_var_148;
    int i_endblk_var_149;
    int i_startblk_2;
    int i_endblk_2;
    int tmp_arg_15;
    double tmp_call_20;
    int l_vert_nested;
    int _for_it_23;
    int i_startidx_in_var_81_1;
    int i_endidx_in_var_82_1;
    int tmp_arg_9;
    int tmp_arg_10;
    int i_startidx_in_var_81_0;
    int i_endidx_in_var_82_0;
    int tmp_arg_12;
    int _for_it_24;
    int _for_it_25;
    int tmp_index_452;
    int tmp_index_454;
    int tmp_index_464;
    int tmp_index_466;
    int tmp_index_476;
    int tmp_index_478;
    int _for_it_30;
    int _for_it_31;
    int _for_it_32;
    int tmp_index_536;
    int _for_it_33;
    int _for_it_34;
    int _for_it_35;
    int _for_it_36;
    int clip_count;
    int _for_it_37;
    int _for_it_38;
    int _if_cond_20;
    double _if_cond_21;
    double _if_cond_22;
    int _for_it_39;
    int _for_it_40;
    int _for_it_41;
    int _for_it_42;
    int tmp_index_585;
    int _for_it_43;
    int _for_it_44;
    int tmp_index_612;
    int tmp_index_616;
    int tmp_index_626;
    int tmp_index_628;
    int tmp_index_638;
    int tmp_index_640;
    int tmp_index_650;
    int tmp_index_652;
    int i_startidx_2;
    int i_endidx_2;
    int i_startidx_var_150;
    int i_endidx_var_151;
    int _for_it_26;
    int _for_it_27;
    int tmp_index_490;
    int tmp_index_492;
    int tmp_index_502;
    int tmp_index_504;
    int tmp_index_514;
    int tmp_index_516;
    int _for_it_28;
    int _for_it_29;
    int _for_it_45;
    int _if_cond_26;
    int _for_it_46;
    int _if_cond_27;
    int tmp_index_667;
    int tmp_index_671;
    int tmp_index_686;
    int tmp_index_688;
    int tmp_index_698;
    int tmp_index_700;
    int tmp_index_710;
    int tmp_index_712;
    int _for_it_47;
    int tmp_call_15;
    int tmp_parfor_0;
    int _for_it_48;
    int i_startidx_in_var_93_0;
    int i_endidx_in_var_94_0;
    int tmp_arg_17;
    int _for_it_57;
    int _if_cond_32;
    int _for_it_58;
    int tmp_index_970;
    int tmp_index_972;
    int tmp_index_982;
    int tmp_index_984;
    int tmp_index_994;
    int tmp_index_998;
    int tmp_index_1016;
    int tmp_index_1018;
    int tmp_index_1028;
    int tmp_index_1030;
    int tmp_index_1040;
    int tmp_index_1042;
    int tmp_index_1052;
    int tmp_index_1054;
    int tmp_index_1065;
    int tmp_index_1067;
    int tmp_index_1074;
    int tmp_index_1076;
    int _for_it_49;
    int _for_it_50;
    int tmp_index_724;
    int tmp_index_743;
    int tmp_index_745;
    int tmp_index_755;
    int tmp_index_757;
    int tmp_index_769;
    int tmp_index_771;
    int tmp_index_778;
    int tmp_index_780;
    int tmp_index_790;
    int tmp_index_792;
    int tmp_index_802;
    int tmp_index_804;
    int _for_it_51;
    int _for_it_52;
    int tmp_index_817;
    int _for_it_3_0;
    int i_startidx_in_var_105_0_0;
    int i_endidx_in_var_106_0_0;
    int _for_it_4_0;
    int _for_it_5_0;
    int tmp_index_92_0;
    int tmp_index_94_0;
    int tmp_index_104_0;
    int tmp_index_106_0;
    int tmp_index_116_0;
    int tmp_index_118_0;
    int tmp_index_128_0;
    int tmp_index_130_0;
    int tmp_index_140_0;
    int tmp_index_142_0;
    int tmp_index_152_0;
    int tmp_index_154_0;
    int i_startidx_var_122_0;
    int i_endidx_var_123_0;
    int _for_it_0_0_0;
    int _for_it_1_0_0;
    int _for_it_2_0_0;
    int tmp_index_36_0_0;
    int tmp_index_74_0_0;
    int tmp_index_50_0_0;
    int tmp_index_24_0_0;
    int tmp_index_60_0_0;
    int tmp_index_62_0_0;
    int tmp_index_14_0_0;
    int tmp_index_72_0_0;
    int tmp_index_38_0_0;
    int tmp_index_26_0_0;
    int tmp_index_48_0_0;
    int tmp_index_12_0_0;
    int i_startidx_var_46_0_0;
    int i_endidx_var_47_0_0;
    int _for_it_6;
    int i_startidx_in_var_93_3;
    int i_endidx_in_var_94_3;
    int _for_it_7;
    int _for_it_8;
    int tmp_index_178;
    int tmp_index_180;
    int tmp_index_190;
    int tmp_index_192;
    int tmp_index_202;
    int tmp_index_204;
    int tmp_index_214;
    int tmp_index_216;
    int _for_it_9;
    int _for_it_10;
    int _for_it_13;
    int _for_it_14;
    int _for_it_15;
    int tmp_index_293;
    int tmp_index_299;
    int tmp_index_305;
    int tmp_index_311;
    int _for_it_16;
    int tmp_index_338;
    int tmp_index_344;
    int tmp_index_350;
    int tmp_index_356;
    int _for_it_11;
    int _for_it_12;
    int _for_it_17;
    int i_startidx_in_var_93_2;
    int i_endidx_in_var_94_2;
    int _for_it_18;
    int _for_it_19;
    int tmp_index_374;
    int tmp_index_376;
    int tmp_index_383;
    int tmp_index_385;
    int tmp_index_399;
    int tmp_index_401;
    int tmp_index_408;
    int tmp_index_410;

    {
        int _anchor_0;
        int _anchor_1;
        int _anchor_2;
        int _anchor_3;
        int _anchor_4;
        int _anchor_5;
        int _anchor_6;
        int _anchor_7;

        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_0)
            _out = (((((((((((((((((((((((((((((((A_z_kin_hor_e_d_0 + A_z_kin_hor_e_d_1) + A_z_kin_hor_e_d_2) + A_z_vt_ie_d_0) + A_z_vt_ie_d_1) + A_z_vt_ie_d_2) + A_z_w_concorr_me_d_0) + A_z_w_concorr_me_d_1) + A_z_w_concorr_me_d_2) + OA_z_kin_hor_e_d_0) + OA_z_kin_hor_e_d_1) + OA_z_kin_hor_e_d_2) + OA_z_vt_ie_d_0) + OA_z_vt_ie_d_1) + OA_z_vt_ie_d_2) + OA_z_w_concorr_me_d_0) + OA_z_w_concorr_me_d_1) + OA_z_w_concorr_me_d_2) + SA_area_d_0_cells_p_patch_2) + SA_area_d_1_cells_p_patch_2) + SA_cell_blk_d_1_edges_p_patch_4) + SA_cell_blk_d_1_verts_p_patch_5) + SA_cell_blk_d_2_edges_p_patch_4) + SA_cell_blk_d_2_verts_p_patch_5) + SA_cell_idx_d_1_edges_p_patch_4) + SA_cell_idx_d_1_verts_p_patch_5) + SA_cell_idx_d_2_edges_p_patch_4) + SA_cell_idx_d_2_verts_p_patch_5) + SA_edge_blk_d_2_cells_p_patch_2) + SA_edge_blk_d_2_verts_p_patch_5) + SA_edge_idx_d_2_cells_p_patch_2) + SA_edge_idx_d_2_verts_p_patch_5);
            ///////////////////

            _anchor_0 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_1)
            _out = (((((((((((((((((((((((((((((((SA_end_block_d_0_cells_p_patch_2 + SA_end_block_d_0_edges_p_patch_4) + SA_end_block_d_0_verts_p_patch_5) + SA_end_index_d_0_cells_p_patch_2) + SA_end_index_d_0_edges_p_patch_4) + SA_end_index_d_0_verts_p_patch_5) + SA_neighbor_blk_d_2_cells_p_patch_2) + SA_neighbor_idx_d_2_cells_p_patch_2) + SA_quad_blk_d_2_edges_p_patch_4) + SA_quad_idx_d_2_edges_p_patch_4) + SA_start_block_d_0_cells_p_patch_2) + SA_start_block_d_0_edges_p_patch_4) + SA_start_block_d_0_verts_p_patch_5) + SA_start_index_d_0_cells_p_patch_2) + SA_start_index_d_0_edges_p_patch_4) + SA_start_index_d_0_verts_p_patch_5) + SA_vertex_blk_d_2_edges_p_patch_4) + SA_vertex_idx_d_2_edges_p_patch_4) + SOA_area_d_0_cells_p_patch_2) + SOA_area_d_1_cells_p_patch_2) + SOA_end_block_d_0_cells_p_patch_2) + SOA_end_block_d_0_edges_p_patch_4) + SOA_end_block_d_0_verts_p_patch_5) + SOA_end_index_d_0_cells_p_patch_2) + SOA_end_index_d_0_edges_p_patch_4) + SOA_end_index_d_0_verts_p_patch_5) + SOA_start_block_d_0_cells_p_patch_2) + SOA_start_block_d_0_edges_p_patch_4) + SOA_start_block_d_0_verts_p_patch_5) + SOA_start_index_d_0_cells_p_patch_2) + SOA_start_index_d_0_edges_p_patch_4) + SOA_start_index_d_0_verts_p_patch_5);
            ///////////////////

            _anchor_1 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_2)
            _out = (((((((((((((((((((((((((((((((__CG_global_data__m_nproma + __CG_p_diag__m_SA_ddt_vn_apc_pc_d_0) + __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) + __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2) + __CG_p_diag__m_SA_ddt_vn_apc_pc_d_3) + __CG_p_diag__m_SA_ddt_vn_cor_pc_d_0) + __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1) + __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2) + __CG_p_diag__m_SA_ddt_vn_cor_pc_d_3) + __CG_p_diag__m_SA_ddt_w_adv_pc_d_0) + __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) + __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) + __CG_p_diag__m_SA_ddt_w_adv_pc_d_3) + __CG_p_diag__m_SA_vn_ie_d_0) + __CG_p_diag__m_SA_vn_ie_d_1) + __CG_p_diag__m_SA_vn_ie_d_2) + __CG_p_diag__m_SA_vn_ie_ubc_d_0) + __CG_p_diag__m_SA_vn_ie_ubc_d_1) + __CG_p_diag__m_SA_vn_ie_ubc_d_2) + __CG_p_diag__m_SA_vt_d_0) + __CG_p_diag__m_SA_vt_d_1) + __CG_p_diag__m_SA_vt_d_2) + __CG_p_diag__m_SA_w_concorr_c_d_0) + __CG_p_diag__m_SA_w_concorr_c_d_1) + __CG_p_diag__m_SA_w_concorr_c_d_2) + __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0) + __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1) + __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2) + __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3) + __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0) + __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1) + __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2);
            ///////////////////

            _anchor_2 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_3)
            _out = (((((((((((((((((((((((((((((((__CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3 + __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3) + __CG_p_diag__m_SOA_vn_ie_d_0) + __CG_p_diag__m_SOA_vn_ie_d_1) + __CG_p_diag__m_SOA_vn_ie_d_2) + __CG_p_diag__m_SOA_vn_ie_ubc_d_0) + __CG_p_diag__m_SOA_vn_ie_ubc_d_1) + __CG_p_diag__m_SOA_vn_ie_ubc_d_2) + __CG_p_diag__m_SOA_vt_d_0) + __CG_p_diag__m_SOA_vt_d_1) + __CG_p_diag__m_SOA_vt_d_2) + __CG_p_diag__m_SOA_w_concorr_c_d_0) + __CG_p_diag__m_SOA_w_concorr_c_d_1) + __CG_p_diag__m_SOA_w_concorr_c_d_2) + __CG_p_int__m_SA_c_lin_e_d_0) + __CG_p_int__m_SA_c_lin_e_d_1) + __CG_p_int__m_SA_c_lin_e_d_2) + __CG_p_int__m_SA_cells_aw_verts_d_0) + __CG_p_int__m_SA_cells_aw_verts_d_1) + __CG_p_int__m_SA_cells_aw_verts_d_2) + __CG_p_int__m_SA_e_bln_c_s_d_0) + __CG_p_int__m_SA_e_bln_c_s_d_1) + __CG_p_int__m_SA_e_bln_c_s_d_2) + __CG_p_int__m_SA_geofac_grdiv_d_0) + __CG_p_int__m_SA_geofac_grdiv_d_1) + __CG_p_int__m_SA_geofac_grdiv_d_2) + __CG_p_int__m_SA_geofac_n2s_d_0) + __CG_p_int__m_SA_geofac_n2s_d_1) + __CG_p_int__m_SA_geofac_n2s_d_2);
            ///////////////////

            _anchor_3 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_4)
            _out = (((((((((((((((((((((((((((((((__CG_p_int__m_SA_geofac_rot_d_0 + __CG_p_int__m_SA_geofac_rot_d_1) + __CG_p_int__m_SA_geofac_rot_d_2) + __CG_p_int__m_SA_rbf_vec_coeff_e_d_0) + __CG_p_int__m_SA_rbf_vec_coeff_e_d_1) + __CG_p_int__m_SA_rbf_vec_coeff_e_d_2) + __CG_p_int__m_SOA_c_lin_e_d_0) + __CG_p_int__m_SOA_c_lin_e_d_1) + __CG_p_int__m_SOA_c_lin_e_d_2) + __CG_p_int__m_SOA_cells_aw_verts_d_0) + __CG_p_int__m_SOA_cells_aw_verts_d_1) + __CG_p_int__m_SOA_cells_aw_verts_d_2) + __CG_p_int__m_SOA_e_bln_c_s_d_0) + __CG_p_int__m_SOA_e_bln_c_s_d_1) + __CG_p_int__m_SOA_e_bln_c_s_d_2) + __CG_p_int__m_SOA_geofac_grdiv_d_0) + __CG_p_int__m_SOA_geofac_grdiv_d_1) + __CG_p_int__m_SOA_geofac_grdiv_d_2) + __CG_p_int__m_SOA_geofac_n2s_d_0) + __CG_p_int__m_SOA_geofac_n2s_d_1) + __CG_p_int__m_SOA_geofac_n2s_d_2) + __CG_p_int__m_SOA_geofac_rot_d_0) + __CG_p_int__m_SOA_geofac_rot_d_1) + __CG_p_int__m_SOA_geofac_rot_d_2) + __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0) + __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1) + __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2) + __CG_p_metrics__m_SA_coeff1_dwdz_d_0) + __CG_p_metrics__m_SA_coeff1_dwdz_d_1) + __CG_p_metrics__m_SA_coeff1_dwdz_d_2) + __CG_p_metrics__m_SA_coeff2_dwdz_d_0) + __CG_p_metrics__m_SA_coeff2_dwdz_d_1);
            ///////////////////

            _anchor_4 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_5)
            _out = (((((((((((((((((((((((((((((((__CG_p_metrics__m_SA_coeff2_dwdz_d_2 + __CG_p_metrics__m_SA_coeff_gradekin_d_0) + __CG_p_metrics__m_SA_coeff_gradekin_d_1) + __CG_p_metrics__m_SA_coeff_gradekin_d_2) + __CG_p_metrics__m_SA_ddqz_z_full_e_d_0) + __CG_p_metrics__m_SA_ddqz_z_full_e_d_1) + __CG_p_metrics__m_SA_ddqz_z_full_e_d_2) + __CG_p_metrics__m_SA_ddqz_z_half_d_0) + __CG_p_metrics__m_SA_ddqz_z_half_d_1) + __CG_p_metrics__m_SA_ddqz_z_half_d_2) + __CG_p_metrics__m_SA_ddxn_z_full_d_0) + __CG_p_metrics__m_SA_ddxn_z_full_d_1) + __CG_p_metrics__m_SA_ddxn_z_full_d_2) + __CG_p_metrics__m_SA_ddxt_z_full_d_0) + __CG_p_metrics__m_SA_ddxt_z_full_d_1) + __CG_p_metrics__m_SA_ddxt_z_full_d_2) + __CG_p_metrics__m_SA_deepatmo_gradh_ifc_d_0) + __CG_p_metrics__m_SA_deepatmo_gradh_mc_d_0) + __CG_p_metrics__m_SA_deepatmo_invr_ifc_d_0) + __CG_p_metrics__m_SA_deepatmo_invr_mc_d_0) + __CG_p_metrics__m_SA_wgtfac_c_d_0) + __CG_p_metrics__m_SA_wgtfac_c_d_1) + __CG_p_metrics__m_SA_wgtfac_c_d_2) + __CG_p_metrics__m_SA_wgtfac_e_d_0) + __CG_p_metrics__m_SA_wgtfac_e_d_1) + __CG_p_metrics__m_SA_wgtfac_e_d_2) + __CG_p_metrics__m_SA_wgtfacq_e_d_0) + __CG_p_metrics__m_SA_wgtfacq_e_d_1) + __CG_p_metrics__m_SA_wgtfacq_e_d_2) + __CG_p_metrics__m_SOA_coeff1_dwdz_d_0) + __CG_p_metrics__m_SOA_coeff1_dwdz_d_1) + __CG_p_metrics__m_SOA_coeff1_dwdz_d_2);
            ///////////////////

            _anchor_5 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_6)
            _out = (((((((((((((((((((((((((((((((__CG_p_metrics__m_SOA_coeff2_dwdz_d_0 + __CG_p_metrics__m_SOA_coeff2_dwdz_d_1) + __CG_p_metrics__m_SOA_coeff2_dwdz_d_2) + __CG_p_metrics__m_SOA_coeff_gradekin_d_0) + __CG_p_metrics__m_SOA_coeff_gradekin_d_1) + __CG_p_metrics__m_SOA_coeff_gradekin_d_2) + __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0) + __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1) + __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2) + __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + __CG_p_metrics__m_SOA_ddxn_z_full_d_0) + __CG_p_metrics__m_SOA_ddxn_z_full_d_1) + __CG_p_metrics__m_SOA_ddxn_z_full_d_2) + __CG_p_metrics__m_SOA_ddxt_z_full_d_0) + __CG_p_metrics__m_SOA_ddxt_z_full_d_1) + __CG_p_metrics__m_SOA_ddxt_z_full_d_2) + __CG_p_metrics__m_SOA_deepatmo_gradh_ifc_d_0) + __CG_p_metrics__m_SOA_deepatmo_gradh_mc_d_0) + __CG_p_metrics__m_SOA_deepatmo_invr_ifc_d_0) + __CG_p_metrics__m_SOA_deepatmo_invr_mc_d_0) + __CG_p_metrics__m_SOA_wgtfac_c_d_0) + __CG_p_metrics__m_SOA_wgtfac_c_d_1) + __CG_p_metrics__m_SOA_wgtfac_c_d_2) + __CG_p_metrics__m_SOA_wgtfac_e_d_0) + __CG_p_metrics__m_SOA_wgtfac_e_d_1) + __CG_p_metrics__m_SOA_wgtfac_e_d_2) + __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + __CG_p_metrics__m_SOA_wgtfacq_e_d_1) + __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + __CG_p_patch__m_nblks_c);
            ///////////////////

            _anchor_6 = _out;
        }
        {
            int _out;

            ///////////////////
            // Tasklet code (__sig_anchor_use_7)
            _out = (((((((((((((((__CG_p_patch__m_nblks_e + __CG_p_patch__m_nblks_v) + __CG_p_patch__m_nlev) + __CG_p_patch__m_nlevp1) + __CG_p_prog__m_SA_vn_d_0) + __CG_p_prog__m_SA_vn_d_1) + __CG_p_prog__m_SA_vn_d_2) + __CG_p_prog__m_SA_w_d_0) + __CG_p_prog__m_SA_w_d_1) + __CG_p_prog__m_SA_w_d_2) + __CG_p_prog__m_SOA_vn_d_0) + __CG_p_prog__m_SOA_vn_d_1) + __CG_p_prog__m_SOA_vn_d_2) + __CG_p_prog__m_SOA_w_d_0) + __CG_p_prog__m_SOA_w_d_1) + __CG_p_prog__m_SOA_w_d_2);
            ///////////////////

            _anchor_7 = _out;
        }

    }
    {

        {
            int global_data_0_in_lvert_nest = __CG_global_data__m_lvert_nest;
            int p_patch_0_in_nshift = __CG_p_patch__m_nshift;
            int _if_cond_9_out;

            ///////////////////
            // Tasklet code (T_l0_c0)
            _if_cond_9_out = (global_data_0_in_lvert_nest && (p_patch_0_in_nshift > 0));
            ///////////////////

            _if_cond_9 = _if_cond_9_out;
        }

    }
    if ((_if_cond_9 == 1)) {

        l_vert_nested = 1;

    } else {

        l_vert_nested = 0;

    }
    {

        {
            int p_patch_0_in_id = __CG_p_patch__m_id;
            int jg_out;

            ///////////////////
            // Tasklet code (T_l407_c407)
            jg_out = p_patch_0_in_id;
            ///////////////////

            jg = jg_out;
        }

    }
    tmp_index_162 = (jg - 1);
    tmp_index_163 = (jg - 1);
    {

        {
            int nlev_var_154_out;

            ///////////////////
            // Tasklet code (T_l410_c410)
            nlev_var_154_out = __CG_p_patch__m_nlev;
            ///////////////////

            nlev_var_154 = nlev_var_154_out;
        }
        {
            int nlevp1_var_155_out;

            ///////////////////
            // Tasklet code (T_l411_c411)
            nlevp1_var_155_out = __CG_p_patch__m_nlevp1;
            ///////////////////

            nlevp1_var_155 = nlevp1_var_155_out;
        }
        {
            int global_data_0_in_nrdmax_0 = __CG_global_data__m_nrdmax[tmp_index_162];
            int nrdmax_jg_out;

            ///////////////////
            // Tasklet code (T_l408_c408)
            nrdmax_jg_out = global_data_0_in_nrdmax_0;
            ///////////////////

            nrdmax_jg = nrdmax_jg_out;
        }
        {
            int global_data_0_in_nflatlev_0 = __CG_global_data__m_nflatlev[tmp_index_163];
            int nflatlev_jg_out;

            ///////////////////
            // Tasklet code (T_l409_c409)
            nflatlev_jg_out = global_data_0_in_nflatlev_0;
            ///////////////////

            nflatlev_jg = nflatlev_jg_out;
        }
        {
            double dtime_0_in = dtime;
            double cfl_w_limit_out;

            ///////////////////
            // Tasklet code (T_l413_c413)
            cfl_w_limit_out = (0.65 / dtime_0_in);
            ///////////////////

            cfl_w_limit = cfl_w_limit_out;
        }
        {
            double cfl_w_limit_0_in = cfl_w_limit;
            double dtime_0_in = dtime;
            double dtime_1_in = dtime;
            double scalfac_exdiff_out;

            ///////////////////
            // Tasklet code (T_l414_c414)
            scalfac_exdiff_out = (0.05 / (dtime_0_in * (0.85 - (cfl_w_limit_0_in * dtime_1_in))));
            ///////////////////

            scalfac_exdiff = scalfac_exdiff_out;
        }

    }
    i_startblk_var_142_0 = __CG_p_patch__CG_verts__m_start_block[(2 - SOA_start_block_d_0_verts_p_patch_5)];
    i_endblk_var_143_0 = __CG_p_patch__CG_verts__m_end_block[((- SOA_end_block_d_0_verts_p_patch_5) - 5)];
    i_startidx_in_var_144_0 = __CG_p_patch__CG_verts__m_start_index[(2 - SOA_start_index_d_0_verts_p_patch_5)];
    i_endidx_in_var_145_0 = __CG_p_patch__CG_verts__m_end_index[((- SOA_end_index_d_0_verts_p_patch_5) - 5)];
    for (_for_it_0_0_0 = i_startblk_var_142_0; (_for_it_0_0_0 <= i_endblk_var_143_0); _for_it_0_0_0 = (_for_it_0_0_0 + 1)) {
        if (((_for_it_0_0_0 == i_startblk_var_142_0) == 1)) {

            i_startidx_var_46_0_0 = i_startidx_in_var_144_0;
            i_endidx_var_47_0_0 = __CG_global_data__m_nproma;
            if (((_for_it_0_0_0 == i_endblk_var_143_0) == 1)) {

                i_endidx_var_47_0_0 = i_endidx_in_var_145_0;

            }
        } else {
            if (((_for_it_0_0_0 == i_endblk_var_143_0) == 1)) {

                i_startidx_var_46_0_0 = 1;
                i_endidx_var_47_0_0 = i_endidx_in_var_145_0;

            } else {

                i_startidx_var_46_0_0 = 1;
                i_endidx_var_47_0_0 = __CG_global_data__m_nproma;

            }
        }
        for (_for_it_1_0_0 = 1; (_for_it_1_0_0 <= __CG_p_prog__m_SA_w_d_1); _for_it_1_0_0 = (_for_it_1_0_0 + 1)) {
            for (_for_it_2_0_0 = i_startidx_var_46_0_0; (_for_it_2_0_0 <= i_endidx_var_47_0_0); _for_it_2_0_0 = (_for_it_2_0_0 + 1)) {

                tmp_index_36_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[(((((2 * SA_cell_idx_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_74_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[(((((5 * SA_cell_blk_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_50_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[(((((3 * SA_cell_blk_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_24_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[((((SA_cell_idx_d_1_verts_p_patch_5 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_60_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[(((((4 * SA_cell_idx_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_62_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[(((((4 * SA_cell_blk_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_14_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[(((__CG_global_data__m_nproma * (_for_it_0_0_0 - 1)) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_72_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[(((((5 * SA_cell_idx_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_38_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[(((((2 * SA_cell_blk_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_26_0_0 = (__CG_p_patch__CG_verts__m_cell_blk[((((SA_cell_blk_d_1_verts_p_patch_5 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_48_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[(((((3 * SA_cell_idx_d_1_verts_p_patch_5) * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_0_0_0 - 1))) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_12_0_0 = (__CG_p_patch__CG_verts__m_cell_idx[(((__CG_global_data__m_nproma * (_for_it_0_0_0 - 1)) + _for_it_2_0_0) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                {

                    {
                        double c_int_var_33_0_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (1 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double c_int_var_33_1_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (2 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double c_int_var_33_2_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (3 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double c_int_var_33_3_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (4 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double c_int_var_33_4_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (5 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double c_int_var_33_5_in_0 = __CG_p_int__m_cells_aw_verts[(((((__CG_p_int__m_SA_cells_aw_verts_d_0 * __CG_p_int__m_SA_cells_aw_verts_d_1) * ((- __CG_p_int__m_SOA_cells_aw_verts_d_2) + _for_it_0_0_0)) + (__CG_p_int__m_SA_cells_aw_verts_d_0 * (6 - __CG_p_int__m_SOA_cells_aw_verts_d_1))) - __CG_p_int__m_SOA_cells_aw_verts_d_0) + _for_it_2_0_0)];
                        double p_cell_in_var_32_0_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_14_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_12_0_0)];
                        double p_cell_in_var_32_1_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_26_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_24_0_0)];
                        double p_cell_in_var_32_2_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_38_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_36_0_0)];
                        double p_cell_in_var_32_3_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_50_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_48_0_0)];
                        double p_cell_in_var_32_4_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_62_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_60_0_0)];
                        double p_cell_in_var_32_5_in_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_74_0_0) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_1_0_0))) + tmp_index_72_0_0)];
                        double p_vert_out_var_34_out_0;

                        ///////////////////
                        // Tasklet code (T_l135_c135)
                        p_vert_out_var_34_out_0 = ((((((c_int_var_33_0_in_0 * p_cell_in_var_32_0_in_0) + (c_int_var_33_1_in_0 * p_cell_in_var_32_1_in_0)) + (c_int_var_33_2_in_0 * p_cell_in_var_32_2_in_0)) + (c_int_var_33_3_in_0 * p_cell_in_var_32_3_in_0)) + (c_int_var_33_4_in_0 * p_cell_in_var_32_4_in_0)) + (c_int_var_33_5_in_0 * p_cell_in_var_32_5_in_0));
                        ///////////////////

                        z_w_v[(((((__CG_global_data__m_nproma * __CG_p_patch__m_nlevp1) * (_for_it_0_0_0 - 1)) + (__CG_global_data__m_nproma * (_for_it_1_0_0 - 1))) + _for_it_2_0_0) - 1)] = p_vert_out_var_34_out_0;
                    }

                }

            }

        }

    }
    i_startblk_var_120_0 = __CG_p_patch__CG_verts__m_start_block[(2 - SOA_start_block_d_0_verts_p_patch_5)];
    i_endblk_var_121_0 = __CG_p_patch__CG_verts__m_end_block[((- SOA_end_block_d_0_verts_p_patch_5) - 5)];
    for (_for_it_3_0 = i_startblk_var_120_0; (_for_it_3_0 <= i_endblk_var_121_0); _for_it_3_0 = (_for_it_3_0 + 1)) {

        i_startidx_in_var_105_0_0 = __CG_p_patch__CG_verts__m_start_index[(2 - SOA_start_index_d_0_verts_p_patch_5)];
        i_endidx_in_var_106_0_0 = __CG_p_patch__CG_verts__m_end_index[((- SOA_end_index_d_0_verts_p_patch_5) - 5)];
        if (((_for_it_3_0 == i_startblk_var_120_0) == 1)) {

            i_startidx_var_122_0 = i_startidx_in_var_105_0_0;
            i_endidx_var_123_0 = __CG_global_data__m_nproma;
            if (((_for_it_3_0 == i_endblk_var_121_0) == 1)) {

                i_endidx_var_123_0 = i_endidx_in_var_106_0_0;

            }
        } else {
            if (((_for_it_3_0 == i_endblk_var_121_0) == 1)) {

                i_endidx_var_123_0 = i_endidx_in_var_106_0_0;
                i_startidx_var_122_0 = 1;

            } else {

                i_startidx_var_122_0 = 1;
                i_endidx_var_123_0 = __CG_global_data__m_nproma;

            }
        }
        for (_for_it_4_0 = 1; (_for_it_4_0 <= __CG_p_prog__m_SA_vn_d_1); _for_it_4_0 = (_for_it_4_0 + 1)) {
            for (_for_it_5_0 = i_startidx_var_122_0; (_for_it_5_0 <= i_endidx_var_123_0); _for_it_5_0 = (_for_it_5_0 + 1)) {

                tmp_index_92_0 = (__CG_p_patch__CG_verts__m_edge_idx[(((__CG_global_data__m_nproma * (_for_it_3_0 - 1)) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_94_0 = (__CG_p_patch__CG_verts__m_edge_blk[(((__CG_global_data__m_nproma * (_for_it_3_0 - 1)) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_104_0 = (__CG_p_patch__CG_verts__m_edge_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_106_0 = (__CG_p_patch__CG_verts__m_edge_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_116_0 = (__CG_p_patch__CG_verts__m_edge_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_118_0 = (__CG_p_patch__CG_verts__m_edge_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_128_0 = (__CG_p_patch__CG_verts__m_edge_idx[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_130_0 = (__CG_p_patch__CG_verts__m_edge_blk[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_140_0 = (__CG_p_patch__CG_verts__m_edge_idx[(((((4 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_142_0 = (__CG_p_patch__CG_verts__m_edge_blk[(((((4 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_152_0 = (__CG_p_patch__CG_verts__m_edge_idx[(((((5 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_154_0 = (__CG_p_patch__CG_verts__m_edge_blk[(((((5 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_3_0 - 1))) + _for_it_5_0) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                {

                    {
                        double ptr_int_0_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (1 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double ptr_int_1_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (2 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double ptr_int_2_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (3 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double ptr_int_3_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (4 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double ptr_int_4_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (5 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double ptr_int_5_in_geofac_rot_0 = __CG_p_int__m_geofac_rot[(((((__CG_p_int__m_SA_geofac_rot_d_0 * __CG_p_int__m_SA_geofac_rot_d_1) * ((- __CG_p_int__m_SOA_geofac_rot_d_2) + _for_it_3_0)) + (__CG_p_int__m_SA_geofac_rot_d_0 * (6 - __CG_p_int__m_SOA_geofac_rot_d_1))) - __CG_p_int__m_SOA_geofac_rot_d_0) + _for_it_5_0)];
                        double vec_e_0_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_94_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_92_0)];
                        double vec_e_1_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_106_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_104_0)];
                        double vec_e_2_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_118_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_116_0)];
                        double vec_e_3_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_130_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_128_0)];
                        double vec_e_4_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_142_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_140_0)];
                        double vec_e_5_in_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_154_0) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_4_0))) + tmp_index_152_0)];
                        double rot_vec_out_0;

                        ///////////////////
                        // Tasklet code (T_l288_c288)
                        rot_vec_out_0 = ((((((vec_e_0_in_0 * ptr_int_0_in_geofac_rot_0) + (vec_e_1_in_0 * ptr_int_1_in_geofac_rot_0)) + (vec_e_2_in_0 * ptr_int_2_in_geofac_rot_0)) + (vec_e_3_in_0 * ptr_int_3_in_geofac_rot_0)) + (vec_e_4_in_0 * ptr_int_4_in_geofac_rot_0)) + (vec_e_5_in_0 * ptr_int_5_in_geofac_rot_0));
                        ///////////////////

                        zeta[(((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (_for_it_3_0 - 1)) + (__CG_global_data__m_nproma * (_for_it_4_0 - 1))) + _for_it_5_0) - 1)] = rot_vec_out_0;
                    }

                }

            }

        }

    }
    i_startblk_var_148 = __CG_p_patch__CG_edges__m_start_block[(5 - SOA_start_block_d_0_edges_p_patch_4)];
    i_endblk_var_149 = __CG_p_patch__CG_edges__m_end_block[((- SOA_end_block_d_0_edges_p_patch_4) - 10)];
    for (_for_it_6 = i_startblk_var_148; (_for_it_6 <= i_endblk_var_149); _for_it_6 = (_for_it_6 + 1)) {

        i_startidx_in_var_93_3 = __CG_p_patch__CG_edges__m_start_index[(5 - SOA_start_index_d_0_edges_p_patch_4)];
        i_endidx_in_var_94_3 = __CG_p_patch__CG_edges__m_end_index[((- SOA_end_index_d_0_edges_p_patch_4) - 10)];
        if ((_for_it_6 != i_startblk_var_148)) {

            i_startidx_var_150 = 1;

        } else {

            i_startidx_var_150 = max(1, i_startidx_in_var_93_3);

        }
        if ((_for_it_6 != i_endblk_var_149)) {

            i_endidx_var_151 = __CG_global_data__m_nproma;

        } else {

            i_endidx_var_151 = i_endidx_in_var_94_3;

        }
        for (_for_it_7 = 1; (_for_it_7 <= nlev_var_154); _for_it_7 = (_for_it_7 + 1)) {
            for (_for_it_8 = i_startidx_var_150; (_for_it_8 <= i_endidx_var_151); _for_it_8 = (_for_it_8 + 1)) {

                tmp_index_178 = (__CG_p_patch__CG_edges__m_quad_idx[(((__CG_global_data__m_nproma * (_for_it_6 - 1)) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_180 = (__CG_p_patch__CG_edges__m_quad_blk[(((__CG_global_data__m_nproma * (_for_it_6 - 1)) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_190 = (__CG_p_patch__CG_edges__m_quad_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_192 = (__CG_p_patch__CG_edges__m_quad_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_202 = (__CG_p_patch__CG_edges__m_quad_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_204 = (__CG_p_patch__CG_edges__m_quad_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                tmp_index_214 = (__CG_p_patch__CG_edges__m_quad_idx[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                tmp_index_216 = (__CG_p_patch__CG_edges__m_quad_blk[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_6 - 1))) + _for_it_8) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                {

                    {
                        double p_int_0_in_rbf_vec_coeff_e_0 = __CG_p_int__m_rbf_vec_coeff_e[(((((__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * __CG_p_int__m_SA_rbf_vec_coeff_e_d_1) * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2) + _for_it_6)) + (__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1) + _for_it_8))) - __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0) + 1)];
                        double p_int_1_in_rbf_vec_coeff_e_0 = __CG_p_int__m_rbf_vec_coeff_e[(((((__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * __CG_p_int__m_SA_rbf_vec_coeff_e_d_1) * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2) + _for_it_6)) + (__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1) + _for_it_8))) - __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0) + 2)];
                        double p_int_2_in_rbf_vec_coeff_e_0 = __CG_p_int__m_rbf_vec_coeff_e[(((((__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * __CG_p_int__m_SA_rbf_vec_coeff_e_d_1) * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2) + _for_it_6)) + (__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1) + _for_it_8))) - __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0) + 3)];
                        double p_int_3_in_rbf_vec_coeff_e_0 = __CG_p_int__m_rbf_vec_coeff_e[(((((__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * __CG_p_int__m_SA_rbf_vec_coeff_e_d_1) * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2) + _for_it_6)) + (__CG_p_int__m_SA_rbf_vec_coeff_e_d_0 * ((- __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1) + _for_it_8))) - __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0) + 4)];
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_180) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_7))) + tmp_index_178)];
                        double p_prog_1_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_192) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_7))) + tmp_index_190)];
                        double p_prog_2_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_204) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_7))) + tmp_index_202)];
                        double p_prog_3_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_216) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_7))) + tmp_index_214)];
                        double p_diag_out_vt_0;

                        ///////////////////
                        // Tasklet code (T_l430_c430)
                        p_diag_out_vt_0 = ((((p_int_0_in_rbf_vec_coeff_e_0 * p_prog_0_in_vn_0) + (p_int_1_in_rbf_vec_coeff_e_0 * p_prog_1_in_vn_0)) + (p_int_2_in_rbf_vec_coeff_e_0 * p_prog_2_in_vn_0)) + (p_int_3_in_rbf_vec_coeff_e_0 * p_prog_3_in_vn_0));
                        ///////////////////

                        __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_7))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_8)] = p_diag_out_vt_0;
                    }

                }

            }

        }
        for (_for_it_9 = 2; (_for_it_9 <= nlev_var_154); _for_it_9 = (_for_it_9 + 1)) {
            for (_for_it_10 = i_startidx_var_150; (_for_it_10 <= i_endidx_var_151); _for_it_10 = (_for_it_10 + 1)) {
                {
                    double tmp_call_2;
                    double tmp_call_1;

                    {
                        double p_metrics_0_in_wgtfac_e_0 = __CG_p_metrics__m_wgtfac_e[(((((__CG_p_metrics__m_SA_wgtfac_e_d_0 * __CG_p_metrics__m_SA_wgtfac_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfac_e_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_1) + _for_it_9))) - __CG_p_metrics__m_SOA_wgtfac_e_d_0) + _for_it_10)];
                        double p_metrics_1_in_wgtfac_e_0 = __CG_p_metrics__m_wgtfac_e[(((((__CG_p_metrics__m_SA_wgtfac_e_d_0 * __CG_p_metrics__m_SA_wgtfac_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfac_e_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_1) + _for_it_9))) - __CG_p_metrics__m_SOA_wgtfac_e_d_0) + _for_it_10)];
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_9))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_10)];
                        double p_prog_1_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * (((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_9) - 1))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_10)];
                        double p_diag_out_vn_ie_0;

                        ///////////////////
                        // Tasklet code (T_l435_c435)
                        p_diag_out_vn_ie_0 = ((p_metrics_0_in_wgtfac_e_0 * p_prog_0_in_vn_0) + ((1.0 - p_metrics_1_in_wgtfac_e_0) * p_prog_1_in_vn_0));
                        ///////////////////

                        __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_d_0 * ((- __CG_p_diag__m_SOA_vn_ie_d_1) + _for_it_9))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_10)] = p_diag_out_vn_ie_0;
                    }
                    {
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_9))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_10)];
                        double tmp_call_1_out;

                        ///////////////////
                        // Tasklet code (T_l436_c436)
                        tmp_call_1_out = (dace::math::ipow(p_prog_0_in_vn_0, 2));
                        ///////////////////

                        tmp_call_1 = tmp_call_1_out;
                    }
                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_9))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_10)];
                        double tmp_call_2_out;

                        ///////////////////
                        // Tasklet code (T_l436_c436)
                        tmp_call_2_out = (dace::math::ipow(p_diag_0_in_vt_0, 2));
                        ///////////////////

                        tmp_call_2 = tmp_call_2_out;
                    }
                    {
                        double tmp_call_1_0_in = tmp_call_1;
                        double tmp_call_2_0_in = tmp_call_2;
                        double z_kin_hor_e_out_0;

                        ///////////////////
                        // Tasklet code (T_l436_c436)
                        z_kin_hor_e_out_0 = (0.5 * (tmp_call_1_0_in + tmp_call_2_0_in));
                        ///////////////////

                        z_kin_hor_e[(((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * ((- OA_z_kin_hor_e_d_2) + _for_it_6)) + (A_z_kin_hor_e_d_0 * ((- OA_z_kin_hor_e_d_1) + _for_it_9))) - OA_z_kin_hor_e_d_0) + _for_it_10)] = z_kin_hor_e_out_0;
                    }

                }

            }

        }
        for (_for_it_11 = 2; (_for_it_11 <= nlev_var_154); _for_it_11 = (_for_it_11 + 1)) {
            for (_for_it_12 = i_startidx_var_150; (_for_it_12 <= i_endidx_var_151); _for_it_12 = (_for_it_12 + 1)) {
                {

                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_11))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_12)];
                        double p_diag_1_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * (((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_11) - 1))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_12)];
                        double p_metrics_0_in_wgtfac_e_0 = __CG_p_metrics__m_wgtfac_e[(((((__CG_p_metrics__m_SA_wgtfac_e_d_0 * __CG_p_metrics__m_SA_wgtfac_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfac_e_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_1) + _for_it_11))) - __CG_p_metrics__m_SOA_wgtfac_e_d_0) + _for_it_12)];
                        double p_metrics_1_in_wgtfac_e_0 = __CG_p_metrics__m_wgtfac_e[(((((__CG_p_metrics__m_SA_wgtfac_e_d_0 * __CG_p_metrics__m_SA_wgtfac_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfac_e_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_e_d_1) + _for_it_11))) - __CG_p_metrics__m_SOA_wgtfac_e_d_0) + _for_it_12)];
                        double z_vt_ie_out_0;

                        ///////////////////
                        // Tasklet code (T_l442_c442)
                        z_vt_ie_out_0 = ((p_metrics_0_in_wgtfac_e_0 * p_diag_0_in_vt_0) + ((1.0 - p_metrics_1_in_wgtfac_e_0) * p_diag_1_in_vt_0));
                        ///////////////////

                        z_vt_ie[(((((A_z_vt_ie_d_0 * A_z_vt_ie_d_1) * ((- OA_z_vt_ie_d_2) + _for_it_6)) + (A_z_vt_ie_d_0 * ((- OA_z_vt_ie_d_1) + _for_it_11))) - OA_z_vt_ie_d_0) + _for_it_12)] = z_vt_ie_out_0;
                    }

                }

            }

        }
        for (_for_it_13 = nflatlev_jg; (_for_it_13 <= nlev_var_154); _for_it_13 = (_for_it_13 + 1)) {
            for (_for_it_14 = i_startidx_var_150; (_for_it_14 <= i_endidx_var_151); _for_it_14 = (_for_it_14 + 1)) {
                {

                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_13))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_14)];
                        double p_metrics_0_in_ddxn_z_full_0 = __CG_p_metrics__m_ddxn_z_full[(((((__CG_p_metrics__m_SA_ddxn_z_full_d_0 * __CG_p_metrics__m_SA_ddxn_z_full_d_1) * ((- __CG_p_metrics__m_SOA_ddxn_z_full_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_ddxn_z_full_d_0 * ((- __CG_p_metrics__m_SOA_ddxn_z_full_d_1) + _for_it_13))) - __CG_p_metrics__m_SOA_ddxn_z_full_d_0) + _for_it_14)];
                        double p_metrics_1_in_ddxt_z_full_0 = __CG_p_metrics__m_ddxt_z_full[(((((__CG_p_metrics__m_SA_ddxt_z_full_d_0 * __CG_p_metrics__m_SA_ddxt_z_full_d_1) * ((- __CG_p_metrics__m_SOA_ddxt_z_full_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_ddxt_z_full_d_0 * ((- __CG_p_metrics__m_SOA_ddxt_z_full_d_1) + _for_it_13))) - __CG_p_metrics__m_SOA_ddxt_z_full_d_0) + _for_it_14)];
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_13))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_14)];
                        double z_w_concorr_me_out_0;

                        ///////////////////
                        // Tasklet code (T_l448_c448)
                        z_w_concorr_me_out_0 = ((p_prog_0_in_vn_0 * p_metrics_0_in_ddxn_z_full_0) + (p_diag_0_in_vt_0 * p_metrics_1_in_ddxt_z_full_0));
                        ///////////////////

                        z_w_concorr_me[(((((A_z_w_concorr_me_d_0 * A_z_w_concorr_me_d_1) * ((- OA_z_w_concorr_me_d_2) + _for_it_6)) + (A_z_w_concorr_me_d_0 * ((- OA_z_w_concorr_me_d_1) + _for_it_13))) - OA_z_w_concorr_me_d_0) + _for_it_14)] = z_w_concorr_me_out_0;
                    }

                }

            }

        }
        if (((1 - l_vert_nested) == 1)) {
            for (_for_it_15 = i_startidx_var_150; (_for_it_15 <= i_endidx_var_151); _for_it_15 = (_for_it_15 + 1)) {
                {

                    {
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * (1 - __CG_p_prog__m_SOA_vn_d_1))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_15)];
                        double p_diag_out_vn_ie_0;

                        ///////////////////
                        // Tasklet code (T_l453_c453)
                        p_diag_out_vn_ie_0 = p_prog_0_in_vn_0;
                        ///////////////////

                        __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_d_0 * (1 - __CG_p_diag__m_SOA_vn_ie_d_1))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_15)] = p_diag_out_vn_ie_0;
                    }
                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * (1 - __CG_p_diag__m_SOA_vt_d_1))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_15)];
                        double z_vt_ie_out_0;

                        ///////////////////
                        // Tasklet code (T_l454_c454)
                        z_vt_ie_out_0 = p_diag_0_in_vt_0;
                        ///////////////////

                        z_vt_ie[(((((A_z_vt_ie_d_0 * A_z_vt_ie_d_1) * ((- OA_z_vt_ie_d_2) + _for_it_6)) + (A_z_vt_ie_d_0 * (1 - OA_z_vt_ie_d_1))) - OA_z_vt_ie_d_0) + _for_it_15)] = z_vt_ie_out_0;
                    }

                }
                tmp_index_293 = (nlevp1_var_155 - __CG_p_diag__m_SOA_vn_ie_d_1);
                tmp_index_299 = (nlev_var_154 - __CG_p_prog__m_SOA_vn_d_1);
                tmp_index_305 = ((nlev_var_154 - 1) - __CG_p_prog__m_SOA_vn_d_1);
                tmp_index_311 = ((nlev_var_154 - 2) - __CG_p_prog__m_SOA_vn_d_1);
                {
                    double tmp_call_4;
                    double tmp_call_3;

                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * (1 - __CG_p_diag__m_SOA_vt_d_1))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_15)];
                        double tmp_call_4_out;

                        ///////////////////
                        // Tasklet code (T_l455_c455)
                        tmp_call_4_out = (dace::math::ipow(p_diag_0_in_vt_0, 2));
                        ///////////////////

                        tmp_call_4 = tmp_call_4_out;
                    }
                    {
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * (1 - __CG_p_prog__m_SOA_vn_d_1))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_15)];
                        double tmp_call_3_out;

                        ///////////////////
                        // Tasklet code (T_l455_c455)
                        tmp_call_3_out = (dace::math::ipow(p_prog_0_in_vn_0, 2));
                        ///////////////////

                        tmp_call_3 = tmp_call_3_out;
                    }
                    {
                        double tmp_call_3_0_in = tmp_call_3;
                        double tmp_call_4_0_in = tmp_call_4;
                        double z_kin_hor_e_out_0;

                        ///////////////////
                        // Tasklet code (T_l455_c455)
                        z_kin_hor_e_out_0 = (0.5 * (tmp_call_3_0_in + tmp_call_4_0_in));
                        ///////////////////

                        z_kin_hor_e[(((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * ((- OA_z_kin_hor_e_d_2) + _for_it_6)) + (A_z_kin_hor_e_d_0 * (1 - OA_z_kin_hor_e_d_1))) - OA_z_kin_hor_e_d_0) + _for_it_15)] = z_kin_hor_e_out_0;
                    }
                    {
                        double p_metrics_0_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (1 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_15)];
                        double p_metrics_1_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (2 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_15)];
                        double p_metrics_2_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (3 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_15)];
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_299)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_15)];
                        double p_prog_1_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_305)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_15)];
                        double p_prog_2_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_311)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_15)];
                        double p_diag_out_vn_ie_0;

                        ///////////////////
                        // Tasklet code (T_l456_c456)
                        p_diag_out_vn_ie_0 = (((p_metrics_0_in_wgtfacq_e_0 * p_prog_0_in_vn_0) + (p_metrics_1_in_wgtfacq_e_0 * p_prog_1_in_vn_0)) + (p_metrics_2_in_wgtfacq_e_0 * p_prog_2_in_vn_0));
                        ///////////////////

                        __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_d_0 * tmp_index_293)) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_15)] = p_diag_out_vn_ie_0;
                    }

                }

            }
        } else {
            for (_for_it_16 = i_startidx_var_150; (_for_it_16 <= i_endidx_var_151); _for_it_16 = (_for_it_16 + 1)) {
                {

                    {
                        double dt_linintp_ubc_0_in = dt_linintp_ubc;
                        double p_diag_0_in_vn_ie_ubc_0 = __CG_p_diag__m_vn_ie_ubc[(((((__CG_p_diag__m_SA_vn_ie_ubc_d_0 * __CG_p_diag__m_SA_vn_ie_ubc_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_ubc_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_ubc_d_0 * (1 - __CG_p_diag__m_SOA_vn_ie_ubc_d_1))) - __CG_p_diag__m_SOA_vn_ie_ubc_d_0) + _for_it_16)];
                        double p_diag_1_in_vn_ie_ubc_0 = __CG_p_diag__m_vn_ie_ubc[(((((__CG_p_diag__m_SA_vn_ie_ubc_d_0 * __CG_p_diag__m_SA_vn_ie_ubc_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_ubc_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_ubc_d_0 * (2 - __CG_p_diag__m_SOA_vn_ie_ubc_d_1))) - __CG_p_diag__m_SOA_vn_ie_ubc_d_0) + _for_it_16)];
                        double p_diag_out_vn_ie_0;

                        ///////////////////
                        // Tasklet code (T_l460_c460)
                        p_diag_out_vn_ie_0 = (p_diag_0_in_vn_ie_ubc_0 + (dt_linintp_ubc_0_in * p_diag_1_in_vn_ie_ubc_0));
                        ///////////////////

                        __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_d_0 * (1 - __CG_p_diag__m_SOA_vn_ie_d_1))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_16)] = p_diag_out_vn_ie_0;
                    }
                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * (1 - __CG_p_diag__m_SOA_vt_d_1))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_16)];
                        double z_vt_ie_out_0;

                        ///////////////////
                        // Tasklet code (T_l461_c461)
                        z_vt_ie_out_0 = p_diag_0_in_vt_0;
                        ///////////////////

                        z_vt_ie[(((((A_z_vt_ie_d_0 * A_z_vt_ie_d_1) * ((- OA_z_vt_ie_d_2) + _for_it_6)) + (A_z_vt_ie_d_0 * (1 - OA_z_vt_ie_d_1))) - OA_z_vt_ie_d_0) + _for_it_16)] = z_vt_ie_out_0;
                    }

                }
                tmp_index_338 = (nlevp1_var_155 - __CG_p_diag__m_SOA_vn_ie_d_1);
                tmp_index_344 = (nlev_var_154 - __CG_p_prog__m_SOA_vn_d_1);
                tmp_index_350 = ((nlev_var_154 - 1) - __CG_p_prog__m_SOA_vn_d_1);
                tmp_index_356 = ((nlev_var_154 - 2) - __CG_p_prog__m_SOA_vn_d_1);
                {
                    double tmp_call_6;
                    double tmp_call_5;

                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vt_d_0 * (1 - __CG_p_diag__m_SOA_vt_d_1))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_16)];
                        double tmp_call_6_out;

                        ///////////////////
                        // Tasklet code (T_l462_c462)
                        tmp_call_6_out = (dace::math::ipow(p_diag_0_in_vt_0, 2));
                        ///////////////////

                        tmp_call_6 = tmp_call_6_out;
                    }
                    {
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * (1 - __CG_p_prog__m_SOA_vn_d_1))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_16)];
                        double tmp_call_5_out;

                        ///////////////////
                        // Tasklet code (T_l462_c462)
                        tmp_call_5_out = (dace::math::ipow(p_prog_0_in_vn_0, 2));
                        ///////////////////

                        tmp_call_5 = tmp_call_5_out;
                    }
                    {
                        double tmp_call_5_0_in = tmp_call_5;
                        double tmp_call_6_0_in = tmp_call_6;
                        double z_kin_hor_e_out_0;

                        ///////////////////
                        // Tasklet code (T_l462_c462)
                        z_kin_hor_e_out_0 = (0.5 * (tmp_call_5_0_in + tmp_call_6_0_in));
                        ///////////////////

                        z_kin_hor_e[(((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * ((- OA_z_kin_hor_e_d_2) + _for_it_6)) + (A_z_kin_hor_e_d_0 * (1 - OA_z_kin_hor_e_d_1))) - OA_z_kin_hor_e_d_0) + _for_it_16)] = z_kin_hor_e_out_0;
                    }
                    {
                        double p_metrics_0_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (1 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_16)];
                        double p_metrics_1_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (2 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_16)];
                        double p_metrics_2_in_wgtfacq_e_0 = __CG_p_metrics__m_wgtfacq_e[(((((__CG_p_metrics__m_SA_wgtfacq_e_d_0 * __CG_p_metrics__m_SA_wgtfacq_e_d_1) * ((- __CG_p_metrics__m_SOA_wgtfacq_e_d_2) + _for_it_6)) + (__CG_p_metrics__m_SA_wgtfacq_e_d_0 * (3 - __CG_p_metrics__m_SOA_wgtfacq_e_d_1))) - __CG_p_metrics__m_SOA_wgtfacq_e_d_0) + _for_it_16)];
                        double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_344)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_16)];
                        double p_prog_1_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_350)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_16)];
                        double p_prog_2_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_6)) + (__CG_p_prog__m_SA_vn_d_0 * tmp_index_356)) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_16)];
                        double p_diag_out_vn_ie_0;

                        ///////////////////
                        // Tasklet code (T_l463_c463)
                        p_diag_out_vn_ie_0 = (((p_metrics_0_in_wgtfacq_e_0 * p_prog_0_in_vn_0) + (p_metrics_1_in_wgtfacq_e_0 * p_prog_1_in_vn_0)) + (p_metrics_2_in_wgtfacq_e_0 * p_prog_2_in_vn_0));
                        ///////////////////

                        __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_6)) + (__CG_p_diag__m_SA_vn_ie_d_0 * tmp_index_338)) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_16)] = p_diag_out_vn_ie_0;
                    }

                }

            }
        }

    }
    i_startblk_var_148 = __CG_p_patch__CG_edges__m_start_block[(7 - SOA_start_block_d_0_edges_p_patch_4)];
    i_endblk_var_149 = __CG_p_patch__CG_edges__m_end_block[((- SOA_end_block_d_0_edges_p_patch_4) - 9)];
    for (_for_it_17 = i_startblk_var_148; (_for_it_17 <= i_endblk_var_149); _for_it_17 = (_for_it_17 + 1)) {

        i_startidx_in_var_93_2 = __CG_p_patch__CG_edges__m_start_index[(7 - SOA_start_index_d_0_edges_p_patch_4)];
        i_endidx_in_var_94_2 = __CG_p_patch__CG_edges__m_end_index[((- SOA_end_index_d_0_edges_p_patch_4) - 9)];
        if ((_for_it_17 != i_startblk_var_148)) {

            i_startidx_var_150 = 1;

        } else {

            i_startidx_var_150 = max(1, i_startidx_in_var_93_2);

        }
        if ((_for_it_17 != i_endblk_var_149)) {

            i_endidx_var_151 = __CG_global_data__m_nproma;

        } else {

            i_endidx_var_151 = i_endidx_in_var_94_2;

        }
        for (_for_it_18 = 1; (_for_it_18 <= nlev_var_154); _for_it_18 = (_for_it_18 + 1)) {
            for (_for_it_19 = i_startidx_var_150; (_for_it_19 <= i_endidx_var_151); _for_it_19 = (_for_it_19 + 1)) {

                tmp_index_374 = (__CG_p_patch__CG_edges__m_cell_idx[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_376 = (__CG_p_patch__CG_edges__m_cell_blk[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_383 = (__CG_p_patch__CG_edges__m_cell_idx[((((SA_cell_idx_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_17 - 1))) + _for_it_19) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                tmp_index_385 = (__CG_p_patch__CG_edges__m_cell_blk[((((SA_cell_blk_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_17 - 1))) + _for_it_19) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                tmp_index_399 = (__CG_p_patch__CG_edges__m_vertex_idx[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)] - 1);
                tmp_index_401 = (__CG_p_patch__CG_edges__m_vertex_blk[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)] - 1);
                tmp_index_408 = (__CG_p_patch__CG_edges__m_vertex_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_17 - 1))) + _for_it_19) - 1)] - 1);
                tmp_index_410 = (__CG_p_patch__CG_edges__m_vertex_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_17 - 1))) + _for_it_19) - 1)] - 1);
                {

                    {
                        double p_diag_0_in_vn_ie_0 = __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_17)) + (__CG_p_diag__m_SA_vn_ie_d_0 * ((- __CG_p_diag__m_SOA_vn_ie_d_1) + _for_it_18))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_19)];
                        double p_patch_0_in_edges_inv_dual_edge_length_0 = __CG_p_patch__CG_edges__m_inv_dual_edge_length[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)];
                        double p_patch_1_in_edges_inv_primal_edge_length_0 = __CG_p_patch__CG_edges__m_inv_primal_edge_length[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)];
                        double p_patch_2_in_edges_tangent_orientation_0 = __CG_p_patch__CG_edges__m_tangent_orientation[(((__CG_global_data__m_nproma * (_for_it_17 - 1)) + _for_it_19) - 1)];
                        double p_prog_0_in_w_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_376) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_18))) + tmp_index_374)];
                        double p_prog_1_in_w_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_385) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_18))) + tmp_index_383)];
                        double z_vt_ie_0_in_0 = z_vt_ie[(((((A_z_vt_ie_d_0 * A_z_vt_ie_d_1) * ((- OA_z_vt_ie_d_2) + _for_it_17)) + (A_z_vt_ie_d_0 * ((- OA_z_vt_ie_d_1) + _for_it_18))) - OA_z_vt_ie_d_0) + _for_it_19)];
                        double z_w_v_0_in_0 = z_w_v[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlevp1) * tmp_index_401) + (__CG_global_data__m_nproma * (_for_it_18 - 1))) + tmp_index_399)];
                        double z_w_v_1_in_0 = z_w_v[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlevp1) * tmp_index_410) + (__CG_global_data__m_nproma * (_for_it_18 - 1))) + tmp_index_408)];
                        double z_v_grad_w_out_0;

                        ///////////////////
                        // Tasklet code (T_l477_c477)
                        z_v_grad_w_out_0 = (((p_diag_0_in_vn_ie_0 * p_patch_0_in_edges_inv_dual_edge_length_0) * (p_prog_0_in_w_0 - p_prog_1_in_w_0)) + (((z_vt_ie_0_in_0 * p_patch_1_in_edges_inv_primal_edge_length_0) * p_patch_2_in_edges_tangent_orientation_0) * (z_w_v_0_in_0 - z_w_v_1_in_0)));
                        ///////////////////

                        z_v_grad_w[(((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (_for_it_17 - 1)) + (__CG_global_data__m_nproma * (_for_it_18 - 1))) + _for_it_19) - 1)] = z_v_grad_w_out_0;
                    }

                }

            }

        }

    }
    i_startblk_var_148 = __CG_p_patch__CG_cells__m_start_block[(4 - SOA_start_block_d_0_cells_p_patch_2)];
    i_endblk_var_149 = __CG_p_patch__CG_cells__m_end_block[((- SOA_end_block_d_0_cells_p_patch_2) - 5)];
    i_startblk_2 = __CG_p_patch__CG_cells__m_start_block[(5 - SOA_start_block_d_0_cells_p_patch_2)];
    i_endblk_2 = __CG_p_patch__CG_cells__m_end_block[((- SOA_end_block_d_0_cells_p_patch_2) - 4)];
    for (_for_it_23 = i_startblk_var_148; (_for_it_23 <= i_endblk_var_149); _for_it_23 = (_for_it_23 + 1)) {

        i_startidx_in_var_81_1 = __CG_p_patch__CG_cells__m_start_index[(4 - SOA_start_index_d_0_cells_p_patch_2)];
        i_endidx_in_var_82_1 = __CG_p_patch__CG_cells__m_end_index[((- SOA_end_index_d_0_cells_p_patch_2) - 5)];
        if (((_for_it_23 == i_startblk_var_148) == 1)) {

            i_startidx_var_150 = max(1, i_startidx_in_var_81_1);
            i_endidx_var_151 = __CG_global_data__m_nproma;
            if (((_for_it_23 == i_endblk_var_149) == 1)) {

                i_endidx_var_151 = i_endidx_in_var_82_1;

            }
        } else {
            if (((_for_it_23 == i_endblk_var_149) == 1)) {

                i_startidx_var_150 = 1;
                i_endidx_var_151 = i_endidx_in_var_82_1;

            } else {

                i_startidx_var_150 = 1;
                i_endidx_var_151 = __CG_global_data__m_nproma;

            }
        }
        for (_for_it_24 = 1; (_for_it_24 <= nlev_var_154); _for_it_24 = (_for_it_24 + 1)) {
            for (_for_it_25 = i_startidx_var_150; (_for_it_25 <= i_endidx_var_151); _for_it_25 = (_for_it_25 + 1)) {

                tmp_index_452 = (__CG_p_patch__CG_cells__m_edge_idx[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_0);
                tmp_index_454 = (__CG_p_patch__CG_cells__m_edge_blk[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_2);
                tmp_index_464 = (__CG_p_patch__CG_cells__m_edge_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_0);
                tmp_index_466 = (__CG_p_patch__CG_cells__m_edge_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_2);
                tmp_index_476 = (__CG_p_patch__CG_cells__m_edge_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_0);
                tmp_index_478 = (__CG_p_patch__CG_cells__m_edge_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_25) - 1)] - OA_z_kin_hor_e_d_2);
                {

                    {
                        double p_int_0_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (1 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_25)];
                        double p_int_1_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (2 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_25)];
                        double p_int_2_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (3 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_25)];
                        double z_kin_hor_e_0_in_0 = z_kin_hor_e[((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * tmp_index_454) + (A_z_kin_hor_e_d_0 * ((- OA_z_kin_hor_e_d_1) + _for_it_24))) + tmp_index_452)];
                        double z_kin_hor_e_1_in_0 = z_kin_hor_e[((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * tmp_index_466) + (A_z_kin_hor_e_d_0 * ((- OA_z_kin_hor_e_d_1) + _for_it_24))) + tmp_index_464)];
                        double z_kin_hor_e_2_in_0 = z_kin_hor_e[((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * tmp_index_478) + (A_z_kin_hor_e_d_0 * ((- OA_z_kin_hor_e_d_1) + _for_it_24))) + tmp_index_476)];
                        double z_ekinh_out_0;

                        ///////////////////
                        // Tasklet code (T_l504_c504)
                        z_ekinh_out_0 = (((p_int_0_in_e_bln_c_s_0 * z_kin_hor_e_0_in_0) + (p_int_1_in_e_bln_c_s_0 * z_kin_hor_e_1_in_0)) + (p_int_2_in_e_bln_c_s_0 * z_kin_hor_e_2_in_0));
                        ///////////////////

                        z_ekinh[(((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (_for_it_23 - 1)) + (__CG_global_data__m_nproma * (_for_it_24 - 1))) + _for_it_25) - 1)] = z_ekinh_out_0;
                    }

                }

            }

        }
        for (_for_it_26 = nflatlev_jg; (_for_it_26 <= nlev_var_154); _for_it_26 = (_for_it_26 + 1)) {
            for (_for_it_27 = i_startidx_var_150; (_for_it_27 <= i_endidx_var_151); _for_it_27 = (_for_it_27 + 1)) {

                tmp_index_490 = (__CG_p_patch__CG_cells__m_edge_idx[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_0);
                tmp_index_492 = (__CG_p_patch__CG_cells__m_edge_blk[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_2);
                tmp_index_502 = (__CG_p_patch__CG_cells__m_edge_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_0);
                tmp_index_504 = (__CG_p_patch__CG_cells__m_edge_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_2);
                tmp_index_514 = (__CG_p_patch__CG_cells__m_edge_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_0);
                tmp_index_516 = (__CG_p_patch__CG_cells__m_edge_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_27) - 1)] - OA_z_w_concorr_me_d_2);
                {

                    {
                        double p_int_0_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (1 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_27)];
                        double p_int_1_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (2 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_27)];
                        double p_int_2_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (3 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_27)];
                        double z_w_concorr_me_0_in_0 = z_w_concorr_me[((((A_z_w_concorr_me_d_0 * A_z_w_concorr_me_d_1) * tmp_index_492) + (A_z_w_concorr_me_d_0 * ((- OA_z_w_concorr_me_d_1) + _for_it_26))) + tmp_index_490)];
                        double z_w_concorr_me_1_in_0 = z_w_concorr_me[((((A_z_w_concorr_me_d_0 * A_z_w_concorr_me_d_1) * tmp_index_504) + (A_z_w_concorr_me_d_0 * ((- OA_z_w_concorr_me_d_1) + _for_it_26))) + tmp_index_502)];
                        double z_w_concorr_me_2_in_0 = z_w_concorr_me[((((A_z_w_concorr_me_d_0 * A_z_w_concorr_me_d_1) * tmp_index_516) + (A_z_w_concorr_me_d_0 * ((- OA_z_w_concorr_me_d_1) + _for_it_26))) + tmp_index_514)];
                        double z_w_concorr_mc_out_0;

                        ///////////////////
                        // Tasklet code (T_l510_c510)
                        z_w_concorr_mc_out_0 = (((p_int_0_in_e_bln_c_s_0 * z_w_concorr_me_0_in_0) + (p_int_1_in_e_bln_c_s_0 * z_w_concorr_me_1_in_0)) + (p_int_2_in_e_bln_c_s_0 * z_w_concorr_me_2_in_0));
                        ///////////////////

                        z_w_concorr_mc[(((__CG_global_data__m_nproma * (_for_it_26 - 1)) + _for_it_27) - 1)] = z_w_concorr_mc_out_0;
                    }

                }

            }

        }
        for (_for_it_28 = (nflatlev_jg + 1); (_for_it_28 <= nlev_var_154); _for_it_28 = (_for_it_28 + 1)) {
            for (_for_it_29 = i_startidx_var_150; (_for_it_29 <= i_endidx_var_151); _for_it_29 = (_for_it_29 + 1)) {
                {

                    {
                        double p_metrics_0_in_wgtfac_c_0 = __CG_p_metrics__m_wgtfac_c[(((((__CG_p_metrics__m_SA_wgtfac_c_d_0 * __CG_p_metrics__m_SA_wgtfac_c_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_c_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_wgtfac_c_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_c_d_1) + _for_it_28))) - __CG_p_metrics__m_SOA_wgtfac_c_d_0) + _for_it_29)];
                        double p_metrics_1_in_wgtfac_c_0 = __CG_p_metrics__m_wgtfac_c[(((((__CG_p_metrics__m_SA_wgtfac_c_d_0 * __CG_p_metrics__m_SA_wgtfac_c_d_1) * ((- __CG_p_metrics__m_SOA_wgtfac_c_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_wgtfac_c_d_0 * ((- __CG_p_metrics__m_SOA_wgtfac_c_d_1) + _for_it_28))) - __CG_p_metrics__m_SOA_wgtfac_c_d_0) + _for_it_29)];
                        double z_w_concorr_mc_0_in_0 = z_w_concorr_mc[(((__CG_global_data__m_nproma * (_for_it_28 - 1)) + _for_it_29) - 1)];
                        double z_w_concorr_mc_1_in_0 = z_w_concorr_mc[(((__CG_global_data__m_nproma * (_for_it_28 - 2)) + _for_it_29) - 1)];
                        double p_diag_out_w_concorr_c_0;

                        ///////////////////
                        // Tasklet code (T_l515_c515)
                        p_diag_out_w_concorr_c_0 = ((p_metrics_0_in_wgtfac_c_0 * z_w_concorr_mc_0_in_0) + ((1.0 - p_metrics_1_in_wgtfac_c_0) * z_w_concorr_mc_1_in_0));
                        ///////////////////

                        __CG_p_diag__m_w_concorr_c[(((((__CG_p_diag__m_SA_w_concorr_c_d_0 * __CG_p_diag__m_SA_w_concorr_c_d_1) * ((- __CG_p_diag__m_SOA_w_concorr_c_d_2) + _for_it_23)) + (__CG_p_diag__m_SA_w_concorr_c_d_0 * ((- __CG_p_diag__m_SOA_w_concorr_c_d_1) + _for_it_28))) - __CG_p_diag__m_SOA_w_concorr_c_d_0) + _for_it_29)] = p_diag_out_w_concorr_c_0;
                    }

                }

            }

        }
        for (_for_it_30 = 1; (_for_it_30 <= nlev_var_154); _for_it_30 = (_for_it_30 + 1)) {
            for (_for_it_31 = i_startidx_var_150; (_for_it_31 <= i_endidx_var_151); _for_it_31 = (_for_it_31 + 1)) {
                {

                    {
                        double p_prog_0_in_w_0 = __CG_p_prog__m_w[(((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * ((- __CG_p_prog__m_SOA_w_d_2) + _for_it_23)) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_30))) - __CG_p_prog__m_SOA_w_d_0) + _for_it_31)];
                        double z_w_con_c_out_0;

                        ///////////////////
                        // Tasklet code (T_l521_c521)
                        z_w_con_c_out_0 = p_prog_0_in_w_0;
                        ///////////////////

                        z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_30 - 1)) + _for_it_31) - 1)] = z_w_con_c_out_0;
                    }

                }

            }

        }
        for (_for_it_32 = i_startidx_var_150; (_for_it_32 <= i_endidx_var_151); _for_it_32 = (_for_it_32 + 1)) {

            tmp_index_536 = (nlevp1_var_155 - 1);
            {

                {
                    double z_w_con_c_out_0;

                    ///////////////////
                    // Tasklet code (T_l525_c525)
                    z_w_con_c_out_0 = 0.0;
                    ///////////////////

                    z_w_con_c[(((__CG_global_data__m_nproma * tmp_index_536) + _for_it_32) - 1)] = z_w_con_c_out_0;
                }

            }

        }
        for (_for_it_33 = nlev_var_154; (_for_it_33 >= (nflatlev_jg + 1)); _for_it_33 = (_for_it_33 + -1)) {
            for (_for_it_34 = i_startidx_var_150; (_for_it_34 <= i_endidx_var_151); _for_it_34 = (_for_it_34 + 1)) {
                {

                    {
                        double p_diag_0_in_w_concorr_c_0 = __CG_p_diag__m_w_concorr_c[(((((__CG_p_diag__m_SA_w_concorr_c_d_0 * __CG_p_diag__m_SA_w_concorr_c_d_1) * ((- __CG_p_diag__m_SOA_w_concorr_c_d_2) + _for_it_23)) + (__CG_p_diag__m_SA_w_concorr_c_d_0 * ((- __CG_p_diag__m_SOA_w_concorr_c_d_1) + _for_it_33))) - __CG_p_diag__m_SOA_w_concorr_c_d_0) + _for_it_34)];
                        double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_33 - 1)) + _for_it_34) - 1)];
                        double z_w_con_c_out_0;

                        ///////////////////
                        // Tasklet code (T_l529_c529)
                        z_w_con_c_out_0 = (z_w_con_c_0_in_0 - p_diag_0_in_w_concorr_c_0);
                        ///////////////////

                        z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_33 - 1)) + _for_it_34) - 1)] = z_w_con_c_out_0;
                    }

                }

            }

        }
        tmp_arg_9 = (nrdmax_jg - 2);
        for (_for_it_35 = max(3, tmp_arg_9); (_for_it_35 <= (nlev_var_154 - 3)); _for_it_35 = (_for_it_35 + 1)) {
            {

                {
                    int levmask_out_0;

                    ///////////////////
                    // Tasklet code (T_l533_c533)
                    levmask_out_0 = 0;
                    ///////////////////

                    levmask[(((__CG_p_patch__m_nblks_c * (_for_it_35 - 1)) + _for_it_23) - 1)] = levmask_out_0;
                }

            }

        }
        tmp_arg_10 = (nrdmax_jg - 2);
        {

            {
                double maxvcfl_out;

                ///////////////////
                // Tasklet code (T_l535_c535)
                maxvcfl_out = 0;
                ///////////////////

                maxvcfl = maxvcfl_out;
            }

        }
        for (_for_it_36 = max(3, tmp_arg_10); (_for_it_36 <= (nlev_var_154 - 3)); _for_it_36 = (_for_it_36 + 1)) {

            clip_count = 0;
            for (_for_it_37 = i_startidx_var_150; (_for_it_37 <= i_endidx_var_151); _for_it_37 = (_for_it_37 + 1)) {
                {
                    double tmp_call_9;

                    {
                        double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_37) - 1)];
                        double tmp_call_9_out;

                        ///////////////////
                        // Tasklet code (T_l539_c539)
                        tmp_call_9_out = abs(z_w_con_c_0_in_0);
                        ///////////////////

                        tmp_call_9 = tmp_call_9_out;
                    }
                    {
                        double cfl_w_limit_0_in = cfl_w_limit;
                        double p_metrics_0_in_ddqz_z_half_0 = __CG_p_metrics__m_ddqz_z_half[(((((__CG_p_metrics__m_SA_ddqz_z_half_d_0 * __CG_p_metrics__m_SA_ddqz_z_half_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_ddqz_z_half_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + _for_it_36))) - __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + _for_it_37)];
                        double tmp_call_9_0_in = tmp_call_9;
                        int cfl_clipping_out_0;

                        ///////////////////
                        // Tasklet code (T_l539_c539)
                        cfl_clipping_out_0 = (tmp_call_9_0_in > (cfl_w_limit_0_in * p_metrics_0_in_ddqz_z_half_0));
                        ///////////////////

                        cfl_clipping[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_37) - 1)] = cfl_clipping_out_0;
                    }
                    {
                        int cfl_clipping_0_in_0 = cfl_clipping[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_37) - 1)];
                        int _if_cond_18_out;

                        ///////////////////
                        // Tasklet code (T_l540_c540)
                        _if_cond_18_out = cfl_clipping_0_in_0;
                        ///////////////////

                        _if_cond_18 = _if_cond_18_out;
                    }

                }
                if ((_if_cond_18 == 1)) {

                    clip_count = (clip_count + 1);

                }

            }
            if (((clip_count == 0) == 1)) {
                continue;
            }
            for (_for_it_38 = i_startidx_var_150; (_for_it_38 <= i_endidx_var_151); _for_it_38 = (_for_it_38 + 1)) {

                _if_cond_20 = cfl_clipping[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_38) - 1)];
                if ((_if_cond_20 == 1)) {
                    {
                        double tmp_call_10;

                        {
                            int levmask_out_0;

                            ///////////////////
                            // Tasklet code (T_l545_c545)
                            levmask_out_0 = 1;
                            ///////////////////

                            levmask[(((__CG_p_patch__m_nblks_c * (_for_it_36 - 1)) + _for_it_23) - 1)] = levmask_out_0;
                        }
                        {
                            double dtime_0_in = dtime;
                            double p_metrics_0_in_ddqz_z_half_0 = __CG_p_metrics__m_ddqz_z_half[(((((__CG_p_metrics__m_SA_ddqz_z_half_d_0 * __CG_p_metrics__m_SA_ddqz_z_half_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_ddqz_z_half_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + _for_it_36))) - __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + _for_it_38)];
                            double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_38) - 1)];
                            double vcfl_out;

                            ///////////////////
                            // Tasklet code (T_l546_c546)
                            vcfl_out = ((z_w_con_c_0_in_0 * dtime_0_in) / p_metrics_0_in_ddqz_z_half_0);
                            ///////////////////

                            vcfl = vcfl_out;
                        }
                        {
                            double vcfl_0_in = vcfl;
                            double tmp_call_10_out;

                            ///////////////////
                            // Tasklet code (T_l547_c547)
                            tmp_call_10_out = abs(vcfl_0_in);
                            ///////////////////

                            tmp_call_10 = tmp_call_10_out;
                        }
                        {
                            double maxvcfl_0_in = maxvcfl;
                            double tmp_call_10_0_in = tmp_call_10;
                            double maxvcfl_out;

                            ///////////////////
                            // Tasklet code (T_l547_c547)
                            maxvcfl_out = max(maxvcfl_0_in, tmp_call_10_0_in);
                            ///////////////////

                            maxvcfl = maxvcfl_out;
                        }

                    }
                    _if_cond_21 = (vcfl < -0.85);
                    if ((_if_cond_21 == 1)) {
                        {

                            {
                                double dtime_0_in = dtime;
                                double p_metrics_0_in_ddqz_z_half_0 = __CG_p_metrics__m_ddqz_z_half[(((((__CG_p_metrics__m_SA_ddqz_z_half_d_0 * __CG_p_metrics__m_SA_ddqz_z_half_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_ddqz_z_half_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + _for_it_36))) - __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + _for_it_38)];
                                double z_w_con_c_out_0;

                                ///////////////////
                                // Tasklet code (T_l549_c549)
                                z_w_con_c_out_0 = (- ((0.85 * p_metrics_0_in_ddqz_z_half_0) / dtime_0_in));
                                ///////////////////

                                z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_38) - 1)] = z_w_con_c_out_0;
                            }

                        }
                    } else {

                        _if_cond_22 = (vcfl > 0.85);
                        if ((_if_cond_22 == 1)) {
                            {

                                {
                                    double dtime_0_in = dtime;
                                    double p_metrics_0_in_ddqz_z_half_0 = __CG_p_metrics__m_ddqz_z_half[(((((__CG_p_metrics__m_SA_ddqz_z_half_d_0 * __CG_p_metrics__m_SA_ddqz_z_half_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_ddqz_z_half_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + _for_it_36))) - __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + _for_it_38)];
                                    double z_w_con_c_out_0;

                                    ///////////////////
                                    // Tasklet code (T_l551_c551)
                                    z_w_con_c_out_0 = ((0.85 * p_metrics_0_in_ddqz_z_half_0) / dtime_0_in);
                                    ///////////////////

                                    z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_36 - 1)) + _for_it_38) - 1)] = z_w_con_c_out_0;
                                }

                            }
                        }
                    }
                }

            }

        }
        for (_for_it_39 = 1; (_for_it_39 <= nlev_var_154); _for_it_39 = (_for_it_39 + 1)) {
            for (_for_it_40 = i_startidx_var_150; (_for_it_40 <= i_endidx_var_151); _for_it_40 = (_for_it_40 + 1)) {
                {

                    {
                        double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_39 - 1)) + _for_it_40) - 1)];
                        double z_w_con_c_1_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * _for_it_39) + _for_it_40) - 1)];
                        double z_w_con_c_full_out_0;

                        ///////////////////
                        // Tasklet code (T_l558_c558)
                        z_w_con_c_full_out_0 = (0.5 * (z_w_con_c_0_in_0 + z_w_con_c_1_in_0));
                        ///////////////////

                        z_w_con_c_full[(((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * (_for_it_23 - 1)) + (__CG_global_data__m_nproma * (_for_it_39 - 1))) + _for_it_40) - 1)] = z_w_con_c_full_out_0;
                    }

                }

            }

        }
        {

            {
                double maxvcfl_0_in = maxvcfl;
                double vcflmax_out_0;

                ///////////////////
                // Tasklet code (T_l561_c561)
                vcflmax_out_0 = maxvcfl_0_in;
                ///////////////////

                vcflmax[(_for_it_23 - 1)] = vcflmax_out_0;
            }

        }
        if ((((_for_it_23 < i_startblk_2) || (_for_it_23 > i_endblk_2)) == 1)) {
            continue;
        }
        i_startidx_in_var_81_0 = __CG_p_patch__CG_cells__m_start_index[(5 - SOA_start_index_d_0_cells_p_patch_2)];
        i_endidx_in_var_82_0 = __CG_p_patch__CG_cells__m_end_index[((- SOA_end_index_d_0_cells_p_patch_2) - 4)];
        if (((_for_it_23 == i_startblk_2) == 1)) {

            i_startidx_2 = max(1, i_startidx_in_var_81_0);
            i_endidx_2 = __CG_global_data__m_nproma;
            if (((_for_it_23 == i_endblk_2) == 1)) {

                i_endidx_2 = i_endidx_in_var_82_0;

            }
        } else {
            if (((_for_it_23 == i_endblk_2) == 1)) {

                i_startidx_2 = 1;
                i_endidx_2 = i_endidx_in_var_82_0;

            } else {

                i_startidx_2 = 1;
                i_endidx_2 = __CG_global_data__m_nproma;

            }
        }
        for (_for_it_41 = 2; (_for_it_41 <= nlev_var_154); _for_it_41 = (_for_it_41 + 1)) {
            for (_for_it_42 = i_startidx_2; (_for_it_42 <= i_endidx_2); _for_it_42 = (_for_it_42 + 1)) {

                tmp_index_585 = (ntnd - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3);
                {

                    {
                        double p_metrics_0_in_coeff1_dwdz_0 = __CG_p_metrics__m_coeff1_dwdz[(((((__CG_p_metrics__m_SA_coeff1_dwdz_d_0 * __CG_p_metrics__m_SA_coeff1_dwdz_d_1) * ((- __CG_p_metrics__m_SOA_coeff1_dwdz_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_coeff1_dwdz_d_0 * ((- __CG_p_metrics__m_SOA_coeff1_dwdz_d_1) + _for_it_41))) - __CG_p_metrics__m_SOA_coeff1_dwdz_d_0) + _for_it_42)];
                        double p_metrics_1_in_coeff2_dwdz_0 = __CG_p_metrics__m_coeff2_dwdz[(((((__CG_p_metrics__m_SA_coeff2_dwdz_d_0 * __CG_p_metrics__m_SA_coeff2_dwdz_d_1) * ((- __CG_p_metrics__m_SOA_coeff2_dwdz_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_coeff2_dwdz_d_0 * ((- __CG_p_metrics__m_SOA_coeff2_dwdz_d_1) + _for_it_41))) - __CG_p_metrics__m_SOA_coeff2_dwdz_d_0) + _for_it_42)];
                        double p_metrics_2_in_coeff2_dwdz_0 = __CG_p_metrics__m_coeff2_dwdz[(((((__CG_p_metrics__m_SA_coeff2_dwdz_d_0 * __CG_p_metrics__m_SA_coeff2_dwdz_d_1) * ((- __CG_p_metrics__m_SOA_coeff2_dwdz_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_coeff2_dwdz_d_0 * ((- __CG_p_metrics__m_SOA_coeff2_dwdz_d_1) + _for_it_41))) - __CG_p_metrics__m_SOA_coeff2_dwdz_d_0) + _for_it_42)];
                        double p_metrics_3_in_coeff1_dwdz_0 = __CG_p_metrics__m_coeff1_dwdz[(((((__CG_p_metrics__m_SA_coeff1_dwdz_d_0 * __CG_p_metrics__m_SA_coeff1_dwdz_d_1) * ((- __CG_p_metrics__m_SOA_coeff1_dwdz_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_coeff1_dwdz_d_0 * ((- __CG_p_metrics__m_SOA_coeff1_dwdz_d_1) + _for_it_41))) - __CG_p_metrics__m_SOA_coeff1_dwdz_d_0) + _for_it_42)];
                        double p_prog_0_in_w_0 = __CG_p_prog__m_w[(((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * ((- __CG_p_prog__m_SOA_w_d_2) + _for_it_23)) + (__CG_p_prog__m_SA_w_d_0 * (((- __CG_p_prog__m_SOA_w_d_1) + _for_it_41) - 1))) - __CG_p_prog__m_SOA_w_d_0) + _for_it_42)];
                        double p_prog_1_in_w_0 = __CG_p_prog__m_w[(((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * ((- __CG_p_prog__m_SOA_w_d_2) + _for_it_23)) + (__CG_p_prog__m_SA_w_d_0 * (((- __CG_p_prog__m_SOA_w_d_1) + _for_it_41) + 1))) - __CG_p_prog__m_SOA_w_d_0) + _for_it_42)];
                        double p_prog_2_in_w_0 = __CG_p_prog__m_w[(((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * ((- __CG_p_prog__m_SOA_w_d_2) + _for_it_23)) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_41))) - __CG_p_prog__m_SOA_w_d_0) + _for_it_42)];
                        double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_41 - 1)) + _for_it_42) - 1)];
                        double p_diag_out_ddt_w_adv_pc_0;

                        ///////////////////
                        // Tasklet code (T_l567_c567)
                        p_diag_out_ddt_w_adv_pc_0 = (- (z_w_con_c_0_in_0 * (((p_prog_0_in_w_0 * p_metrics_0_in_coeff1_dwdz_0) - (p_prog_1_in_w_0 * p_metrics_1_in_coeff2_dwdz_0)) + (p_prog_2_in_w_0 * (p_metrics_2_in_coeff2_dwdz_0 - p_metrics_3_in_coeff1_dwdz_0)))));
                        ///////////////////

                        __CG_p_diag__m_ddt_w_adv_pc[(((((((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) * tmp_index_585) + ((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + _for_it_23))) + (__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + _for_it_41))) - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + _for_it_42)] = p_diag_out_ddt_w_adv_pc_0;
                    }

                }

            }

        }
        for (_for_it_43 = 2; (_for_it_43 <= nlev_var_154); _for_it_43 = (_for_it_43 + 1)) {
            for (_for_it_44 = i_startidx_2; (_for_it_44 <= i_endidx_2); _for_it_44 = (_for_it_44 + 1)) {

                tmp_index_612 = (ntnd - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3);
                tmp_index_616 = (ntnd - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3);
                tmp_index_626 = (__CG_p_patch__CG_cells__m_edge_idx[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_44) - 1)] - 1);
                tmp_index_628 = (__CG_p_patch__CG_cells__m_edge_blk[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_44) - 1)] - 1);
                tmp_index_638 = (__CG_p_patch__CG_cells__m_edge_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_44) - 1)] - 1);
                tmp_index_640 = (__CG_p_patch__CG_cells__m_edge_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_44) - 1)] - 1);
                tmp_index_650 = (__CG_p_patch__CG_cells__m_edge_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_44) - 1)] - 1);
                tmp_index_652 = (__CG_p_patch__CG_cells__m_edge_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_44) - 1)] - 1);
                {

                    {
                        double p_diag_0_in_ddt_w_adv_pc_0 = __CG_p_diag__m_ddt_w_adv_pc[(((((((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) * tmp_index_616) + ((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + _for_it_23))) + (__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + _for_it_43))) - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + _for_it_44)];
                        double p_int_0_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (1 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_44)];
                        double p_int_1_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (2 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_44)];
                        double p_int_2_in_e_bln_c_s_0 = __CG_p_int__m_e_bln_c_s[(((((__CG_p_int__m_SA_e_bln_c_s_d_0 * __CG_p_int__m_SA_e_bln_c_s_d_1) * ((- __CG_p_int__m_SOA_e_bln_c_s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_e_bln_c_s_d_0 * (3 - __CG_p_int__m_SOA_e_bln_c_s_d_1))) - __CG_p_int__m_SOA_e_bln_c_s_d_0) + _for_it_44)];
                        double z_v_grad_w_0_in_0 = z_v_grad_w[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_628) + (__CG_global_data__m_nproma * (_for_it_43 - 1))) + tmp_index_626)];
                        double z_v_grad_w_1_in_0 = z_v_grad_w[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_640) + (__CG_global_data__m_nproma * (_for_it_43 - 1))) + tmp_index_638)];
                        double z_v_grad_w_2_in_0 = z_v_grad_w[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_652) + (__CG_global_data__m_nproma * (_for_it_43 - 1))) + tmp_index_650)];
                        double p_diag_out_ddt_w_adv_pc_0;

                        ///////////////////
                        // Tasklet code (T_l572_c572)
                        p_diag_out_ddt_w_adv_pc_0 = (((p_diag_0_in_ddt_w_adv_pc_0 + (p_int_0_in_e_bln_c_s_0 * z_v_grad_w_0_in_0)) + (p_int_1_in_e_bln_c_s_0 * z_v_grad_w_1_in_0)) + (p_int_2_in_e_bln_c_s_0 * z_v_grad_w_2_in_0));
                        ///////////////////

                        __CG_p_diag__m_ddt_w_adv_pc[(((((((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) * tmp_index_612) + ((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + _for_it_23))) + (__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + _for_it_43))) - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + _for_it_44)] = p_diag_out_ddt_w_adv_pc_0;
                    }

                }

            }

        }
        tmp_arg_12 = (nrdmax_jg - 2);
        for (_for_it_45 = max(3, tmp_arg_12); (_for_it_45 <= (nlev_var_154 - 3)); _for_it_45 = (_for_it_45 + 1)) {

            _if_cond_26 = levmask[(((__CG_p_patch__m_nblks_c * (_for_it_45 - 1)) + _for_it_23) - 1)];
            if ((_if_cond_26 == 1)) {
                for (_for_it_46 = i_startidx_2; (_for_it_46 <= i_endidx_2); _for_it_46 = (_for_it_46 + 1)) {

                    _if_cond_27 = (cfl_clipping[(((__CG_global_data__m_nproma * (_for_it_45 - 1)) + _for_it_46) - 1)] && __CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_46) - 1)]);
                    if ((_if_cond_27 == 1)) {

                        tmp_index_667 = (ntnd - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3);
                        tmp_index_671 = (ntnd - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3);
                        tmp_index_686 = (__CG_p_patch__CG_cells__m_neighbor_idx[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                        tmp_index_688 = (__CG_p_patch__CG_cells__m_neighbor_blk[(((__CG_global_data__m_nproma * (_for_it_23 - 1)) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                        tmp_index_698 = (__CG_p_patch__CG_cells__m_neighbor_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                        tmp_index_700 = (__CG_p_patch__CG_cells__m_neighbor_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                        tmp_index_710 = (__CG_p_patch__CG_cells__m_neighbor_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_0);
                        tmp_index_712 = (__CG_p_patch__CG_cells__m_neighbor_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_c) + (__CG_global_data__m_nproma * (_for_it_23 - 1))) + _for_it_46) - 1)] - __CG_p_prog__m_SOA_w_d_2);
                        {
                            double tmp_call_12;
                            double tmp_call_13;
                            double tmp_arg_13;
                            double tmp_arg_14;

                            {
                                double z_w_con_c_0_in_0 = z_w_con_c[(((__CG_global_data__m_nproma * (_for_it_45 - 1)) + _for_it_46) - 1)];
                                double tmp_call_13_out;

                                ///////////////////
                                // Tasklet code (T_l580_c580)
                                tmp_call_13_out = abs(z_w_con_c_0_in_0);
                                ///////////////////

                                tmp_call_13 = tmp_call_13_out;
                            }
                            {
                                double cfl_w_limit_0_in = cfl_w_limit;
                                double dtime_0_in = dtime;
                                double tmp_arg_13_out;

                                ///////////////////
                                // Tasklet code (T_l580_c580)
                                tmp_arg_13_out = (0.85 - (cfl_w_limit_0_in * dtime_0_in));
                                ///////////////////

                                tmp_arg_13 = tmp_arg_13_out;
                            }
                            {
                                double cfl_w_limit_0_in = cfl_w_limit;
                                double dtime_0_in = dtime;
                                double dtime_1_in = dtime;
                                double p_metrics_0_in_ddqz_z_half_0 = __CG_p_metrics__m_ddqz_z_half[(((((__CG_p_metrics__m_SA_ddqz_z_half_d_0 * __CG_p_metrics__m_SA_ddqz_z_half_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_2) + _for_it_23)) + (__CG_p_metrics__m_SA_ddqz_z_half_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_half_d_1) + _for_it_45))) - __CG_p_metrics__m_SOA_ddqz_z_half_d_0) + _for_it_46)];
                                double tmp_call_13_0_in = tmp_call_13;
                                double tmp_arg_14_out;

                                ///////////////////
                                // Tasklet code (T_l580_c580)
                                tmp_arg_14_out = (((tmp_call_13_0_in * dtime_0_in) / p_metrics_0_in_ddqz_z_half_0) - (cfl_w_limit_0_in * dtime_1_in));
                                ///////////////////

                                tmp_arg_14 = tmp_arg_14_out;
                            }
                            {
                                double tmp_arg_13_0_in = tmp_arg_13;
                                double tmp_arg_14_0_in = tmp_arg_14;
                                double tmp_call_12_out;

                                ///////////////////
                                // Tasklet code (T_l580_c580)
                                tmp_call_12_out = min(tmp_arg_13_0_in, tmp_arg_14_0_in);
                                ///////////////////

                                tmp_call_12 = tmp_call_12_out;
                            }
                            {
                                double scalfac_exdiff_0_in = scalfac_exdiff;
                                double tmp_call_12_0_in = tmp_call_12;
                                double difcoef_out;

                                ///////////////////
                                // Tasklet code (T_l580_c580)
                                difcoef_out = (scalfac_exdiff_0_in * tmp_call_12_0_in);
                                ///////////////////

                                difcoef = difcoef_out;
                            }
                            {
                                double difcoef_0_in = difcoef;
                                double p_diag_0_in_ddt_w_adv_pc_0 = __CG_p_diag__m_ddt_w_adv_pc[(((((((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) * tmp_index_671) + ((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + _for_it_23))) + (__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + _for_it_45))) - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + _for_it_46)];
                                double p_int_0_in_geofac_n2s_0 = __CG_p_int__m_geofac_n2s[(((((__CG_p_int__m_SA_geofac_n2s_d_0 * __CG_p_int__m_SA_geofac_n2s_d_1) * ((- __CG_p_int__m_SOA_geofac_n2s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_geofac_n2s_d_0 * (1 - __CG_p_int__m_SOA_geofac_n2s_d_1))) - __CG_p_int__m_SOA_geofac_n2s_d_0) + _for_it_46)];
                                double p_int_1_in_geofac_n2s_0 = __CG_p_int__m_geofac_n2s[(((((__CG_p_int__m_SA_geofac_n2s_d_0 * __CG_p_int__m_SA_geofac_n2s_d_1) * ((- __CG_p_int__m_SOA_geofac_n2s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_geofac_n2s_d_0 * (2 - __CG_p_int__m_SOA_geofac_n2s_d_1))) - __CG_p_int__m_SOA_geofac_n2s_d_0) + _for_it_46)];
                                double p_int_2_in_geofac_n2s_0 = __CG_p_int__m_geofac_n2s[(((((__CG_p_int__m_SA_geofac_n2s_d_0 * __CG_p_int__m_SA_geofac_n2s_d_1) * ((- __CG_p_int__m_SOA_geofac_n2s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_geofac_n2s_d_0 * (3 - __CG_p_int__m_SOA_geofac_n2s_d_1))) - __CG_p_int__m_SOA_geofac_n2s_d_0) + _for_it_46)];
                                double p_int_3_in_geofac_n2s_0 = __CG_p_int__m_geofac_n2s[(((((__CG_p_int__m_SA_geofac_n2s_d_0 * __CG_p_int__m_SA_geofac_n2s_d_1) * ((- __CG_p_int__m_SOA_geofac_n2s_d_2) + _for_it_23)) + (__CG_p_int__m_SA_geofac_n2s_d_0 * (4 - __CG_p_int__m_SOA_geofac_n2s_d_1))) - __CG_p_int__m_SOA_geofac_n2s_d_0) + _for_it_46)];
                                double p_patch_0_in_cells_area_0 = __CG_p_patch__CG_cells__m_area[(((SA_area_d_0_cells_p_patch_2 * ((- SOA_area_d_1_cells_p_patch_2) + _for_it_23)) - SOA_area_d_0_cells_p_patch_2) + _for_it_46)];
                                double p_prog_0_in_w_0 = __CG_p_prog__m_w[(((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * ((- __CG_p_prog__m_SOA_w_d_2) + _for_it_23)) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_45))) - __CG_p_prog__m_SOA_w_d_0) + _for_it_46)];
                                double p_prog_1_in_w_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_688) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_45))) + tmp_index_686)];
                                double p_prog_2_in_w_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_700) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_45))) + tmp_index_698)];
                                double p_prog_3_in_w_0 = __CG_p_prog__m_w[((((__CG_p_prog__m_SA_w_d_0 * __CG_p_prog__m_SA_w_d_1) * tmp_index_712) + (__CG_p_prog__m_SA_w_d_0 * ((- __CG_p_prog__m_SOA_w_d_1) + _for_it_45))) + tmp_index_710)];
                                double p_diag_out_ddt_w_adv_pc_0;

                                ///////////////////
                                // Tasklet code (T_l581_c581)
                                p_diag_out_ddt_w_adv_pc_0 = (p_diag_0_in_ddt_w_adv_pc_0 + ((difcoef_0_in * p_patch_0_in_cells_area_0) * ((((p_prog_0_in_w_0 * p_int_0_in_geofac_n2s_0) + (p_prog_1_in_w_0 * p_int_1_in_geofac_n2s_0)) + (p_prog_2_in_w_0 * p_int_2_in_geofac_n2s_0)) + (p_prog_3_in_w_0 * p_int_3_in_geofac_n2s_0))));
                                ///////////////////

                                __CG_p_diag__m_ddt_w_adv_pc[(((((((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * __CG_p_diag__m_SA_ddt_w_adv_pc_d_2) * tmp_index_667) + ((__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * __CG_p_diag__m_SA_ddt_w_adv_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2) + _for_it_23))) + (__CG_p_diag__m_SA_ddt_w_adv_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1) + _for_it_45))) - __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0) + _for_it_46)] = p_diag_out_ddt_w_adv_pc_0;
                            }

                        }
                    }

                }
            }

        }

    }
    tmp_arg_15 = (nrdmax_jg - 2);
    for (_for_it_47 = max(3, tmp_arg_15); (_for_it_47 <= (nlev_var_154 - 3)); _for_it_47 = (_for_it_47 + 1)) {

        tmp_call_15 = 0;
        for (tmp_parfor_0 = i_startblk_var_148; (tmp_parfor_0 <= i_endblk_var_149); tmp_parfor_0 = (tmp_parfor_0 + 1)) {
            if ((levmask[(((__CG_p_patch__m_nblks_c * (_for_it_47 - 1)) + tmp_parfor_0) - 1)] == 1)) {

                tmp_call_15 = 1;

            }

        }
        {

            {
                int levelmask_out_0;

                ///////////////////
                // Tasklet code (T_l589_c589)
                levelmask_out_0 = tmp_call_15;
                ///////////////////

                levelmask[(_for_it_47 - 1)] = levelmask_out_0;
            }

        }

    }
    i_startblk_var_148 = __CG_p_patch__CG_edges__m_start_block[(10 - SOA_start_block_d_0_edges_p_patch_4)];
    i_endblk_var_149 = __CG_p_patch__CG_edges__m_end_block[((- SOA_end_block_d_0_edges_p_patch_4) - 8)];
    for (_for_it_48 = i_startblk_var_148; (_for_it_48 <= i_endblk_var_149); _for_it_48 = (_for_it_48 + 1)) {

        i_startidx_in_var_93_0 = __CG_p_patch__CG_edges__m_start_index[(10 - SOA_start_index_d_0_edges_p_patch_4)];
        i_endidx_in_var_94_0 = __CG_p_patch__CG_edges__m_end_index[((- SOA_end_index_d_0_edges_p_patch_4) - 8)];
        if ((_for_it_48 != i_startblk_var_148)) {

            i_startidx_var_150 = 1;

        } else {

            i_startidx_var_150 = max(1, i_startidx_in_var_93_0);

        }
        if ((_for_it_48 != i_endblk_var_149)) {

            i_endidx_var_151 = __CG_global_data__m_nproma;

        } else {

            i_endidx_var_151 = i_endidx_in_var_94_0;

        }
        for (_for_it_49 = 1; (_for_it_49 <= nlev_var_154); _for_it_49 = (_for_it_49 + 1)) {
            for (_for_it_50 = i_startidx_var_150; (_for_it_50 <= i_endidx_var_151); _for_it_50 = (_for_it_50 + 1)) {

                tmp_index_724 = (ntnd - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3);
                tmp_index_743 = (__CG_p_patch__CG_edges__m_cell_idx[((((SA_cell_idx_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                tmp_index_745 = (__CG_p_patch__CG_edges__m_cell_blk[((((SA_cell_blk_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                tmp_index_755 = (__CG_p_patch__CG_edges__m_cell_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_757 = (__CG_p_patch__CG_edges__m_cell_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_769 = (__CG_p_patch__CG_edges__m_vertex_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_771 = (__CG_p_patch__CG_edges__m_vertex_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_778 = (__CG_p_patch__CG_edges__m_vertex_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                tmp_index_780 = (__CG_p_patch__CG_edges__m_vertex_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                tmp_index_790 = (__CG_p_patch__CG_edges__m_cell_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_792 = (__CG_p_patch__CG_edges__m_cell_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)] - 1);
                tmp_index_802 = (__CG_p_patch__CG_edges__m_cell_idx[((((SA_cell_idx_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                tmp_index_804 = (__CG_p_patch__CG_edges__m_cell_blk[((((SA_cell_blk_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_50) - 1)] - 1);
                {

                    {
                        double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_48)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_49))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_50)];
                        double p_diag_1_in_vn_ie_0 = __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_48)) + (__CG_p_diag__m_SA_vn_ie_d_0 * ((- __CG_p_diag__m_SOA_vn_ie_d_1) + _for_it_49))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_50)];
                        double p_diag_2_in_vn_ie_0 = __CG_p_diag__m_vn_ie[(((((__CG_p_diag__m_SA_vn_ie_d_0 * __CG_p_diag__m_SA_vn_ie_d_1) * ((- __CG_p_diag__m_SOA_vn_ie_d_2) + _for_it_48)) + (__CG_p_diag__m_SA_vn_ie_d_0 * (((- __CG_p_diag__m_SOA_vn_ie_d_1) + _for_it_49) + 1))) - __CG_p_diag__m_SOA_vn_ie_d_0) + _for_it_50)];
                        double p_int_0_in_c_lin_e_0 = __CG_p_int__m_c_lin_e[(((((__CG_p_int__m_SA_c_lin_e_d_0 * __CG_p_int__m_SA_c_lin_e_d_1) * ((- __CG_p_int__m_SOA_c_lin_e_d_2) + _for_it_48)) + (__CG_p_int__m_SA_c_lin_e_d_0 * (1 - __CG_p_int__m_SOA_c_lin_e_d_1))) - __CG_p_int__m_SOA_c_lin_e_d_0) + _for_it_50)];
                        double p_int_1_in_c_lin_e_0 = __CG_p_int__m_c_lin_e[(((((__CG_p_int__m_SA_c_lin_e_d_0 * __CG_p_int__m_SA_c_lin_e_d_1) * ((- __CG_p_int__m_SOA_c_lin_e_d_2) + _for_it_48)) + (__CG_p_int__m_SA_c_lin_e_d_0 * (2 - __CG_p_int__m_SOA_c_lin_e_d_1))) - __CG_p_int__m_SOA_c_lin_e_d_0) + _for_it_50)];
                        double p_metrics_0_in_coeff_gradekin_0 = __CG_p_metrics__m_coeff_gradekin[(((((__CG_p_metrics__m_SA_coeff_gradekin_d_0 * __CG_p_metrics__m_SA_coeff_gradekin_d_1) * ((- __CG_p_metrics__m_SOA_coeff_gradekin_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_coeff_gradekin_d_0 * (1 - __CG_p_metrics__m_SOA_coeff_gradekin_d_1))) - __CG_p_metrics__m_SOA_coeff_gradekin_d_0) + _for_it_50)];
                        double p_metrics_1_in_coeff_gradekin_0 = __CG_p_metrics__m_coeff_gradekin[(((((__CG_p_metrics__m_SA_coeff_gradekin_d_0 * __CG_p_metrics__m_SA_coeff_gradekin_d_1) * ((- __CG_p_metrics__m_SOA_coeff_gradekin_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_coeff_gradekin_d_0 * (2 - __CG_p_metrics__m_SOA_coeff_gradekin_d_1))) - __CG_p_metrics__m_SOA_coeff_gradekin_d_0) + _for_it_50)];
                        double p_metrics_2_in_coeff_gradekin_0 = __CG_p_metrics__m_coeff_gradekin[(((((__CG_p_metrics__m_SA_coeff_gradekin_d_0 * __CG_p_metrics__m_SA_coeff_gradekin_d_1) * ((- __CG_p_metrics__m_SOA_coeff_gradekin_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_coeff_gradekin_d_0 * (2 - __CG_p_metrics__m_SOA_coeff_gradekin_d_1))) - __CG_p_metrics__m_SOA_coeff_gradekin_d_0) + _for_it_50)];
                        double p_metrics_3_in_coeff_gradekin_0 = __CG_p_metrics__m_coeff_gradekin[(((((__CG_p_metrics__m_SA_coeff_gradekin_d_0 * __CG_p_metrics__m_SA_coeff_gradekin_d_1) * ((- __CG_p_metrics__m_SOA_coeff_gradekin_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_coeff_gradekin_d_0 * (1 - __CG_p_metrics__m_SOA_coeff_gradekin_d_1))) - __CG_p_metrics__m_SOA_coeff_gradekin_d_0) + _for_it_50)];
                        double p_metrics_4_in_ddqz_z_full_e_0 = __CG_p_metrics__m_ddqz_z_full_e[(((((__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * __CG_p_metrics__m_SA_ddqz_z_full_e_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1) + _for_it_49))) - __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0) + _for_it_50)];
                        double p_patch_0_in_edges_f_e_0 = __CG_p_patch__CG_edges__m_f_e[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_50) - 1)];
                        double z_ekinh_0_in_0 = z_ekinh[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_745) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_743)];
                        double z_ekinh_1_in_0 = z_ekinh[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_757) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_755)];
                        double z_kin_hor_e_0_in_0 = z_kin_hor_e[(((((A_z_kin_hor_e_d_0 * A_z_kin_hor_e_d_1) * ((- OA_z_kin_hor_e_d_2) + _for_it_48)) + (A_z_kin_hor_e_d_0 * ((- OA_z_kin_hor_e_d_1) + _for_it_49))) - OA_z_kin_hor_e_d_0) + _for_it_50)];
                        double z_w_con_c_full_0_in_0 = z_w_con_c_full[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_792) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_790)];
                        double z_w_con_c_full_1_in_0 = z_w_con_c_full[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_804) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_802)];
                        double zeta_0_in_0 = zeta[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_771) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_769)];
                        double zeta_1_in_0 = zeta[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_780) + (__CG_global_data__m_nproma * (_for_it_49 - 1))) + tmp_index_778)];
                        double p_diag_out_ddt_vn_apc_pc_0;

                        ///////////////////
                        // Tasklet code (T_l600_c600)
                        p_diag_out_ddt_vn_apc_pc_0 = (- (((((z_kin_hor_e_0_in_0 * (p_metrics_0_in_coeff_gradekin_0 - p_metrics_1_in_coeff_gradekin_0)) + (p_metrics_2_in_coeff_gradekin_0 * z_ekinh_0_in_0)) - (p_metrics_3_in_coeff_gradekin_0 * z_ekinh_1_in_0)) + (p_diag_0_in_vt_0 * (p_patch_0_in_edges_f_e_0 + (0.5 * (zeta_0_in_0 + zeta_1_in_0))))) + ((((p_int_0_in_c_lin_e_0 * z_w_con_c_full_0_in_0) + (p_int_1_in_c_lin_e_0 * z_w_con_c_full_1_in_0)) * (p_diag_1_in_vn_ie_0 - p_diag_2_in_vn_ie_0)) / p_metrics_4_in_ddqz_z_full_e_0)));
                        ///////////////////

                        __CG_p_diag__m_ddt_vn_apc_pc[(((((((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2) * tmp_index_724) + ((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2) + _for_it_48))) + (__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1) + _for_it_49))) - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0) + _for_it_50)] = p_diag_out_ddt_vn_apc_pc_0;
                    }

                }

            }

        }
        {

            {
                int p_diag_0_in_ddt_vn_adv_is_associated = __CG_p_diag__m_ddt_vn_adv_is_associated;
                int p_diag_1_in_ddt_vn_cor_is_associated = __CG_p_diag__m_ddt_vn_cor_is_associated;
                int _if_cond_29_out;

                ///////////////////
                // Tasklet code (T_l0_c0)
                _if_cond_29_out = (p_diag_0_in_ddt_vn_adv_is_associated || p_diag_1_in_ddt_vn_cor_is_associated);
                ///////////////////

                _if_cond_29 = _if_cond_29_out;
            }

        }
        if ((_if_cond_29 == 1)) {
            for (_for_it_51 = 1; (_for_it_51 <= nlev_var_154); _for_it_51 = (_for_it_51 + 1)) {
                for (_for_it_52 = i_startidx_var_150; (_for_it_52 <= i_endidx_var_151); _for_it_52 = (_for_it_52 + 1)) {

                    tmp_index_817 = (ntnd - __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3);
                    {

                        {
                            double p_diag_0_in_vt_0 = __CG_p_diag__m_vt[(((((__CG_p_diag__m_SA_vt_d_0 * __CG_p_diag__m_SA_vt_d_1) * ((- __CG_p_diag__m_SOA_vt_d_2) + _for_it_48)) + (__CG_p_diag__m_SA_vt_d_0 * ((- __CG_p_diag__m_SOA_vt_d_1) + _for_it_51))) - __CG_p_diag__m_SOA_vt_d_0) + _for_it_52)];
                            double p_patch_0_in_edges_f_e_0 = __CG_p_patch__CG_edges__m_f_e[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_52) - 1)];
                            double p_diag_out_ddt_vn_cor_pc_0;

                            ///////////////////
                            // Tasklet code (T_l606_c606)
                            p_diag_out_ddt_vn_cor_pc_0 = (- (p_diag_0_in_vt_0 * p_patch_0_in_edges_f_e_0));
                            ///////////////////

                            __CG_p_diag__m_ddt_vn_cor_pc[(((((((__CG_p_diag__m_SA_ddt_vn_cor_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1) * __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2) * tmp_index_817) + ((__CG_p_diag__m_SA_ddt_vn_cor_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2) + _for_it_48))) + (__CG_p_diag__m_SA_ddt_vn_cor_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1) + _for_it_51))) - __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0) + _for_it_52)] = p_diag_out_ddt_vn_cor_pc_0;
                        }

                    }

                }

            }
        }
        tmp_arg_17 = (nrdmax_jg - 2);
        for (_for_it_57 = max(3, tmp_arg_17); (_for_it_57 <= (nlev_var_154 - 4)); _for_it_57 = (_for_it_57 + 1)) {

            _if_cond_32 = (levelmask[(_for_it_57 - 1)] || levelmask[_for_it_57]);
            if ((_if_cond_32 == 1)) {
                for (_for_it_58 = i_startidx_var_150; (_for_it_58 <= i_endidx_var_151); _for_it_58 = (_for_it_58 + 1)) {

                    tmp_index_970 = (__CG_p_patch__CG_edges__m_cell_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - 1);
                    tmp_index_972 = (__CG_p_patch__CG_edges__m_cell_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - 1);
                    tmp_index_982 = (__CG_p_patch__CG_edges__m_cell_idx[((((SA_cell_idx_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - 1);
                    tmp_index_984 = (__CG_p_patch__CG_edges__m_cell_blk[((((SA_cell_blk_d_1_edges_p_patch_4 * __CG_global_data__m_nproma) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - 1);
                    {
                        double tmp_call_17;

                        {
                            double p_int_0_in_c_lin_e_0 = __CG_p_int__m_c_lin_e[(((((__CG_p_int__m_SA_c_lin_e_d_0 * __CG_p_int__m_SA_c_lin_e_d_1) * ((- __CG_p_int__m_SOA_c_lin_e_d_2) + _for_it_48)) + (__CG_p_int__m_SA_c_lin_e_d_0 * (1 - __CG_p_int__m_SOA_c_lin_e_d_1))) - __CG_p_int__m_SOA_c_lin_e_d_0) + _for_it_58)];
                            double p_int_1_in_c_lin_e_0 = __CG_p_int__m_c_lin_e[(((((__CG_p_int__m_SA_c_lin_e_d_0 * __CG_p_int__m_SA_c_lin_e_d_1) * ((- __CG_p_int__m_SOA_c_lin_e_d_2) + _for_it_48)) + (__CG_p_int__m_SA_c_lin_e_d_0 * (2 - __CG_p_int__m_SOA_c_lin_e_d_1))) - __CG_p_int__m_SOA_c_lin_e_d_0) + _for_it_58)];
                            double z_w_con_c_full_0_in_0 = z_w_con_c_full[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_972) + (__CG_global_data__m_nproma * (_for_it_57 - 1))) + tmp_index_970)];
                            double z_w_con_c_full_1_in_0 = z_w_con_c_full[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_984) + (__CG_global_data__m_nproma * (_for_it_57 - 1))) + tmp_index_982)];
                            double w_con_e_out;

                            ///////////////////
                            // Tasklet code (T_l629_c629)
                            w_con_e_out = ((p_int_0_in_c_lin_e_0 * z_w_con_c_full_0_in_0) + (p_int_1_in_c_lin_e_0 * z_w_con_c_full_1_in_0));
                            ///////////////////

                            w_con_e = w_con_e_out;
                        }
                        {
                            double w_con_e_0_in = w_con_e;
                            double tmp_call_17_out;

                            ///////////////////
                            // Tasklet code (T_l0_c0)
                            tmp_call_17_out = abs(w_con_e_0_in);
                            ///////////////////

                            tmp_call_17 = tmp_call_17_out;
                        }
                        {
                            double cfl_w_limit_0_in = cfl_w_limit;
                            double p_metrics_0_in_ddqz_z_full_e_0 = __CG_p_metrics__m_ddqz_z_full_e[(((((__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * __CG_p_metrics__m_SA_ddqz_z_full_e_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1) + _for_it_57))) - __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0) + _for_it_58)];
                            double tmp_call_17_0_in = tmp_call_17;
                            double _if_cond_33_out;

                            ///////////////////
                            // Tasklet code (T_l0_c0)
                            _if_cond_33_out = (tmp_call_17_0_in > (cfl_w_limit_0_in * p_metrics_0_in_ddqz_z_full_e_0));
                            ///////////////////

                            _if_cond_33 = _if_cond_33_out;
                        }

                    }
                    if ((_if_cond_33 == 1)) {
                        {

                            {
                                double w_con_e_0_in = w_con_e;
                                double tmp_call_19_out;

                                ///////////////////
                                // Tasklet code (T_l631_c631)
                                tmp_call_19_out = abs(w_con_e_0_in);
                                ///////////////////

                                tmp_call_19 = tmp_call_19_out;
                            }

                        }
                        tmp_index_994 = (ntnd - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3);
                        tmp_index_998 = (ntnd - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3);
                        tmp_index_1016 = (__CG_p_patch__CG_edges__m_quad_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                        tmp_index_1018 = (__CG_p_patch__CG_edges__m_quad_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                        tmp_index_1028 = (__CG_p_patch__CG_edges__m_quad_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                        tmp_index_1030 = (__CG_p_patch__CG_edges__m_quad_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                        tmp_index_1040 = (__CG_p_patch__CG_edges__m_quad_idx[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                        tmp_index_1042 = (__CG_p_patch__CG_edges__m_quad_blk[(((((2 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                        tmp_index_1052 = (__CG_p_patch__CG_edges__m_quad_idx[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_0);
                        tmp_index_1054 = (__CG_p_patch__CG_edges__m_quad_blk[(((((3 * __CG_global_data__m_nproma) * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - __CG_p_prog__m_SOA_vn_d_2);
                        tmp_index_1065 = (__CG_p_patch__CG_edges__m_vertex_idx[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - 1);
                        tmp_index_1067 = (__CG_p_patch__CG_edges__m_vertex_blk[((((__CG_global_data__m_nproma * __CG_p_patch__m_nblks_e) + (__CG_global_data__m_nproma * (_for_it_48 - 1))) + _for_it_58) - 1)] - 1);
                        tmp_index_1074 = (__CG_p_patch__CG_edges__m_vertex_idx[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - 1);
                        tmp_index_1076 = (__CG_p_patch__CG_edges__m_vertex_blk[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)] - 1);
                        {
                            double tmp_call_18;
                            double tmp_arg_18;
                            double tmp_arg_19;

                            {
                                double cfl_w_limit_0_in = cfl_w_limit;
                                double dtime_0_in = dtime;
                                double tmp_arg_18_out;

                                ///////////////////
                                // Tasklet code (T_l631_c631)
                                tmp_arg_18_out = (0.85 - (cfl_w_limit_0_in * dtime_0_in));
                                ///////////////////

                                tmp_arg_18 = tmp_arg_18_out;
                            }
                            {
                                double cfl_w_limit_0_in = cfl_w_limit;
                                double dtime_0_in = dtime;
                                double dtime_1_in = dtime;
                                double p_metrics_0_in_ddqz_z_full_e_0 = __CG_p_metrics__m_ddqz_z_full_e[(((((__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * __CG_p_metrics__m_SA_ddqz_z_full_e_d_1) * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2) + _for_it_48)) + (__CG_p_metrics__m_SA_ddqz_z_full_e_d_0 * ((- __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1) + _for_it_57))) - __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0) + _for_it_58)];
                                double tmp_call_19_0_in = tmp_call_19;
                                double tmp_arg_19_out;

                                ///////////////////
                                // Tasklet code (T_l631_c631)
                                tmp_arg_19_out = (((tmp_call_19_0_in * dtime_0_in) / p_metrics_0_in_ddqz_z_full_e_0) - (cfl_w_limit_0_in * dtime_1_in));
                                ///////////////////

                                tmp_arg_19 = tmp_arg_19_out;
                            }
                            {
                                double tmp_arg_18_0_in = tmp_arg_18;
                                double tmp_arg_19_0_in = tmp_arg_19;
                                double tmp_call_18_out;

                                ///////////////////
                                // Tasklet code (T_l631_c631)
                                tmp_call_18_out = min(tmp_arg_18_0_in, tmp_arg_19_0_in);
                                ///////////////////

                                tmp_call_18 = tmp_call_18_out;
                            }
                            {
                                double scalfac_exdiff_0_in = scalfac_exdiff;
                                double tmp_call_18_0_in = tmp_call_18;
                                double difcoef_out;

                                ///////////////////
                                // Tasklet code (T_l631_c631)
                                difcoef_out = (scalfac_exdiff_0_in * tmp_call_18_0_in);
                                ///////////////////

                                difcoef = difcoef_out;
                            }
                            {
                                double difcoef_0_in = difcoef;
                                double p_diag_0_in_ddt_vn_apc_pc_0 = __CG_p_diag__m_ddt_vn_apc_pc[(((((((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2) * tmp_index_998) + ((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2) + _for_it_48))) + (__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1) + _for_it_57))) - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0) + _for_it_58)];
                                double p_int_0_in_geofac_grdiv_0 = __CG_p_int__m_geofac_grdiv[(((((__CG_p_int__m_SA_geofac_grdiv_d_0 * __CG_p_int__m_SA_geofac_grdiv_d_1) * ((- __CG_p_int__m_SOA_geofac_grdiv_d_2) + _for_it_48)) + (__CG_p_int__m_SA_geofac_grdiv_d_0 * (1 - __CG_p_int__m_SOA_geofac_grdiv_d_1))) - __CG_p_int__m_SOA_geofac_grdiv_d_0) + _for_it_58)];
                                double p_int_1_in_geofac_grdiv_0 = __CG_p_int__m_geofac_grdiv[(((((__CG_p_int__m_SA_geofac_grdiv_d_0 * __CG_p_int__m_SA_geofac_grdiv_d_1) * ((- __CG_p_int__m_SOA_geofac_grdiv_d_2) + _for_it_48)) + (__CG_p_int__m_SA_geofac_grdiv_d_0 * (2 - __CG_p_int__m_SOA_geofac_grdiv_d_1))) - __CG_p_int__m_SOA_geofac_grdiv_d_0) + _for_it_58)];
                                double p_int_2_in_geofac_grdiv_0 = __CG_p_int__m_geofac_grdiv[(((((__CG_p_int__m_SA_geofac_grdiv_d_0 * __CG_p_int__m_SA_geofac_grdiv_d_1) * ((- __CG_p_int__m_SOA_geofac_grdiv_d_2) + _for_it_48)) + (__CG_p_int__m_SA_geofac_grdiv_d_0 * (3 - __CG_p_int__m_SOA_geofac_grdiv_d_1))) - __CG_p_int__m_SOA_geofac_grdiv_d_0) + _for_it_58)];
                                double p_int_3_in_geofac_grdiv_0 = __CG_p_int__m_geofac_grdiv[(((((__CG_p_int__m_SA_geofac_grdiv_d_0 * __CG_p_int__m_SA_geofac_grdiv_d_1) * ((- __CG_p_int__m_SOA_geofac_grdiv_d_2) + _for_it_48)) + (__CG_p_int__m_SA_geofac_grdiv_d_0 * (4 - __CG_p_int__m_SOA_geofac_grdiv_d_1))) - __CG_p_int__m_SOA_geofac_grdiv_d_0) + _for_it_58)];
                                double p_int_4_in_geofac_grdiv_0 = __CG_p_int__m_geofac_grdiv[(((((__CG_p_int__m_SA_geofac_grdiv_d_0 * __CG_p_int__m_SA_geofac_grdiv_d_1) * ((- __CG_p_int__m_SOA_geofac_grdiv_d_2) + _for_it_48)) + (__CG_p_int__m_SA_geofac_grdiv_d_0 * (5 - __CG_p_int__m_SOA_geofac_grdiv_d_1))) - __CG_p_int__m_SOA_geofac_grdiv_d_0) + _for_it_58)];
                                double p_patch_0_in_edges_area_edge_0 = __CG_p_patch__CG_edges__m_area_edge[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)];
                                double p_patch_1_in_edges_tangent_orientation_0 = __CG_p_patch__CG_edges__m_tangent_orientation[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)];
                                double p_patch_2_in_edges_inv_primal_edge_length_0 = __CG_p_patch__CG_edges__m_inv_primal_edge_length[(((__CG_global_data__m_nproma * (_for_it_48 - 1)) + _for_it_58) - 1)];
                                double p_prog_0_in_vn_0 = __CG_p_prog__m_vn[(((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * ((- __CG_p_prog__m_SOA_vn_d_2) + _for_it_48)) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_57))) - __CG_p_prog__m_SOA_vn_d_0) + _for_it_58)];
                                double p_prog_1_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_1018) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_57))) + tmp_index_1016)];
                                double p_prog_2_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_1030) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_57))) + tmp_index_1028)];
                                double p_prog_3_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_1042) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_57))) + tmp_index_1040)];
                                double p_prog_4_in_vn_0 = __CG_p_prog__m_vn[((((__CG_p_prog__m_SA_vn_d_0 * __CG_p_prog__m_SA_vn_d_1) * tmp_index_1054) + (__CG_p_prog__m_SA_vn_d_0 * ((- __CG_p_prog__m_SOA_vn_d_1) + _for_it_57))) + tmp_index_1052)];
                                double zeta_0_in_0 = zeta[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_1067) + (__CG_global_data__m_nproma * (_for_it_57 - 1))) + tmp_index_1065)];
                                double zeta_1_in_0 = zeta[((((__CG_global_data__m_nproma * __CG_p_patch__m_nlev) * tmp_index_1076) + (__CG_global_data__m_nproma * (_for_it_57 - 1))) + tmp_index_1074)];
                                double p_diag_out_ddt_vn_apc_pc_0;

                                ///////////////////
                                // Tasklet code (T_l632_c632)
                                p_diag_out_ddt_vn_apc_pc_0 = (p_diag_0_in_ddt_vn_apc_pc_0 + ((difcoef_0_in * p_patch_0_in_edges_area_edge_0) * ((((((p_int_0_in_geofac_grdiv_0 * p_prog_0_in_vn_0) + (p_int_1_in_geofac_grdiv_0 * p_prog_1_in_vn_0)) + (p_int_2_in_geofac_grdiv_0 * p_prog_2_in_vn_0)) + (p_int_3_in_geofac_grdiv_0 * p_prog_3_in_vn_0)) + (p_int_4_in_geofac_grdiv_0 * p_prog_4_in_vn_0)) + ((p_patch_1_in_edges_tangent_orientation_0 * p_patch_2_in_edges_inv_primal_edge_length_0) * (zeta_0_in_0 - zeta_1_in_0)))));
                                ///////////////////

                                __CG_p_diag__m_ddt_vn_apc_pc[(((((((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2) * tmp_index_994) + ((__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1) * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2) + _for_it_48))) + (__CG_p_diag__m_SA_ddt_vn_apc_pc_d_0 * ((- __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1) + _for_it_57))) - __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0) + _for_it_58)] = p_diag_out_ddt_vn_apc_pc_0;
                            }

                        }
                    }

                }
            }

        }

    }
    i_startblk_var_148 = __CG_p_patch__CG_cells__m_start_block[(4 - SOA_start_block_d_0_cells_p_patch_2)];
    i_endblk_var_149 = __CG_p_patch__CG_cells__m_end_block[((- SOA_end_block_d_0_cells_p_patch_2) - 4)];
    tmp_call_20 = -1.7976931348623157e+308;
    for (tmp_parfor_0 = i_startblk_var_148; (tmp_parfor_0 <= i_endblk_var_149); tmp_parfor_0 = (tmp_parfor_0 + 1)) {
        if ((vcflmax[(tmp_parfor_0 - 1)] > tmp_call_20)) {

            tmp_call_20 = vcflmax[(tmp_parfor_0 - 1)];

        }

    }
    {
        double max_vcfl_dyn_var_156;

        {
            double p_diag_0_in_max_vcfl_dyn = __CG_p_diag__m_max_vcfl_dyn[0];
            double max_vcfl_dyn_var_156_out;

            ///////////////////
            // Tasklet code (T_l641_c641)
            max_vcfl_dyn_var_156_out = max(p_diag_0_in_max_vcfl_dyn, tmp_call_20);
            ///////////////////

            max_vcfl_dyn_var_156 = max_vcfl_dyn_var_156_out;
        }
        {
            double max_vcfl_dyn_var_156_0_in = max_vcfl_dyn_var_156;
            double p_diag_out_max_vcfl_dyn;

            ///////////////////
            // Tasklet code (T_l642_c642)
            p_diag_out_max_vcfl_dyn = max_vcfl_dyn_var_156_0_in;
            ///////////////////

            __CG_p_diag__m_max_vcfl_dyn[0] = p_diag_out_max_vcfl_dyn;
        }

    }
    delete[] z_w_concorr_mc;
    delete[] z_w_con_c;
    delete[] z_w_con_c_full;
    delete[] z_v_grad_w;
    delete[] z_w_v;
    delete[] zeta;
    delete[] z_ekinh;
    delete[] vcflmax;
    delete[] levmask;
    delete[] levelmask;
    delete[] cfl_clipping;
}

DACE_EXPORTED void __program_velocity_no_nproma_if_prop_lvn_only_0_istep_1(velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t *__state, int * __restrict__ __CG_global_data__m_nflatlev, int * __restrict__ __CG_global_data__m_nrdmax, double * __restrict__ __CG_p_diag__m_ddt_vn_apc_pc, double * __restrict__ __CG_p_diag__m_ddt_vn_cor_pc, double * __restrict__ __CG_p_diag__m_ddt_w_adv_pc, double * __restrict__ __CG_p_diag__m_max_vcfl_dyn, double * __restrict__ __CG_p_diag__m_vn_ie, double * __restrict__ __CG_p_diag__m_vn_ie_ubc, double * __restrict__ __CG_p_diag__m_vt, double * __restrict__ __CG_p_diag__m_w_concorr_c, double * __restrict__ __CG_p_int__m_c_lin_e, double * __restrict__ __CG_p_int__m_cells_aw_verts, double * __restrict__ __CG_p_int__m_e_bln_c_s, double * __restrict__ __CG_p_int__m_geofac_grdiv, double * __restrict__ __CG_p_int__m_geofac_n2s, double * __restrict__ __CG_p_int__m_geofac_rot, double * __restrict__ __CG_p_int__m_rbf_vec_coeff_e, double * __restrict__ __CG_p_metrics__m_coeff1_dwdz, double * __restrict__ __CG_p_metrics__m_coeff2_dwdz, double * __restrict__ __CG_p_metrics__m_coeff_gradekin, double * __restrict__ __CG_p_metrics__m_ddqz_z_full_e, double * __restrict__ __CG_p_metrics__m_ddqz_z_half, double * __restrict__ __CG_p_metrics__m_ddxn_z_full, double * __restrict__ __CG_p_metrics__m_ddxt_z_full, double * __restrict__ __CG_p_metrics__m_deepatmo_gradh_ifc, double * __restrict__ __CG_p_metrics__m_deepatmo_gradh_mc, double * __restrict__ __CG_p_metrics__m_deepatmo_invr_ifc, double * __restrict__ __CG_p_metrics__m_deepatmo_invr_mc, double * __restrict__ __CG_p_metrics__m_wgtfac_c, double * __restrict__ __CG_p_metrics__m_wgtfac_e, double * __restrict__ __CG_p_metrics__m_wgtfacq_e, int * __restrict__ __CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask, double * __restrict__ __CG_p_patch__CG_cells__m_area, int * __restrict__ __CG_p_patch__CG_cells__m_edge_blk, int * __restrict__ __CG_p_patch__CG_cells__m_edge_idx, int * __restrict__ __CG_p_patch__CG_cells__m_end_block, int * __restrict__ __CG_p_patch__CG_cells__m_end_index, int * __restrict__ __CG_p_patch__CG_cells__m_neighbor_blk, int * __restrict__ __CG_p_patch__CG_cells__m_neighbor_idx, int * __restrict__ __CG_p_patch__CG_cells__m_start_block, int * __restrict__ __CG_p_patch__CG_cells__m_start_index, double * __restrict__ __CG_p_patch__CG_edges__m_area_edge, int * __restrict__ __CG_p_patch__CG_edges__m_cell_blk, int * __restrict__ __CG_p_patch__CG_edges__m_cell_idx, int * __restrict__ __CG_p_patch__CG_edges__m_end_block, int * __restrict__ __CG_p_patch__CG_edges__m_end_index, double * __restrict__ __CG_p_patch__CG_edges__m_f_e, double * __restrict__ __CG_p_patch__CG_edges__m_fn_e, double * __restrict__ __CG_p_patch__CG_edges__m_ft_e, double * __restrict__ __CG_p_patch__CG_edges__m_inv_dual_edge_length, double * __restrict__ __CG_p_patch__CG_edges__m_inv_primal_edge_length, int * __restrict__ __CG_p_patch__CG_edges__m_quad_blk, int * __restrict__ __CG_p_patch__CG_edges__m_quad_idx, int * __restrict__ __CG_p_patch__CG_edges__m_start_block, int * __restrict__ __CG_p_patch__CG_edges__m_start_index, double * __restrict__ __CG_p_patch__CG_edges__m_tangent_orientation, int * __restrict__ __CG_p_patch__CG_edges__m_vertex_blk, int * __restrict__ __CG_p_patch__CG_edges__m_vertex_idx, int * __restrict__ __CG_p_patch__CG_verts__m_cell_blk, int * __restrict__ __CG_p_patch__CG_verts__m_cell_idx, int * __restrict__ __CG_p_patch__CG_verts__m_edge_blk, int * __restrict__ __CG_p_patch__CG_verts__m_edge_idx, int * __restrict__ __CG_p_patch__CG_verts__m_end_block, int * __restrict__ __CG_p_patch__CG_verts__m_end_index, int * __restrict__ __CG_p_patch__CG_verts__m_start_block, int * __restrict__ __CG_p_patch__CG_verts__m_start_index, double * __restrict__ __CG_p_prog__m_vn, double * __restrict__ __CG_p_prog__m_w, double * __restrict__ z_kin_hor_e, double * __restrict__ z_vt_ie, double * __restrict__ z_w_concorr_me, int A_z_kin_hor_e_d_0, int A_z_kin_hor_e_d_1, int A_z_kin_hor_e_d_2, int A_z_vt_ie_d_0, int A_z_vt_ie_d_1, int A_z_vt_ie_d_2, int A_z_w_concorr_me_d_0, int A_z_w_concorr_me_d_1, int A_z_w_concorr_me_d_2, int OA_z_kin_hor_e_d_0, int OA_z_kin_hor_e_d_1, int OA_z_kin_hor_e_d_2, int OA_z_vt_ie_d_0, int OA_z_vt_ie_d_1, int OA_z_vt_ie_d_2, int OA_z_w_concorr_me_d_0, int OA_z_w_concorr_me_d_1, int OA_z_w_concorr_me_d_2, int SA_area_d_0_cells_p_patch_2, int SA_area_d_1_cells_p_patch_2, int SA_cell_blk_d_1_edges_p_patch_4, int SA_cell_blk_d_1_verts_p_patch_5, int SA_cell_blk_d_2_edges_p_patch_4, int SA_cell_blk_d_2_verts_p_patch_5, int SA_cell_idx_d_1_edges_p_patch_4, int SA_cell_idx_d_1_verts_p_patch_5, int SA_cell_idx_d_2_edges_p_patch_4, int SA_cell_idx_d_2_verts_p_patch_5, int SA_edge_blk_d_2_cells_p_patch_2, int SA_edge_blk_d_2_verts_p_patch_5, int SA_edge_idx_d_2_cells_p_patch_2, int SA_edge_idx_d_2_verts_p_patch_5, int SA_end_block_d_0_cells_p_patch_2, int SA_end_block_d_0_edges_p_patch_4, int SA_end_block_d_0_verts_p_patch_5, int SA_end_index_d_0_cells_p_patch_2, int SA_end_index_d_0_edges_p_patch_4, int SA_end_index_d_0_verts_p_patch_5, int SA_neighbor_blk_d_2_cells_p_patch_2, int SA_neighbor_idx_d_2_cells_p_patch_2, int SA_quad_blk_d_2_edges_p_patch_4, int SA_quad_idx_d_2_edges_p_patch_4, int SA_start_block_d_0_cells_p_patch_2, int SA_start_block_d_0_edges_p_patch_4, int SA_start_block_d_0_verts_p_patch_5, int SA_start_index_d_0_cells_p_patch_2, int SA_start_index_d_0_edges_p_patch_4, int SA_start_index_d_0_verts_p_patch_5, int SA_vertex_blk_d_2_edges_p_patch_4, int SA_vertex_idx_d_2_edges_p_patch_4, int SOA_area_d_0_cells_p_patch_2, int SOA_area_d_1_cells_p_patch_2, int SOA_end_block_d_0_cells_p_patch_2, int SOA_end_block_d_0_edges_p_patch_4, int SOA_end_block_d_0_verts_p_patch_5, int SOA_end_index_d_0_cells_p_patch_2, int SOA_end_index_d_0_edges_p_patch_4, int SOA_end_index_d_0_verts_p_patch_5, int SOA_start_block_d_0_cells_p_patch_2, int SOA_start_block_d_0_edges_p_patch_4, int SOA_start_block_d_0_verts_p_patch_5, int SOA_start_index_d_0_cells_p_patch_2, int SOA_start_index_d_0_edges_p_patch_4, int SOA_start_index_d_0_verts_p_patch_5, int __CG_global_data__m_i_am_accel_node, int __CG_global_data__m_lextra_diffu, int __CG_global_data__m_lvert_nest, int __CG_global_data__m_nproma, int __CG_global_data__m_timer_intp, int __CG_global_data__m_timer_solve_nh_veltend, int __CG_global_data__m_timers_level, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SA_vn_ie_d_0, int __CG_p_diag__m_SA_vn_ie_d_1, int __CG_p_diag__m_SA_vn_ie_d_2, int __CG_p_diag__m_SA_vn_ie_ubc_d_0, int __CG_p_diag__m_SA_vn_ie_ubc_d_1, int __CG_p_diag__m_SA_vn_ie_ubc_d_2, int __CG_p_diag__m_SA_vt_d_0, int __CG_p_diag__m_SA_vt_d_1, int __CG_p_diag__m_SA_vt_d_2, int __CG_p_diag__m_SA_w_concorr_c_d_0, int __CG_p_diag__m_SA_w_concorr_c_d_1, int __CG_p_diag__m_SA_w_concorr_c_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SOA_vn_ie_d_0, int __CG_p_diag__m_SOA_vn_ie_d_1, int __CG_p_diag__m_SOA_vn_ie_d_2, int __CG_p_diag__m_SOA_vn_ie_ubc_d_0, int __CG_p_diag__m_SOA_vn_ie_ubc_d_1, int __CG_p_diag__m_SOA_vn_ie_ubc_d_2, int __CG_p_diag__m_SOA_vt_d_0, int __CG_p_diag__m_SOA_vt_d_1, int __CG_p_diag__m_SOA_vt_d_2, int __CG_p_diag__m_SOA_w_concorr_c_d_0, int __CG_p_diag__m_SOA_w_concorr_c_d_1, int __CG_p_diag__m_SOA_w_concorr_c_d_2, int __CG_p_diag__m_ddt_vn_adv_is_associated, int __CG_p_diag__m_ddt_vn_cor_is_associated, int __CG_p_int__m_SA_c_lin_e_d_0, int __CG_p_int__m_SA_c_lin_e_d_1, int __CG_p_int__m_SA_c_lin_e_d_2, int __CG_p_int__m_SA_cells_aw_verts_d_0, int __CG_p_int__m_SA_cells_aw_verts_d_1, int __CG_p_int__m_SA_cells_aw_verts_d_2, int __CG_p_int__m_SA_e_bln_c_s_d_0, int __CG_p_int__m_SA_e_bln_c_s_d_1, int __CG_p_int__m_SA_e_bln_c_s_d_2, int __CG_p_int__m_SA_geofac_grdiv_d_0, int __CG_p_int__m_SA_geofac_grdiv_d_1, int __CG_p_int__m_SA_geofac_grdiv_d_2, int __CG_p_int__m_SA_geofac_n2s_d_0, int __CG_p_int__m_SA_geofac_n2s_d_1, int __CG_p_int__m_SA_geofac_n2s_d_2, int __CG_p_int__m_SA_geofac_rot_d_0, int __CG_p_int__m_SA_geofac_rot_d_1, int __CG_p_int__m_SA_geofac_rot_d_2, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_2, int __CG_p_int__m_SOA_c_lin_e_d_0, int __CG_p_int__m_SOA_c_lin_e_d_1, int __CG_p_int__m_SOA_c_lin_e_d_2, int __CG_p_int__m_SOA_cells_aw_verts_d_0, int __CG_p_int__m_SOA_cells_aw_verts_d_1, int __CG_p_int__m_SOA_cells_aw_verts_d_2, int __CG_p_int__m_SOA_e_bln_c_s_d_0, int __CG_p_int__m_SOA_e_bln_c_s_d_1, int __CG_p_int__m_SOA_e_bln_c_s_d_2, int __CG_p_int__m_SOA_geofac_grdiv_d_0, int __CG_p_int__m_SOA_geofac_grdiv_d_1, int __CG_p_int__m_SOA_geofac_grdiv_d_2, int __CG_p_int__m_SOA_geofac_n2s_d_0, int __CG_p_int__m_SOA_geofac_n2s_d_1, int __CG_p_int__m_SOA_geofac_n2s_d_2, int __CG_p_int__m_SOA_geofac_rot_d_0, int __CG_p_int__m_SOA_geofac_rot_d_1, int __CG_p_int__m_SOA_geofac_rot_d_2, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2, int __CG_p_metrics__m_SA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SA_coeff_gradekin_d_0, int __CG_p_metrics__m_SA_coeff_gradekin_d_1, int __CG_p_metrics__m_SA_coeff_gradekin_d_2, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SA_ddqz_z_half_d_0, int __CG_p_metrics__m_SA_ddqz_z_half_d_1, int __CG_p_metrics__m_SA_ddqz_z_half_d_2, int __CG_p_metrics__m_SA_ddxn_z_full_d_0, int __CG_p_metrics__m_SA_ddxn_z_full_d_1, int __CG_p_metrics__m_SA_ddxn_z_full_d_2, int __CG_p_metrics__m_SA_ddxt_z_full_d_0, int __CG_p_metrics__m_SA_ddxt_z_full_d_1, int __CG_p_metrics__m_SA_ddxt_z_full_d_2, int __CG_p_metrics__m_SA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_1, int __CG_p_metrics__m_SA_wgtfac_c_d_2, int __CG_p_metrics__m_SA_wgtfac_e_d_0, int __CG_p_metrics__m_SA_wgtfac_e_d_1, int __CG_p_metrics__m_SA_wgtfac_e_d_2, int __CG_p_metrics__m_SA_wgtfacq_e_d_0, int __CG_p_metrics__m_SA_wgtfacq_e_d_1, int __CG_p_metrics__m_SA_wgtfacq_e_d_2, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff_gradekin_d_0, int __CG_p_metrics__m_SOA_coeff_gradekin_d_1, int __CG_p_metrics__m_SOA_coeff_gradekin_d_2, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SOA_ddqz_z_half_d_0, int __CG_p_metrics__m_SOA_ddqz_z_half_d_1, int __CG_p_metrics__m_SOA_ddqz_z_half_d_2, int __CG_p_metrics__m_SOA_ddxn_z_full_d_0, int __CG_p_metrics__m_SOA_ddxn_z_full_d_1, int __CG_p_metrics__m_SOA_ddxn_z_full_d_2, int __CG_p_metrics__m_SOA_ddxt_z_full_d_0, int __CG_p_metrics__m_SOA_ddxt_z_full_d_1, int __CG_p_metrics__m_SOA_ddxt_z_full_d_2, int __CG_p_metrics__m_SOA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_1, int __CG_p_metrics__m_SOA_wgtfac_c_d_2, int __CG_p_metrics__m_SOA_wgtfac_e_d_0, int __CG_p_metrics__m_SOA_wgtfac_e_d_1, int __CG_p_metrics__m_SOA_wgtfac_e_d_2, int __CG_p_metrics__m_SOA_wgtfacq_e_d_0, int __CG_p_metrics__m_SOA_wgtfacq_e_d_1, int __CG_p_metrics__m_SOA_wgtfacq_e_d_2, int __CG_p_patch__m_id, int __CG_p_patch__m_nblks_c, int __CG_p_patch__m_nblks_e, int __CG_p_patch__m_nblks_v, int __CG_p_patch__m_nlev, int __CG_p_patch__m_nlevp1, int __CG_p_patch__m_nshift, int __CG_p_prog__m_SA_vn_d_0, int __CG_p_prog__m_SA_vn_d_1, int __CG_p_prog__m_SA_vn_d_2, int __CG_p_prog__m_SA_w_d_0, int __CG_p_prog__m_SA_w_d_1, int __CG_p_prog__m_SA_w_d_2, int __CG_p_prog__m_SOA_vn_d_0, int __CG_p_prog__m_SOA_vn_d_1, int __CG_p_prog__m_SOA_vn_d_2, int __CG_p_prog__m_SOA_w_d_0, int __CG_p_prog__m_SOA_w_d_1, int __CG_p_prog__m_SOA_w_d_2, double dt_linintp_ubc, double dtime, int ntnd)
{
    __program_velocity_no_nproma_if_prop_lvn_only_0_istep_1_internal(__state, __CG_global_data__m_nflatlev, __CG_global_data__m_nrdmax, __CG_p_diag__m_ddt_vn_apc_pc, __CG_p_diag__m_ddt_vn_cor_pc, __CG_p_diag__m_ddt_w_adv_pc, __CG_p_diag__m_max_vcfl_dyn, __CG_p_diag__m_vn_ie, __CG_p_diag__m_vn_ie_ubc, __CG_p_diag__m_vt, __CG_p_diag__m_w_concorr_c, __CG_p_int__m_c_lin_e, __CG_p_int__m_cells_aw_verts, __CG_p_int__m_e_bln_c_s, __CG_p_int__m_geofac_grdiv, __CG_p_int__m_geofac_n2s, __CG_p_int__m_geofac_rot, __CG_p_int__m_rbf_vec_coeff_e, __CG_p_metrics__m_coeff1_dwdz, __CG_p_metrics__m_coeff2_dwdz, __CG_p_metrics__m_coeff_gradekin, __CG_p_metrics__m_ddqz_z_full_e, __CG_p_metrics__m_ddqz_z_half, __CG_p_metrics__m_ddxn_z_full, __CG_p_metrics__m_ddxt_z_full, __CG_p_metrics__m_deepatmo_gradh_ifc, __CG_p_metrics__m_deepatmo_gradh_mc, __CG_p_metrics__m_deepatmo_invr_ifc, __CG_p_metrics__m_deepatmo_invr_mc, __CG_p_metrics__m_wgtfac_c, __CG_p_metrics__m_wgtfac_e, __CG_p_metrics__m_wgtfacq_e, __CG_p_patch__CG_cells__CG_decomp_info__m_owner_mask, __CG_p_patch__CG_cells__m_area, __CG_p_patch__CG_cells__m_edge_blk, __CG_p_patch__CG_cells__m_edge_idx, __CG_p_patch__CG_cells__m_end_block, __CG_p_patch__CG_cells__m_end_index, __CG_p_patch__CG_cells__m_neighbor_blk, __CG_p_patch__CG_cells__m_neighbor_idx, __CG_p_patch__CG_cells__m_start_block, __CG_p_patch__CG_cells__m_start_index, __CG_p_patch__CG_edges__m_area_edge, __CG_p_patch__CG_edges__m_cell_blk, __CG_p_patch__CG_edges__m_cell_idx, __CG_p_patch__CG_edges__m_end_block, __CG_p_patch__CG_edges__m_end_index, __CG_p_patch__CG_edges__m_f_e, __CG_p_patch__CG_edges__m_fn_e, __CG_p_patch__CG_edges__m_ft_e, __CG_p_patch__CG_edges__m_inv_dual_edge_length, __CG_p_patch__CG_edges__m_inv_primal_edge_length, __CG_p_patch__CG_edges__m_quad_blk, __CG_p_patch__CG_edges__m_quad_idx, __CG_p_patch__CG_edges__m_start_block, __CG_p_patch__CG_edges__m_start_index, __CG_p_patch__CG_edges__m_tangent_orientation, __CG_p_patch__CG_edges__m_vertex_blk, __CG_p_patch__CG_edges__m_vertex_idx, __CG_p_patch__CG_verts__m_cell_blk, __CG_p_patch__CG_verts__m_cell_idx, __CG_p_patch__CG_verts__m_edge_blk, __CG_p_patch__CG_verts__m_edge_idx, __CG_p_patch__CG_verts__m_end_block, __CG_p_patch__CG_verts__m_end_index, __CG_p_patch__CG_verts__m_start_block, __CG_p_patch__CG_verts__m_start_index, __CG_p_prog__m_vn, __CG_p_prog__m_w, z_kin_hor_e, z_vt_ie, z_w_concorr_me, A_z_kin_hor_e_d_0, A_z_kin_hor_e_d_1, A_z_kin_hor_e_d_2, A_z_vt_ie_d_0, A_z_vt_ie_d_1, A_z_vt_ie_d_2, A_z_w_concorr_me_d_0, A_z_w_concorr_me_d_1, A_z_w_concorr_me_d_2, OA_z_kin_hor_e_d_0, OA_z_kin_hor_e_d_1, OA_z_kin_hor_e_d_2, OA_z_vt_ie_d_0, OA_z_vt_ie_d_1, OA_z_vt_ie_d_2, OA_z_w_concorr_me_d_0, OA_z_w_concorr_me_d_1, OA_z_w_concorr_me_d_2, SA_area_d_0_cells_p_patch_2, SA_area_d_1_cells_p_patch_2, SA_cell_blk_d_1_edges_p_patch_4, SA_cell_blk_d_1_verts_p_patch_5, SA_cell_blk_d_2_edges_p_patch_4, SA_cell_blk_d_2_verts_p_patch_5, SA_cell_idx_d_1_edges_p_patch_4, SA_cell_idx_d_1_verts_p_patch_5, SA_cell_idx_d_2_edges_p_patch_4, SA_cell_idx_d_2_verts_p_patch_5, SA_edge_blk_d_2_cells_p_patch_2, SA_edge_blk_d_2_verts_p_patch_5, SA_edge_idx_d_2_cells_p_patch_2, SA_edge_idx_d_2_verts_p_patch_5, SA_end_block_d_0_cells_p_patch_2, SA_end_block_d_0_edges_p_patch_4, SA_end_block_d_0_verts_p_patch_5, SA_end_index_d_0_cells_p_patch_2, SA_end_index_d_0_edges_p_patch_4, SA_end_index_d_0_verts_p_patch_5, SA_neighbor_blk_d_2_cells_p_patch_2, SA_neighbor_idx_d_2_cells_p_patch_2, SA_quad_blk_d_2_edges_p_patch_4, SA_quad_idx_d_2_edges_p_patch_4, SA_start_block_d_0_cells_p_patch_2, SA_start_block_d_0_edges_p_patch_4, SA_start_block_d_0_verts_p_patch_5, SA_start_index_d_0_cells_p_patch_2, SA_start_index_d_0_edges_p_patch_4, SA_start_index_d_0_verts_p_patch_5, SA_vertex_blk_d_2_edges_p_patch_4, SA_vertex_idx_d_2_edges_p_patch_4, SOA_area_d_0_cells_p_patch_2, SOA_area_d_1_cells_p_patch_2, SOA_end_block_d_0_cells_p_patch_2, SOA_end_block_d_0_edges_p_patch_4, SOA_end_block_d_0_verts_p_patch_5, SOA_end_index_d_0_cells_p_patch_2, SOA_end_index_d_0_edges_p_patch_4, SOA_end_index_d_0_verts_p_patch_5, SOA_start_block_d_0_cells_p_patch_2, SOA_start_block_d_0_edges_p_patch_4, SOA_start_block_d_0_verts_p_patch_5, SOA_start_index_d_0_cells_p_patch_2, SOA_start_index_d_0_edges_p_patch_4, SOA_start_index_d_0_verts_p_patch_5, __CG_global_data__m_i_am_accel_node, __CG_global_data__m_lextra_diffu, __CG_global_data__m_lvert_nest, __CG_global_data__m_nproma, __CG_global_data__m_timer_intp, __CG_global_data__m_timer_solve_nh_veltend, __CG_global_data__m_timers_level, __CG_p_diag__m_SA_ddt_vn_apc_pc_d_0, __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1, __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2, __CG_p_diag__m_SA_ddt_vn_apc_pc_d_3, __CG_p_diag__m_SA_ddt_vn_cor_pc_d_0, __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1, __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2, __CG_p_diag__m_SA_ddt_vn_cor_pc_d_3, __CG_p_diag__m_SA_ddt_w_adv_pc_d_0, __CG_p_diag__m_SA_ddt_w_adv_pc_d_1, __CG_p_diag__m_SA_ddt_w_adv_pc_d_2, __CG_p_diag__m_SA_ddt_w_adv_pc_d_3, __CG_p_diag__m_SA_vn_ie_d_0, __CG_p_diag__m_SA_vn_ie_d_1, __CG_p_diag__m_SA_vn_ie_d_2, __CG_p_diag__m_SA_vn_ie_ubc_d_0, __CG_p_diag__m_SA_vn_ie_ubc_d_1, __CG_p_diag__m_SA_vn_ie_ubc_d_2, __CG_p_diag__m_SA_vt_d_0, __CG_p_diag__m_SA_vt_d_1, __CG_p_diag__m_SA_vt_d_2, __CG_p_diag__m_SA_w_concorr_c_d_0, __CG_p_diag__m_SA_w_concorr_c_d_1, __CG_p_diag__m_SA_w_concorr_c_d_2, __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0, __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1, __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2, __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3, __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0, __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1, __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2, __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3, __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0, __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1, __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2, __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3, __CG_p_diag__m_SOA_vn_ie_d_0, __CG_p_diag__m_SOA_vn_ie_d_1, __CG_p_diag__m_SOA_vn_ie_d_2, __CG_p_diag__m_SOA_vn_ie_ubc_d_0, __CG_p_diag__m_SOA_vn_ie_ubc_d_1, __CG_p_diag__m_SOA_vn_ie_ubc_d_2, __CG_p_diag__m_SOA_vt_d_0, __CG_p_diag__m_SOA_vt_d_1, __CG_p_diag__m_SOA_vt_d_2, __CG_p_diag__m_SOA_w_concorr_c_d_0, __CG_p_diag__m_SOA_w_concorr_c_d_1, __CG_p_diag__m_SOA_w_concorr_c_d_2, __CG_p_diag__m_ddt_vn_adv_is_associated, __CG_p_diag__m_ddt_vn_cor_is_associated, __CG_p_int__m_SA_c_lin_e_d_0, __CG_p_int__m_SA_c_lin_e_d_1, __CG_p_int__m_SA_c_lin_e_d_2, __CG_p_int__m_SA_cells_aw_verts_d_0, __CG_p_int__m_SA_cells_aw_verts_d_1, __CG_p_int__m_SA_cells_aw_verts_d_2, __CG_p_int__m_SA_e_bln_c_s_d_0, __CG_p_int__m_SA_e_bln_c_s_d_1, __CG_p_int__m_SA_e_bln_c_s_d_2, __CG_p_int__m_SA_geofac_grdiv_d_0, __CG_p_int__m_SA_geofac_grdiv_d_1, __CG_p_int__m_SA_geofac_grdiv_d_2, __CG_p_int__m_SA_geofac_n2s_d_0, __CG_p_int__m_SA_geofac_n2s_d_1, __CG_p_int__m_SA_geofac_n2s_d_2, __CG_p_int__m_SA_geofac_rot_d_0, __CG_p_int__m_SA_geofac_rot_d_1, __CG_p_int__m_SA_geofac_rot_d_2, __CG_p_int__m_SA_rbf_vec_coeff_e_d_0, __CG_p_int__m_SA_rbf_vec_coeff_e_d_1, __CG_p_int__m_SA_rbf_vec_coeff_e_d_2, __CG_p_int__m_SOA_c_lin_e_d_0, __CG_p_int__m_SOA_c_lin_e_d_1, __CG_p_int__m_SOA_c_lin_e_d_2, __CG_p_int__m_SOA_cells_aw_verts_d_0, __CG_p_int__m_SOA_cells_aw_verts_d_1, __CG_p_int__m_SOA_cells_aw_verts_d_2, __CG_p_int__m_SOA_e_bln_c_s_d_0, __CG_p_int__m_SOA_e_bln_c_s_d_1, __CG_p_int__m_SOA_e_bln_c_s_d_2, __CG_p_int__m_SOA_geofac_grdiv_d_0, __CG_p_int__m_SOA_geofac_grdiv_d_1, __CG_p_int__m_SOA_geofac_grdiv_d_2, __CG_p_int__m_SOA_geofac_n2s_d_0, __CG_p_int__m_SOA_geofac_n2s_d_1, __CG_p_int__m_SOA_geofac_n2s_d_2, __CG_p_int__m_SOA_geofac_rot_d_0, __CG_p_int__m_SOA_geofac_rot_d_1, __CG_p_int__m_SOA_geofac_rot_d_2, __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0, __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1, __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2, __CG_p_metrics__m_SA_coeff1_dwdz_d_0, __CG_p_metrics__m_SA_coeff1_dwdz_d_1, __CG_p_metrics__m_SA_coeff1_dwdz_d_2, __CG_p_metrics__m_SA_coeff2_dwdz_d_0, __CG_p_metrics__m_SA_coeff2_dwdz_d_1, __CG_p_metrics__m_SA_coeff2_dwdz_d_2, __CG_p_metrics__m_SA_coeff_gradekin_d_0, __CG_p_metrics__m_SA_coeff_gradekin_d_1, __CG_p_metrics__m_SA_coeff_gradekin_d_2, __CG_p_metrics__m_SA_ddqz_z_full_e_d_0, __CG_p_metrics__m_SA_ddqz_z_full_e_d_1, __CG_p_metrics__m_SA_ddqz_z_full_e_d_2, __CG_p_metrics__m_SA_ddqz_z_half_d_0, __CG_p_metrics__m_SA_ddqz_z_half_d_1, __CG_p_metrics__m_SA_ddqz_z_half_d_2, __CG_p_metrics__m_SA_ddxn_z_full_d_0, __CG_p_metrics__m_SA_ddxn_z_full_d_1, __CG_p_metrics__m_SA_ddxn_z_full_d_2, __CG_p_metrics__m_SA_ddxt_z_full_d_0, __CG_p_metrics__m_SA_ddxt_z_full_d_1, __CG_p_metrics__m_SA_ddxt_z_full_d_2, __CG_p_metrics__m_SA_deepatmo_gradh_ifc_d_0, __CG_p_metrics__m_SA_deepatmo_gradh_mc_d_0, __CG_p_metrics__m_SA_deepatmo_invr_ifc_d_0, __CG_p_metrics__m_SA_deepatmo_invr_mc_d_0, __CG_p_metrics__m_SA_wgtfac_c_d_0, __CG_p_metrics__m_SA_wgtfac_c_d_1, __CG_p_metrics__m_SA_wgtfac_c_d_2, __CG_p_metrics__m_SA_wgtfac_e_d_0, __CG_p_metrics__m_SA_wgtfac_e_d_1, __CG_p_metrics__m_SA_wgtfac_e_d_2, __CG_p_metrics__m_SA_wgtfacq_e_d_0, __CG_p_metrics__m_SA_wgtfacq_e_d_1, __CG_p_metrics__m_SA_wgtfacq_e_d_2, __CG_p_metrics__m_SOA_coeff1_dwdz_d_0, __CG_p_metrics__m_SOA_coeff1_dwdz_d_1, __CG_p_metrics__m_SOA_coeff1_dwdz_d_2, __CG_p_metrics__m_SOA_coeff2_dwdz_d_0, __CG_p_metrics__m_SOA_coeff2_dwdz_d_1, __CG_p_metrics__m_SOA_coeff2_dwdz_d_2, __CG_p_metrics__m_SOA_coeff_gradekin_d_0, __CG_p_metrics__m_SOA_coeff_gradekin_d_1, __CG_p_metrics__m_SOA_coeff_gradekin_d_2, __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0, __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1, __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2, __CG_p_metrics__m_SOA_ddqz_z_half_d_0, __CG_p_metrics__m_SOA_ddqz_z_half_d_1, __CG_p_metrics__m_SOA_ddqz_z_half_d_2, __CG_p_metrics__m_SOA_ddxn_z_full_d_0, __CG_p_metrics__m_SOA_ddxn_z_full_d_1, __CG_p_metrics__m_SOA_ddxn_z_full_d_2, __CG_p_metrics__m_SOA_ddxt_z_full_d_0, __CG_p_metrics__m_SOA_ddxt_z_full_d_1, __CG_p_metrics__m_SOA_ddxt_z_full_d_2, __CG_p_metrics__m_SOA_deepatmo_gradh_ifc_d_0, __CG_p_metrics__m_SOA_deepatmo_gradh_mc_d_0, __CG_p_metrics__m_SOA_deepatmo_invr_ifc_d_0, __CG_p_metrics__m_SOA_deepatmo_invr_mc_d_0, __CG_p_metrics__m_SOA_wgtfac_c_d_0, __CG_p_metrics__m_SOA_wgtfac_c_d_1, __CG_p_metrics__m_SOA_wgtfac_c_d_2, __CG_p_metrics__m_SOA_wgtfac_e_d_0, __CG_p_metrics__m_SOA_wgtfac_e_d_1, __CG_p_metrics__m_SOA_wgtfac_e_d_2, __CG_p_metrics__m_SOA_wgtfacq_e_d_0, __CG_p_metrics__m_SOA_wgtfacq_e_d_1, __CG_p_metrics__m_SOA_wgtfacq_e_d_2, __CG_p_patch__m_id, __CG_p_patch__m_nblks_c, __CG_p_patch__m_nblks_e, __CG_p_patch__m_nblks_v, __CG_p_patch__m_nlev, __CG_p_patch__m_nlevp1, __CG_p_patch__m_nshift, __CG_p_prog__m_SA_vn_d_0, __CG_p_prog__m_SA_vn_d_1, __CG_p_prog__m_SA_vn_d_2, __CG_p_prog__m_SA_w_d_0, __CG_p_prog__m_SA_w_d_1, __CG_p_prog__m_SA_w_d_2, __CG_p_prog__m_SOA_vn_d_0, __CG_p_prog__m_SOA_vn_d_1, __CG_p_prog__m_SOA_vn_d_2, __CG_p_prog__m_SOA_w_d_0, __CG_p_prog__m_SOA_w_d_1, __CG_p_prog__m_SOA_w_d_2, dt_linintp_ubc, dtime, ntnd);
}

DACE_EXPORTED velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t *__dace_init_velocity_no_nproma_if_prop_lvn_only_0_istep_1(int A_z_kin_hor_e_d_0, int A_z_kin_hor_e_d_1, int A_z_kin_hor_e_d_2, int A_z_vt_ie_d_0, int A_z_vt_ie_d_1, int A_z_vt_ie_d_2, int A_z_w_concorr_me_d_0, int A_z_w_concorr_me_d_1, int A_z_w_concorr_me_d_2, int OA_z_kin_hor_e_d_0, int OA_z_kin_hor_e_d_1, int OA_z_kin_hor_e_d_2, int OA_z_vt_ie_d_0, int OA_z_vt_ie_d_1, int OA_z_vt_ie_d_2, int OA_z_w_concorr_me_d_0, int OA_z_w_concorr_me_d_1, int OA_z_w_concorr_me_d_2, int SA_area_d_0_cells_p_patch_2, int SA_area_d_1_cells_p_patch_2, int SA_cell_blk_d_1_edges_p_patch_4, int SA_cell_blk_d_1_verts_p_patch_5, int SA_cell_blk_d_2_edges_p_patch_4, int SA_cell_blk_d_2_verts_p_patch_5, int SA_cell_idx_d_1_edges_p_patch_4, int SA_cell_idx_d_1_verts_p_patch_5, int SA_cell_idx_d_2_edges_p_patch_4, int SA_cell_idx_d_2_verts_p_patch_5, int SA_edge_blk_d_2_cells_p_patch_2, int SA_edge_blk_d_2_verts_p_patch_5, int SA_edge_idx_d_2_cells_p_patch_2, int SA_edge_idx_d_2_verts_p_patch_5, int SA_end_block_d_0_cells_p_patch_2, int SA_end_block_d_0_edges_p_patch_4, int SA_end_block_d_0_verts_p_patch_5, int SA_end_index_d_0_cells_p_patch_2, int SA_end_index_d_0_edges_p_patch_4, int SA_end_index_d_0_verts_p_patch_5, int SA_neighbor_blk_d_2_cells_p_patch_2, int SA_neighbor_idx_d_2_cells_p_patch_2, int SA_quad_blk_d_2_edges_p_patch_4, int SA_quad_idx_d_2_edges_p_patch_4, int SA_start_block_d_0_cells_p_patch_2, int SA_start_block_d_0_edges_p_patch_4, int SA_start_block_d_0_verts_p_patch_5, int SA_start_index_d_0_cells_p_patch_2, int SA_start_index_d_0_edges_p_patch_4, int SA_start_index_d_0_verts_p_patch_5, int SA_vertex_blk_d_2_edges_p_patch_4, int SA_vertex_idx_d_2_edges_p_patch_4, int SOA_area_d_0_cells_p_patch_2, int SOA_area_d_1_cells_p_patch_2, int SOA_end_block_d_0_cells_p_patch_2, int SOA_end_block_d_0_edges_p_patch_4, int SOA_end_block_d_0_verts_p_patch_5, int SOA_end_index_d_0_cells_p_patch_2, int SOA_end_index_d_0_edges_p_patch_4, int SOA_end_index_d_0_verts_p_patch_5, int SOA_start_block_d_0_cells_p_patch_2, int SOA_start_block_d_0_edges_p_patch_4, int SOA_start_block_d_0_verts_p_patch_5, int SOA_start_index_d_0_cells_p_patch_2, int SOA_start_index_d_0_edges_p_patch_4, int SOA_start_index_d_0_verts_p_patch_5, int __CG_global_data__m_nproma, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SA_vn_ie_d_0, int __CG_p_diag__m_SA_vn_ie_d_1, int __CG_p_diag__m_SA_vn_ie_d_2, int __CG_p_diag__m_SA_vn_ie_ubc_d_0, int __CG_p_diag__m_SA_vn_ie_ubc_d_1, int __CG_p_diag__m_SA_vn_ie_ubc_d_2, int __CG_p_diag__m_SA_vt_d_0, int __CG_p_diag__m_SA_vt_d_1, int __CG_p_diag__m_SA_vt_d_2, int __CG_p_diag__m_SA_w_concorr_c_d_0, int __CG_p_diag__m_SA_w_concorr_c_d_1, int __CG_p_diag__m_SA_w_concorr_c_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_apc_pc_d_3, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_0, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_1, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_2, int __CG_p_diag__m_SOA_ddt_vn_cor_pc_d_3, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_0, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_1, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_2, int __CG_p_diag__m_SOA_ddt_w_adv_pc_d_3, int __CG_p_diag__m_SOA_vn_ie_d_0, int __CG_p_diag__m_SOA_vn_ie_d_1, int __CG_p_diag__m_SOA_vn_ie_d_2, int __CG_p_diag__m_SOA_vn_ie_ubc_d_0, int __CG_p_diag__m_SOA_vn_ie_ubc_d_1, int __CG_p_diag__m_SOA_vn_ie_ubc_d_2, int __CG_p_diag__m_SOA_vt_d_0, int __CG_p_diag__m_SOA_vt_d_1, int __CG_p_diag__m_SOA_vt_d_2, int __CG_p_diag__m_SOA_w_concorr_c_d_0, int __CG_p_diag__m_SOA_w_concorr_c_d_1, int __CG_p_diag__m_SOA_w_concorr_c_d_2, int __CG_p_int__m_SA_c_lin_e_d_0, int __CG_p_int__m_SA_c_lin_e_d_1, int __CG_p_int__m_SA_c_lin_e_d_2, int __CG_p_int__m_SA_cells_aw_verts_d_0, int __CG_p_int__m_SA_cells_aw_verts_d_1, int __CG_p_int__m_SA_cells_aw_verts_d_2, int __CG_p_int__m_SA_e_bln_c_s_d_0, int __CG_p_int__m_SA_e_bln_c_s_d_1, int __CG_p_int__m_SA_e_bln_c_s_d_2, int __CG_p_int__m_SA_geofac_grdiv_d_0, int __CG_p_int__m_SA_geofac_grdiv_d_1, int __CG_p_int__m_SA_geofac_grdiv_d_2, int __CG_p_int__m_SA_geofac_n2s_d_0, int __CG_p_int__m_SA_geofac_n2s_d_1, int __CG_p_int__m_SA_geofac_n2s_d_2, int __CG_p_int__m_SA_geofac_rot_d_0, int __CG_p_int__m_SA_geofac_rot_d_1, int __CG_p_int__m_SA_geofac_rot_d_2, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SA_rbf_vec_coeff_e_d_2, int __CG_p_int__m_SOA_c_lin_e_d_0, int __CG_p_int__m_SOA_c_lin_e_d_1, int __CG_p_int__m_SOA_c_lin_e_d_2, int __CG_p_int__m_SOA_cells_aw_verts_d_0, int __CG_p_int__m_SOA_cells_aw_verts_d_1, int __CG_p_int__m_SOA_cells_aw_verts_d_2, int __CG_p_int__m_SOA_e_bln_c_s_d_0, int __CG_p_int__m_SOA_e_bln_c_s_d_1, int __CG_p_int__m_SOA_e_bln_c_s_d_2, int __CG_p_int__m_SOA_geofac_grdiv_d_0, int __CG_p_int__m_SOA_geofac_grdiv_d_1, int __CG_p_int__m_SOA_geofac_grdiv_d_2, int __CG_p_int__m_SOA_geofac_n2s_d_0, int __CG_p_int__m_SOA_geofac_n2s_d_1, int __CG_p_int__m_SOA_geofac_n2s_d_2, int __CG_p_int__m_SOA_geofac_rot_d_0, int __CG_p_int__m_SOA_geofac_rot_d_1, int __CG_p_int__m_SOA_geofac_rot_d_2, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_0, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_1, int __CG_p_int__m_SOA_rbf_vec_coeff_e_d_2, int __CG_p_metrics__m_SA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SA_coeff_gradekin_d_0, int __CG_p_metrics__m_SA_coeff_gradekin_d_1, int __CG_p_metrics__m_SA_coeff_gradekin_d_2, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SA_ddqz_z_half_d_0, int __CG_p_metrics__m_SA_ddqz_z_half_d_1, int __CG_p_metrics__m_SA_ddqz_z_half_d_2, int __CG_p_metrics__m_SA_ddxn_z_full_d_0, int __CG_p_metrics__m_SA_ddxn_z_full_d_1, int __CG_p_metrics__m_SA_ddxn_z_full_d_2, int __CG_p_metrics__m_SA_ddxt_z_full_d_0, int __CG_p_metrics__m_SA_ddxt_z_full_d_1, int __CG_p_metrics__m_SA_ddxt_z_full_d_2, int __CG_p_metrics__m_SA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_0, int __CG_p_metrics__m_SA_wgtfac_c_d_1, int __CG_p_metrics__m_SA_wgtfac_c_d_2, int __CG_p_metrics__m_SA_wgtfac_e_d_0, int __CG_p_metrics__m_SA_wgtfac_e_d_1, int __CG_p_metrics__m_SA_wgtfac_e_d_2, int __CG_p_metrics__m_SA_wgtfacq_e_d_0, int __CG_p_metrics__m_SA_wgtfacq_e_d_1, int __CG_p_metrics__m_SA_wgtfacq_e_d_2, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff1_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_0, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_1, int __CG_p_metrics__m_SOA_coeff2_dwdz_d_2, int __CG_p_metrics__m_SOA_coeff_gradekin_d_0, int __CG_p_metrics__m_SOA_coeff_gradekin_d_1, int __CG_p_metrics__m_SOA_coeff_gradekin_d_2, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_0, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_1, int __CG_p_metrics__m_SOA_ddqz_z_full_e_d_2, int __CG_p_metrics__m_SOA_ddqz_z_half_d_0, int __CG_p_metrics__m_SOA_ddqz_z_half_d_1, int __CG_p_metrics__m_SOA_ddqz_z_half_d_2, int __CG_p_metrics__m_SOA_ddxn_z_full_d_0, int __CG_p_metrics__m_SOA_ddxn_z_full_d_1, int __CG_p_metrics__m_SOA_ddxn_z_full_d_2, int __CG_p_metrics__m_SOA_ddxt_z_full_d_0, int __CG_p_metrics__m_SOA_ddxt_z_full_d_1, int __CG_p_metrics__m_SOA_ddxt_z_full_d_2, int __CG_p_metrics__m_SOA_deepatmo_gradh_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_gradh_mc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_ifc_d_0, int __CG_p_metrics__m_SOA_deepatmo_invr_mc_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_0, int __CG_p_metrics__m_SOA_wgtfac_c_d_1, int __CG_p_metrics__m_SOA_wgtfac_c_d_2, int __CG_p_metrics__m_SOA_wgtfac_e_d_0, int __CG_p_metrics__m_SOA_wgtfac_e_d_1, int __CG_p_metrics__m_SOA_wgtfac_e_d_2, int __CG_p_metrics__m_SOA_wgtfacq_e_d_0, int __CG_p_metrics__m_SOA_wgtfacq_e_d_1, int __CG_p_metrics__m_SOA_wgtfacq_e_d_2, int __CG_p_patch__m_nblks_c, int __CG_p_patch__m_nblks_e, int __CG_p_patch__m_nblks_v, int __CG_p_patch__m_nlev, int __CG_p_patch__m_nlevp1, int __CG_p_prog__m_SA_vn_d_0, int __CG_p_prog__m_SA_vn_d_1, int __CG_p_prog__m_SA_vn_d_2, int __CG_p_prog__m_SA_w_d_0, int __CG_p_prog__m_SA_w_d_1, int __CG_p_prog__m_SA_w_d_2, int __CG_p_prog__m_SOA_vn_d_0, int __CG_p_prog__m_SOA_vn_d_1, int __CG_p_prog__m_SOA_vn_d_2, int __CG_p_prog__m_SOA_w_d_0, int __CG_p_prog__m_SOA_w_d_1, int __CG_p_prog__m_SOA_w_d_2)
{

    int __result = 0;
    velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t *__state = new velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t;

    if (__result) {
        delete __state;
        return nullptr;
    }

    return __state;
}

DACE_EXPORTED int __dace_exit_velocity_no_nproma_if_prop_lvn_only_0_istep_1(velocity_no_nproma_if_prop_lvn_only_0_istep_1_state_t *__state)
{

    int __err = 0;
    delete __state;
    return __err;
}
