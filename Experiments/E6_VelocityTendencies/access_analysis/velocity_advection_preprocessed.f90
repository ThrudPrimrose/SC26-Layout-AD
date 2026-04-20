MODULE mo_velocity_advection
  USE mo_kind,                 ONLY: wp, vp
  USE mo_nonhydrostatic_config,ONLY: lextra_diffu
  USE mo_parallel_config,   ONLY: nproma
  USE mo_run_config,        ONLY: lvert_nest, timers_level
  USE mo_model_domain,      ONLY: t_patch
  USE mo_intp_data_strc,    ONLY: t_int_state
  USE mo_icon_interpolation_scalar, ONLY: cells2verts_scalar_ri
  USE mo_nonhydro_types,    ONLY: t_nh_metrics, t_nh_diag, t_nh_prog
  USE mo_math_divrot,       ONLY: rot_vertex_ri
  USE mo_vertical_grid,     ONLY: nrdmax
  USE mo_init_vgrid,        ONLY: nflatlev
  USE mo_loopindices,       ONLY: get_indices_c, get_indices_e
  USE mo_impl_constants,    ONLY: min_rlcell_int, min_rledge_int, min_rlvert_int
  USE mo_impl_constants_grf,ONLY: grf_bdywidth_c, grf_bdywidth_e
  USE mo_timer,             ONLY: timer_solve_nh_veltend, timer_start, timer_stop
  IMPLICIT NONE
  PRIVATE
  PUBLIC :: velocity_tendencies
  CONTAINS
  SUBROUTINE velocity_tendencies (p_prog, p_patch, p_int, p_metrics, p_diag, z_w_concorr_me, z_kin_hor_e, &
                                  z_vt_ie, ntnd, istep, lvn_only, dtime, dt_linintp_ubc, ldeepatmo)
    TYPE(t_patch), TARGET, INTENT(IN)    :: p_patch
    TYPE(t_int_state), TARGET, INTENT(IN):: p_int
    TYPE(t_nh_prog), INTENT(INOUT)       :: p_prog
    TYPE(t_nh_metrics), INTENT(INOUT)    :: p_metrics
    TYPE(t_nh_diag), INTENT(INOUT)       :: p_diag
    REAL(vp), DIMENSION(:,:,:), INTENT(INOUT) :: z_w_concorr_me, z_kin_hor_e, z_vt_ie
    INTEGER, INTENT(IN)  :: ntnd     ! time level of ddt fields used to store tendencies
    INTEGER, INTENT(IN)  :: istep    ! 1: predictor step, 2: corrector step
    LOGICAL, INTENT(IN)  :: lvn_only ! true: compute only vn tendency
    REAL(wp),INTENT(IN)  :: dtime    ! time step
    REAL(wp),INTENT(IN)  :: dt_linintp_ubc  ! time shift for upper boundary condition
    LOGICAL, INTENT(IN)  :: ldeepatmo! true: deep-atmosphere modification
    INTEGER :: jb, jk, jc, je
    INTEGER :: i_startblk, i_endblk, i_startidx, i_endidx
    INTEGER :: i_startblk_2, i_endblk_2, i_startidx_2, i_endidx_2
    INTEGER :: rl_start, rl_end, rl_start_2, rl_end_2
    REAL(vp):: z_w_concorr_mc(nproma,p_patch%nlev)
    REAL(vp):: z_w_con_c(nproma,p_patch%nlevp1)
    REAL(vp):: z_w_con_c_full(nproma,p_patch%nlev,p_patch%nblks_c)
    REAL(vp):: z_v_grad_w(nproma,p_patch%nlev,p_patch%nblks_e)
    REAL(vp):: z_w_v(nproma,p_patch%nlevp1,p_patch%nblks_v)
    REAL(vp):: zeta(nproma,p_patch%nlev,p_patch%nblks_v)
    REAL(vp):: z_ekinh(nproma,p_patch%nlev,p_patch%nblks_c)
    INTEGER, DIMENSION(:,:,:), POINTER, CONTIGUOUS :: &
      icidx, icblk, ieidx, ieblk, iqidx, iqblk, ividx, ivblk, incidx, incblk
    INTEGER  :: nlev, nlevp1          !< number of full and half levels
    LOGICAL :: l_vert_nested
    INTEGER :: jg
    REAL(vp) :: cfl_w_limit, vcfl, maxvcfl, vcflmax(p_patch%nblks_c)
    REAL(wp) :: w_con_e, scalfac_exdiff, difcoef, max_vcfl_dyn
    INTEGER  :: ie, nrdmax_jg, nflatlev_jg, clip_count
    LOGICAL  :: levmask(p_patch%nblks_c,p_patch%nlev),levelmask(p_patch%nlev)
    LOGICAL  :: cfl_clipping(nproma,p_patch%nlevp1)   ! CFL > 0.85
    IF (timers_level > 5) CALL timer_start(timer_solve_nh_veltend)
    IF ((lvert_nest) .AND. (p_patch%nshift > 0)) THEN
      l_vert_nested = .TRUE.
    ELSE
      l_vert_nested = .FALSE.
    ENDIF
    jg = p_patch%id
    nrdmax_jg     = nrdmax(jg)
    nflatlev_jg   = nflatlev(jg)
    nlev   = p_patch%nlev
    nlevp1 = p_patch%nlevp1
    icidx => p_patch%edges%cell_idx
    icblk => p_patch%edges%cell_blk
    ieidx => p_patch%cells%edge_idx
    ieblk => p_patch%cells%edge_blk
    ividx => p_patch%edges%vertex_idx
    ivblk => p_patch%edges%vertex_blk
    incidx => p_patch%cells%neighbor_idx
    incblk => p_patch%cells%neighbor_blk
    iqidx => p_patch%edges%quad_idx
    iqblk => p_patch%edges%quad_blk
    IF (lextra_diffu) THEN
      cfl_w_limit = 0.65_wp/dtime   ! this means 65% of the nominal CFL stability limit
      scalfac_exdiff = 0.05_wp / ( dtime*(0.85_wp - cfl_w_limit*dtime) )
    ELSE
      cfl_w_limit = 0.85_wp/dtime   ! this means 65% of the nominal CFL stability limit
      scalfac_exdiff = 0._wp
    ENDIF
    IF (.NOT. lvn_only) CALL cells2verts_scalar_ri(p_prog%w, p_patch, &
      p_int%cells_aw_verts, z_w_v, lacc=.TRUE., opt_rlend=min_rlvert_int-1, opt_acc_async=.TRUE.)
    CALL rot_vertex_ri (p_prog%vn, p_patch, p_int, zeta, lacc=.TRUE.,   &
                        opt_rlend=min_rlvert_int-1, opt_acc_async=.TRUE.)
    IF (istep == 1) THEN ! Computations of velocity-derived quantities that come from solve_nh in istep=2
      rl_start = 5
      rl_end = min_rledge_int - 2
      i_startblk = p_patch%edges%start_block(rl_start)
      i_endblk   = p_patch%edges%end_block(rl_end)
      DO jb = i_startblk, i_endblk
        CALL get_indices_e(p_patch, jb, i_startblk, i_endblk, &
                           i_startidx, i_endidx, rl_start, rl_end)
        DO jk = 1, nlev
          DO je = i_startidx, i_endidx
            p_diag%vt(je,jk,jb) = &
              p_int%rbf_vec_coeff_e(1,je,jb) * p_prog%vn(iqidx(je,jb,1),jk,iqblk(je,jb,1)) + &
              p_int%rbf_vec_coeff_e(2,je,jb) * p_prog%vn(iqidx(je,jb,2),jk,iqblk(je,jb,2)) + &
              p_int%rbf_vec_coeff_e(3,je,jb) * p_prog%vn(iqidx(je,jb,3),jk,iqblk(je,jb,3)) + &
              p_int%rbf_vec_coeff_e(4,je,jb) * p_prog%vn(iqidx(je,jb,4),jk,iqblk(je,jb,4))
            IF (jk >= 2) THEN
            p_diag%vn_ie(je,jk,jb) =                                    &
              p_metrics%wgtfac_e(je,jk,jb)*p_prog%vn(je,jk,jb) +        &
             (1._wp - p_metrics%wgtfac_e(je,jk,jb))*p_prog%vn(je,jk-1,jb)
            z_kin_hor_e(je,jk,jb) = 0.5_wp*(p_prog%vn(je,jk,jb)**2 + p_diag%vt(je,jk,jb)**2)
            ENDIF
          ENDDO
        ENDDO
        IF (.NOT. lvn_only) THEN ! Interpolate also vt to interface levels
          DO jk = 2, nlev
            DO je = i_startidx, i_endidx
              z_vt_ie(je,jk,jb) =                                         &
                p_metrics%wgtfac_e(je,jk,jb)*p_diag%vt(je,jk,jb) +        &
               (1._wp - p_metrics%wgtfac_e(je,jk,jb))*p_diag%vt(je,jk-1,jb)
            ENDDO
          ENDDO
        ENDIF
        DO jk = nflatlev_jg, nlev
          DO je = i_startidx, i_endidx
            z_w_concorr_me(je,jk,jb) =                              &
              p_prog%vn(je,jk,jb)*p_metrics%ddxn_z_full(je,jk,jb) + &
              p_diag%vt(je,jk,jb)*p_metrics%ddxt_z_full(je,jk,jb)
          ENDDO
        ENDDO
        IF (.NOT. l_vert_nested) THEN
          DO je = i_startidx, i_endidx
            p_diag%vn_ie(je,1,jb) = p_prog%vn(je,1,jb)
            z_vt_ie(je,1,jb) = p_diag%vt(je,1,jb)
            z_kin_hor_e(je,1,jb) = 0.5_wp*(p_prog%vn(je,1,jb)**2 + p_diag%vt(je,1,jb)**2)
            p_diag%vn_ie(je,nlevp1,jb) =                           &
              p_metrics%wgtfacq_e(je,1,jb)*p_prog%vn(je,nlev,jb) +   &
              p_metrics%wgtfacq_e(je,2,jb)*p_prog%vn(je,nlev-1,jb) + &
              p_metrics%wgtfacq_e(je,3,jb)*p_prog%vn(je,nlev-2,jb)
          ENDDO
        ELSE
          DO je = i_startidx, i_endidx
            p_diag%vn_ie(je,1,jb) = p_diag%vn_ie_ubc(je,1,jb)+dt_linintp_ubc*p_diag%vn_ie_ubc(je,2,jb)
            z_vt_ie(je,1,jb) = p_diag%vt(je,1,jb)
            z_kin_hor_e(je,1,jb) = 0.5_wp*(p_prog%vn(je,1,jb)**2 + p_diag%vt(je,1,jb)**2)
            p_diag%vn_ie(je,nlevp1,jb) =                           &
              p_metrics%wgtfacq_e(je,1,jb)*p_prog%vn(je,nlev,jb) +   &
              p_metrics%wgtfacq_e(je,2,jb)*p_prog%vn(je,nlev-1,jb) + &
              p_metrics%wgtfacq_e(je,3,jb)*p_prog%vn(je,nlev-2,jb)
          ENDDO
        ENDIF
      ENDDO
    ENDIF ! istep = 1
    rl_start = 7
    rl_end = min_rledge_int - 1
    i_startblk = p_patch%edges%start_block(rl_start)
    i_endblk   = p_patch%edges%end_block(rl_end)
    IF (.NOT. lvn_only) THEN
      DO jb = i_startblk, i_endblk
        CALL get_indices_e(p_patch, jb, i_startblk, i_endblk, &
                           i_startidx, i_endidx, rl_start, rl_end)
        DO jk = 1, nlev
          DO je = i_startidx, i_endidx
            z_v_grad_w(je,jk,jb) = p_diag%vn_ie(je,jk,jb) * p_patch%edges%inv_dual_edge_length(je,jb)* &
             (p_prog%w(icidx(je,jb,1),jk,icblk(je,jb,1)) - p_prog%w(icidx(je,jb,2),jk,icblk(je,jb,2))) &
             + z_vt_ie(je,jk,jb) * p_patch%edges%inv_primal_edge_length(je,jb) *                       &
             p_patch%edges%tangent_orientation(je,jb) *                                                 &
             (z_w_v(ividx(je,jb,1),jk,ivblk(je,jb,1)) - z_w_v(ividx(je,jb,2),jk,ivblk(je,jb,2)))
          ENDDO
        ENDDO
      ENDDO
    ENDIF
    IF (.NOT. lvn_only .AND. ldeepatmo) THEN
      DO jb = i_startblk, i_endblk
        CALL get_indices_e(p_patch, jb, i_startblk, i_endblk, i_startidx, i_endidx, rl_start, rl_end)
        DO jk = 1, nlev
          DO je = i_startidx, i_endidx
            z_v_grad_w(je,jk,jb) = z_v_grad_w(je,jk,jb) * p_metrics%deepatmo_gradh_ifc(jk)              &
             + p_diag%vn_ie(je,jk,jb) *                                                                 &
               ( p_diag%vn_ie(je,jk,jb) * p_metrics%deepatmo_invr_ifc(jk) - p_patch%edges%ft_e(je,jb) ) &
             + z_vt_ie(je,jk,jb) *                                                                      &
               ( z_vt_ie(je,jk,jb) * p_metrics%deepatmo_invr_ifc(jk) + p_patch%edges%fn_e(je,jb) )
          ENDDO
        ENDDO
      ENDDO
    ENDIF
    rl_start = 4
    rl_end = min_rlcell_int - 1
    i_startblk = p_patch%cells%start_block(rl_start)
    i_endblk   = p_patch%cells%end_block(rl_end)
    rl_start_2 = grf_bdywidth_c+1
    rl_end_2   = min_rlcell_int
    i_startblk_2 = p_patch%cells%start_block(rl_start_2)
    i_endblk_2   = p_patch%cells%end_block(rl_end_2)
    DO jb = i_startblk, i_endblk
      CALL get_indices_c(p_patch, jb, i_startblk, i_endblk, &
                         i_startidx, i_endidx, rl_start, rl_end)
      DO jk = 1, nlev
        DO jc = i_startidx, i_endidx
        z_ekinh(jc,jk,jb) =  &
          p_int%e_bln_c_s(jc,1,jb)*z_kin_hor_e(ieidx(jc,jb,1),jk,ieblk(jc,jb,1)) + &
          p_int%e_bln_c_s(jc,2,jb)*z_kin_hor_e(ieidx(jc,jb,2),jk,ieblk(jc,jb,2)) + &
          p_int%e_bln_c_s(jc,3,jb)*z_kin_hor_e(ieidx(jc,jb,3),jk,ieblk(jc,jb,3))
        ENDDO
      ENDDO
      IF (istep == 1) THEN
        DO jk = nflatlev_jg, nlev
          DO jc = i_startidx, i_endidx
            z_w_concorr_mc(jc,jk) =  &
              p_int%e_bln_c_s(jc,1,jb)*z_w_concorr_me(ieidx(jc,jb,1),jk,ieblk(jc,jb,1)) + &
              p_int%e_bln_c_s(jc,2,jb)*z_w_concorr_me(ieidx(jc,jb,2),jk,ieblk(jc,jb,2)) + &
              p_int%e_bln_c_s(jc,3,jb)*z_w_concorr_me(ieidx(jc,jb,3),jk,ieblk(jc,jb,3))
          ENDDO
        ENDDO
        DO jk = nflatlev_jg+1, nlev
          DO jc = i_startidx, i_endidx
            p_diag%w_concorr_c(jc,jk,jb) =                                &
              p_metrics%wgtfac_c(jc,jk,jb)*z_w_concorr_mc(jc,jk) +        &
             (1._vp - p_metrics%wgtfac_c(jc,jk,jb))*z_w_concorr_mc(jc,jk-1)
          ENDDO
        ENDDO
      ENDIF
      DO jk = 1, nlev
        DO jc = i_startidx, i_endidx
          z_w_con_c(jc,jk) =  p_prog%w(jc,jk,jb)
        ENDDO
      ENDDO
      DO jc = i_startidx, i_endidx
        z_w_con_c(jc,nlevp1) = 0.0_vp
      ENDDO
      DO jk = nlev, nflatlev_jg+1, -1
        DO jc = i_startidx, i_endidx
          z_w_con_c(jc,jk) = z_w_con_c(jc,jk) - p_diag%w_concorr_c(jc,jk,jb)
        ENDDO
      ENDDO
      DO jk = MAX(3,nrdmax_jg-2), nlev-3
        levmask(jb,jk) = .FALSE.
      ENDDO
        maxvcfl = 0
      DO jk = MAX(3,nrdmax_jg-2), nlev-3
        DO jc = i_startidx, i_endidx
          cfl_clipping(jc,jk) = (ABS(z_w_con_c(jc,jk)) > cfl_w_limit*p_metrics%ddqz_z_half(jc,jk,jb))
          IF ( cfl_clipping(jc,jk) ) THEN
            levmask(jb,jk) = .TRUE.
            vcfl = z_w_con_c(jc,jk)*dtime/p_metrics%ddqz_z_half(jc,jk,jb)
            maxvcfl = MAX( maxvcfl, ABS( vcfl ) )
            IF (vcfl < -0.85_vp) THEN
              z_w_con_c(jc,jk)           = -0.85_vp*p_metrics%ddqz_z_half(jc,jk,jb)/dtime
            ELSE IF (vcfl > 0.85_vp) THEN
              z_w_con_c(jc,jk)           = 0.85_vp*p_metrics%ddqz_z_half(jc,jk,jb)/dtime
            ENDIF
          ENDIF
        ENDDO
      ENDDO
      DO jk = 1, nlev
        DO jc = i_startidx, i_endidx
          z_w_con_c_full(jc,jk,jb) = 0.5_vp*(z_w_con_c(jc,jk)+z_w_con_c(jc,jk+1))
        ENDDO
      ENDDO
      vcflmax(jb) = maxvcfl
      IF (lvn_only) CYCLE
      IF (jb < i_startblk_2 .OR. jb > i_endblk_2) CYCLE
      CALL get_indices_c(p_patch, jb, i_startblk_2, i_endblk_2, &
                         i_startidx_2, i_endidx_2, rl_start_2, rl_end_2)
      DO jk = 2, nlev
        DO jc = i_startidx_2, i_endidx_2
          p_diag%ddt_w_adv_pc(jc,jk,jb,ntnd) =  - z_w_con_c(jc,jk)*                                 &
            (p_prog%w(jc,jk-1,jb)*p_metrics%coeff1_dwdz(jc,jk,jb) -                                 &
             p_prog%w(jc,jk+1,jb)*p_metrics%coeff2_dwdz(jc,jk,jb) +                                 &
             p_prog%w(jc,jk,jb)*(p_metrics%coeff2_dwdz(jc,jk,jb) - p_metrics%coeff1_dwdz(jc,jk,jb)) )
        ENDDO
      ENDDO
      DO jk = 2, nlev
        DO jc = i_startidx_2, i_endidx_2
          p_diag%ddt_w_adv_pc(jc,jk,jb,ntnd) = p_diag%ddt_w_adv_pc(jc,jk,jb,ntnd) + &
            p_int%e_bln_c_s(jc,1,jb)*z_v_grad_w(ieidx(jc,jb,1),jk,ieblk(jc,jb,1)) + &
            p_int%e_bln_c_s(jc,2,jb)*z_v_grad_w(ieidx(jc,jb,2),jk,ieblk(jc,jb,2)) + &
            p_int%e_bln_c_s(jc,3,jb)*z_v_grad_w(ieidx(jc,jb,3),jk,ieblk(jc,jb,3))
        ENDDO
      ENDDO
      IF (lextra_diffu) THEN
        DO jk = MAX(3,nrdmax_jg-2), nlev-3
          IF (levmask(jb,jk)) THEN
            DO jc = i_startidx_2, i_endidx_2
              IF (cfl_clipping(jc,jk) .AND. p_patch%cells%decomp_info%owner_mask(jc,jb)) THEN
                difcoef = scalfac_exdiff * MIN(0.85_wp - cfl_w_limit*dtime,                       &
                  ABS(z_w_con_c(jc,jk))*dtime/p_metrics%ddqz_z_half(jc,jk,jb) - cfl_w_limit*dtime )
                p_diag%ddt_w_adv_pc(jc,jk,jb,ntnd) = p_diag%ddt_w_adv_pc(jc,jk,jb,ntnd)  + &
                  difcoef * p_patch%cells%area(jc,jb) * (                                  &
                  p_prog%w(jc,jk,jb)                          *p_int%geofac_n2s(jc,1,jb) + &
                  p_prog%w(incidx(jc,jb,1),jk,incblk(jc,jb,1))*p_int%geofac_n2s(jc,2,jb) + &
                  p_prog%w(incidx(jc,jb,2),jk,incblk(jc,jb,2))*p_int%geofac_n2s(jc,3,jb) + &
                  p_prog%w(incidx(jc,jb,3),jk,incblk(jc,jb,3))*p_int%geofac_n2s(jc,4,jb)   )
              ENDIF
            ENDDO
          ENDIF
        ENDDO
      ENDIF
    ENDDO
    DO jk = MAX(3,nrdmax_jg-2), nlev-3
      levelmask(jk) = ANY(levmask(i_startblk:i_endblk,jk))
    ENDDO
    rl_start = grf_bdywidth_e+1
    rl_end = min_rledge_int
    i_startblk = p_patch%edges%start_block(rl_start)
    i_endblk   = p_patch%edges%end_block(rl_end)
    DO jb = i_startblk, i_endblk
      CALL get_indices_e(p_patch, jb, i_startblk, i_endblk, &
                         i_startidx, i_endidx, rl_start, rl_end)
      IF (.NOT. ldeepatmo) THEN
        DO jk = 1, nlev
          DO je = i_startidx, i_endidx
            p_diag%ddt_vn_apc_pc(je,jk,jb,ntnd) = - ( z_kin_hor_e(je,jk,jb) *                     &
             (p_metrics%coeff_gradekin(je,1,jb) - p_metrics%coeff_gradekin(je,2,jb)) +            &
              p_metrics%coeff_gradekin(je,2,jb)*z_ekinh(icidx(je,jb,2),jk,icblk(je,jb,2)) -       &
              p_metrics%coeff_gradekin(je,1,jb)*z_ekinh(icidx(je,jb,1),jk,icblk(je,jb,1)) +       &
              p_diag%vt(je,jk,jb) * ( p_patch%edges%f_e(je,jb) + 0.5_vp*                          &
             (zeta(ividx(je,jb,1),jk,ivblk(je,jb,1)) + zeta(ividx(je,jb,2),jk,ivblk(je,jb,2)))) + &
             (p_int%c_lin_e(je,1,jb)*z_w_con_c_full(icidx(je,jb,1),jk,icblk(je,jb,1)) +           &
              p_int%c_lin_e(je,2,jb)*z_w_con_c_full(icidx(je,jb,2),jk,icblk(je,jb,2)))*           &
             (p_diag%vn_ie(je,jk,jb) - p_diag%vn_ie(je,jk+1,jb))/p_metrics%ddqz_z_full_e(je,jk,jb))
          ENDDO
        ENDDO
        IF (p_diag%ddt_vn_adv_is_associated .OR. p_diag%ddt_vn_cor_is_associated) THEN
          DO jk = 1, nlev
            DO je = i_startidx, i_endidx
              p_diag%ddt_vn_cor_pc(je,jk,jb,ntnd) = - p_diag%vt(je,jk,jb) * p_patch%edges%f_e(je,jb)
            ENDDO
          ENDDO
        END IF
      ELSE
        DO jk = 1, nlev
          DO je = i_startidx, i_endidx
            p_diag%ddt_vn_apc_pc(je,jk,jb,ntnd) = - (                                             &
               (                                                                                  &
                z_kin_hor_e(je,jk,jb) *                                                           &
                (p_metrics%coeff_gradekin(je,1,jb) - p_metrics%coeff_gradekin(je,2,jb)) +         &
                p_metrics%coeff_gradekin(je,2,jb)*z_ekinh(icidx(je,jb,2),jk,icblk(je,jb,2)) -     &
                p_metrics%coeff_gradekin(je,1,jb)*z_ekinh(icidx(je,jb,1),jk,icblk(je,jb,1))       &
               ) * p_metrics%deepatmo_gradh_mc(jk)                                                &
               + p_diag%vt(je,jk,jb) * ( p_patch%edges%f_e(je,jb) + 0.5_vp*                       &
               (zeta(ividx(je,jb,1),jk,ivblk(je,jb,1)) + zeta(ividx(je,jb,2),jk,ivblk(je,jb,2)))  &
               * p_metrics%deepatmo_gradh_mc(jk) )                                                &
               + (p_int%c_lin_e(je,1,jb)*z_w_con_c_full(icidx(je,jb,1),jk,icblk(je,jb,1)) +       &
               p_int%c_lin_e(je,2,jb)*z_w_con_c_full(icidx(je,jb,2),jk,icblk(je,jb,2))) *         &
               (                                                                                  &
               (p_diag%vn_ie(je,jk,jb) - p_diag%vn_ie(je,jk+1,jb))/p_metrics%ddqz_z_full_e(je,jk,jb) &
               + p_prog%vn(je,jk,jb) * p_metrics%deepatmo_invr_mc(jk)                             &
               - p_patch%edges%ft_e(je,jb)                                                        &
               )                                                                                  &
              )
          ENDDO
        ENDDO
        IF (p_diag%ddt_vn_adv_is_associated .OR. p_diag%ddt_vn_cor_is_associated) THEN
          DO jk = 1, nlev
            DO je = i_startidx, i_endidx
              p_diag%ddt_vn_cor_pc(je,jk,jb,ntnd) = - (                                        &
                 + p_diag%vt(je,jk,jb) * ( p_patch%edges%f_e(je,jb) )                          &
                 + (p_int%c_lin_e(je,1,jb)*z_w_con_c_full(icidx(je,jb,1),jk,icblk(je,jb,1)) +  &
                 p_int%c_lin_e(je,2,jb)*z_w_con_c_full(icidx(je,jb,2),jk,icblk(je,jb,2))) *    &
                 (                                                                             &
                 - p_patch%edges%ft_e(je,jb)                                                   &
                 )                                                                             &
                )
            ENDDO
          ENDDO
        END IF
      ENDIF
      IF (lextra_diffu) THEN
        ie = 0
        DO jk = MAX(3,nrdmax_jg-2), nlev-4
          IF (levelmask(jk) .OR. levelmask(jk+1)) THEN
            DO je = i_startidx, i_endidx
              w_con_e = p_int%c_lin_e(je,1,jb)*z_w_con_c_full(icidx(je,jb,1),jk,icblk(je,jb,1)) + &
                        p_int%c_lin_e(je,2,jb)*z_w_con_c_full(icidx(je,jb,2),jk,icblk(je,jb,2))
              IF (ABS(w_con_e) > cfl_w_limit*p_metrics%ddqz_z_full_e(je,jk,jb)) THEN
                difcoef = scalfac_exdiff * MIN(0.85_wp - cfl_w_limit*dtime,                &
                  ABS(w_con_e)*dtime/p_metrics%ddqz_z_full_e(je,jk,jb) - cfl_w_limit*dtime )
                p_diag%ddt_vn_apc_pc(je,jk,jb,ntnd) = p_diag%ddt_vn_apc_pc(je,jk,jb,ntnd) +             &
                  difcoef * p_patch%edges%area_edge(je,jb) * (                                          &
                  p_int%geofac_grdiv(je,1,jb)*p_prog%vn(je,jk,jb)                         +             &
                  p_int%geofac_grdiv(je,2,jb)*p_prog%vn(iqidx(je,jb,1),jk,iqblk(je,jb,1)) +             &
                  p_int%geofac_grdiv(je,3,jb)*p_prog%vn(iqidx(je,jb,2),jk,iqblk(je,jb,2)) +             &
                  p_int%geofac_grdiv(je,4,jb)*p_prog%vn(iqidx(je,jb,3),jk,iqblk(je,jb,3)) +             &
                  p_int%geofac_grdiv(je,5,jb)*p_prog%vn(iqidx(je,jb,4),jk,iqblk(je,jb,4)) +             &
                  p_patch%edges%tangent_orientation(je,jb)*p_patch%edges%inv_primal_edge_length(je,jb) * &
                  (zeta(ividx(je,jb,2),jk,ivblk(je,jb,2)) - zeta(ividx(je,jb,1),jk,ivblk(je,jb,1))) )
              ENDIF
            ENDDO
          ENDIF
        ENDDO
      ENDIF
    ENDDO
    i_startblk = p_patch%cells%start_block(grf_bdywidth_c)
    i_endblk   = p_patch%cells%end_block(min_rlcell_int)
    max_vcfl_dyn = MAX(p_diag%max_vcfl_dyn,MAXVAL(vcflmax(i_startblk:i_endblk)))
    p_diag%max_vcfl_dyn = max_vcfl_dyn
    IF (timers_level > 5) CALL timer_stop(timer_solve_nh_veltend)
  END SUBROUTINE velocity_tendencies
     SUBROUTINE h2d_velocity_tendencies( ntnd, p_prog, p_diag, z_w_concorr_me, z_kin_hor_e, z_vt_ie )
       INTEGER, INTENT(IN)                       :: ntnd
       TYPE(t_nh_prog), INTENT(INOUT)            :: p_prog
       TYPE(t_nh_diag), INTENT(INOUT)            :: p_diag
       REAL(vp), DIMENSION(:,:,:), INTENT(INOUT) :: z_w_concorr_me, z_kin_hor_e, z_vt_ie
       REAL(wp), DIMENSION(:,:,:),   POINTER  :: vn_tmp, w_tmp
       REAL(vp), DIMENSION(:,:,:),   POINTER  :: vt_tmp, vn_ie_tmp
       REAL(vp), DIMENSION(:,:,:),   POINTER  :: w_concorr_c_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_vn_apc_pc_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_vn_cor_pc_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_w_adv_pc_tmp
       vn_tmp              => p_prog%vn
       w_tmp               => p_prog%w
       vt_tmp              => p_diag%vt
       vn_ie_tmp           => p_diag%vn_ie
       w_concorr_c_tmp     => p_diag%w_concorr_c
       ddt_vn_apc_pc_tmp   => p_diag%ddt_vn_apc_pc
       ddt_w_adv_pc_tmp    => p_diag%ddt_w_adv_pc
       IF (p_diag%ddt_vn_adv_is_associated .OR. p_diag%ddt_vn_cor_is_associated) THEN
          ddt_vn_cor_pc_tmp   => p_diag%ddt_vn_cor_pc
       END IF
     END SUBROUTINE h2d_velocity_tendencies
     SUBROUTINE d2h_velocity_tendencies( istep, ntnd, p_diag, z_w_concorr_me, z_kin_hor_e, z_vt_ie )
       INTEGER, INTENT(IN)                       :: istep, ntnd
       TYPE(t_nh_diag), INTENT(INOUT)            :: p_diag
       REAL(vp), DIMENSION(:,:,:), INTENT(INOUT) :: z_w_concorr_me, z_kin_hor_e, z_vt_ie
       REAL(vp), DIMENSION(:,:,:),   POINTER  :: vt_tmp, vn_ie_tmp
       REAL(vp), DIMENSION(:,:,:),   POINTER  :: w_concorr_c_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_vn_apc_pc_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_vn_cor_pc_tmp
       REAL(vp), DIMENSION(:,:,:,:), POINTER  :: ddt_w_adv_pc_tmp
       vt_tmp              => p_diag%vt
       vn_ie_tmp           => p_diag%vn_ie
       w_concorr_c_tmp     => p_diag%w_concorr_c
       ddt_vn_apc_pc_tmp   => p_diag%ddt_vn_apc_pc
       ddt_w_adv_pc_tmp    => p_diag%ddt_w_adv_pc
       IF (p_diag%ddt_vn_adv_is_associated .OR. p_diag%ddt_vn_cor_is_associated) THEN
          ddt_vn_cor_pc_tmp   => p_diag%ddt_vn_cor_pc
       END IF
     END SUBROUTINE d2h_velocity_tendencies
END MODULE mo_velocity_advection
