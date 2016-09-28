/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_store_direct 1
#define HAVE_unary_comparison 1
#define HAVE_addsi_compare_2 1
#define HAVE_abssi2 1
#define HAVE_smaxsi3 1
#define HAVE_sminsi3 1
#define HAVE_mulhisi3_imm (EM_MUL_MPYW)
#define HAVE_mulhisi3_reg (EM_MUL_MPYW)
#define HAVE_umulhisi3_imm (EM_MUL_MPYW)
#define HAVE_umulhisi3_reg (EM_MUL_MPYW)
#define HAVE_umul_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_mac_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_mulsi_600 (TARGET_MUL64_SET)
#define HAVE_mulsi3_600_lib (!TARGET_MUL64_SET && !TARGET_MULMAC_32BY16_SET \
   && (!TARGET_ARC700 || TARGET_NOMPY_SET) \
   && SFUNC_CHECK_PREDICABLE \
   && (!EM_MULTI))
#define HAVE_mulsidi_600 (TARGET_MUL64_SET)
#define HAVE_umulsidi_600 (TARGET_MUL64_SET)
#define HAVE_mulsi3_700 ((TARGET_ARC700 && !TARGET_NOMPY_SET) || EM_MULTI)
#define HAVE_mul64_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_mac64_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_mulsidi3_700 ((TARGET_ARC700 && !TARGET_NOMPY_SET) || EM_MULTI)
#define HAVE_mulsi3_highpart ((TARGET_ARC700 && !TARGET_NOMPY_SET) || EM_MULTI)
#define HAVE_umulsi3_highpart_600_lib_le (!TARGET_BIG_ENDIAN \
   && !TARGET_MUL64_SET && !TARGET_MULMAC_32BY16_SET \
   && (!TARGET_ARC700 || TARGET_NOMPY_SET) \
   && SFUNC_CHECK_PREDICABLE)
#define HAVE_umulsi3_highpart_600_lib_be (TARGET_BIG_ENDIAN \
   && !TARGET_MUL64_SET && !TARGET_MULMAC_32BY16_SET \
   && (!TARGET_ARC700 || TARGET_NOMPY_SET) \
   && SFUNC_CHECK_PREDICABLE)
#define HAVE_umulsi3_highpart_int ((TARGET_ARC700 && !TARGET_NOMPY_SET) || EM_MULTI)
#define HAVE_umul64_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_umac64_600 (TARGET_MULMAC_32BY16_SET)
#define HAVE_umulsidi3_700 ((TARGET_ARC700 && !TARGET_NOMPY_SET) || EM_MULTI)
#define HAVE_umulsidi3_600_lib (!TARGET_MUL64_SET && !TARGET_MULMAC_32BY16_SET \
   && (!TARGET_ARC700 || TARGET_NOMPY_SET) \
   && SFUNC_CHECK_PREDICABLE)
#define HAVE_add_f 1
#define HAVE_adc (register_operand (operands[1], SImode) \
   || register_operand (operands[2], SImode))
#define HAVE_subsi3_insn (register_operand (operands[1], SImode) \
   || register_operand (operands[2], SImode))
#define HAVE_subdi3_i (register_operand (operands[1], DImode) \
   || register_operand (operands[2], DImode))
#define HAVE_sbc (register_operand (operands[1], SImode) \
   || register_operand (operands[2], SImode))
#define HAVE_sub_f (register_operand (operands[1], SImode) \
   || register_operand (operands[2], SImode))
#define HAVE_andsi3_i ((register_operand (operands[1], SImode) \
    && nonmemory_operand (operands[2], SImode)) \
   || (memory_operand (operands[1], SImode) \
       && satisfies_constraint_Cux (operands[2])))
#define HAVE_iorsi3 1
#define HAVE_xorsi3 1
#define HAVE_negsi2 1
#define HAVE_one_cmplsi2 1
#define HAVE_one_cmpldi2 1
#define HAVE_shift_si3 (!TARGET_BARREL_SHIFTER)
#define HAVE_shift_si3_loop (!TARGET_BARREL_SHIFTER)
#define HAVE_cmpsi_cc_insn_mixed 1
#define HAVE_jump_i (!TARGET_LONG_CALLS_SET || !find_reg_note (insn, REG_CROSSING_JUMP, NULL_RTX))
#define HAVE_indirect_jump 1
#define HAVE_casesi_load 1
#define HAVE_casesi_jump 1
#define HAVE_casesi_compact_jump (TARGET_COMPACT_CASESI)
#define HAVE_call_prof 1
#define HAVE_call_value_prof 1
#define HAVE_nop 1
#define HAVE_flush_icache 1
#define HAVE_norm (TARGET_NORM)
#define HAVE_norm_f (TARGET_NORM)
#define HAVE_normw (TARGET_NORM)
#define HAVE_swap (TARGET_SWAP)
#define HAVE_mul64 (TARGET_MUL64_SET)
#define HAVE_mulu64 (TARGET_MUL64_SET)
#define HAVE_divaw ((TARGET_ARC700 || TARGET_EA_SET) && !TARGET_EM)
#define HAVE_flag 1
#define HAVE_brk 1
#define HAVE_rtie 1
#define HAVE_sync 1
#define HAVE_swi 1
#define HAVE_sleep (check_if_valid_sleep_operand(operands,0))
#define HAVE_core_read 1
#define HAVE_core_write 1
#define HAVE_lr 1
#define HAVE_sr 1
#define HAVE_trap_s (TARGET_ARC700 || TARGET_EM)
#define HAVE_unimp_s (TARGET_ARC700 || TARGET_EM)
#define HAVE_kflag (TARGET_EM)
#define HAVE_clri (TARGET_EM)
#define HAVE_ffs (TARGET_NORM && TARGET_EM)
#define HAVE_ffs_f (TARGET_NORM && TARGET_EM)
#define HAVE_fls (TARGET_NORM && TARGET_EM)
#define HAVE_seti (TARGET_EM)
#define HAVE_sibcall_prof 1
#define HAVE_sibcall_value_prof 1
#define HAVE_simple_return (reload_completed)
#define HAVE_p_return_i (reload_completed)
#define HAVE_eh_return 1
#define HAVE_cbranchsi4_scratch ((reload_completed \
     || (TARGET_EARLY_CBRANCHSI \
	 && brcc_nolimm_operator (operands[0], VOIDmode))) \
    && !find_reg_note (insn, REG_CROSSING_JUMP, NULL_RTX))
#define HAVE_doloop_begin_i 1
#define HAVE_doloop_end_i 1
#define HAVE_doloop_fallback_m (reload_completed)
#define HAVE_abssf2 1
#define HAVE_negsf2 1
#define HAVE_bswapsi2 (TARGET_EM && TARGET_SWAP)
#define HAVE_prefetch_1 (TARGET_EM)
#define HAVE_prefetch_2 (TARGET_EM)
#define HAVE_prefetch_3 (TARGET_EM)
#define HAVE_divsi3 (TARGET_DIVREM)
#define HAVE_udivsi3 (TARGET_DIVREM)
#define HAVE_modsi3 (TARGET_DIVREM)
#define HAVE_umodsi3 (TARGET_DIVREM)
#define HAVE_addsf3 (TARGET_SPFP)
#define HAVE_subsf3 (TARGET_SPFP)
#define HAVE_mulsf3 (TARGET_SPFP)
#define HAVE_cmpsfpx_raw (TARGET_ARGONAUT_SET && TARGET_SPFP)
#define HAVE_cmpdfpx_raw (TARGET_ARGONAUT_SET && TARGET_DPFP)
#define HAVE_adddf3_insn (TARGET_DPFP && \
   !(GET_CODE(operands[2]) == CONST_DOUBLE && GET_CODE(operands[3]) == CONST_INT))
#define HAVE_muldf3_insn (TARGET_DPFP && \
   !(GET_CODE(operands[2]) == CONST_DOUBLE && GET_CODE(operands[3]) == CONST_INT))
#define HAVE_subdf3_insn (TARGET_DPFP && \
   !(GET_CODE(operands[2]) == CONST_DOUBLE && GET_CODE(operands[3]) == CONST_INT) && \
   !(GET_CODE(operands[1]) == CONST_DOUBLE && GET_CODE(operands[3]) == CONST_INT))
#define HAVE_vld128_insn (TARGET_SIMD_SET)
#define HAVE_vst128_insn (TARGET_SIMD_SET)
#define HAVE_vst64_insn (TARGET_SIMD_SET)
#define HAVE_movv8hi_insn (TARGET_SIMD_SET && !(GET_CODE (operands[0]) == MEM && GET_CODE(operands[1]) == MEM))
#define HAVE_movti_insn 1
#define HAVE_vaddaw_insn (TARGET_SIMD_SET)
#define HAVE_vaddw_insn (TARGET_SIMD_SET)
#define HAVE_vavb_insn (TARGET_SIMD_SET)
#define HAVE_vavrb_insn (TARGET_SIMD_SET)
#define HAVE_vdifaw_insn (TARGET_SIMD_SET)
#define HAVE_vdifw_insn (TARGET_SIMD_SET)
#define HAVE_vmaxaw_insn (TARGET_SIMD_SET)
#define HAVE_vmaxw_insn (TARGET_SIMD_SET)
#define HAVE_vminaw_insn (TARGET_SIMD_SET)
#define HAVE_vminw_insn (TARGET_SIMD_SET)
#define HAVE_vmulaw_insn (TARGET_SIMD_SET)
#define HAVE_vmulfaw_insn (TARGET_SIMD_SET)
#define HAVE_vmulfw_insn (TARGET_SIMD_SET)
#define HAVE_vmulw_insn (TARGET_SIMD_SET)
#define HAVE_vsubaw_insn (TARGET_SIMD_SET)
#define HAVE_vsubw_insn (TARGET_SIMD_SET)
#define HAVE_vsummw_insn (TARGET_SIMD_SET)
#define HAVE_vand_insn (TARGET_SIMD_SET)
#define HAVE_vandaw_insn (TARGET_SIMD_SET)
#define HAVE_vbic_insn (TARGET_SIMD_SET)
#define HAVE_vbicaw_insn (TARGET_SIMD_SET)
#define HAVE_vor_insn (TARGET_SIMD_SET)
#define HAVE_vxor_insn (TARGET_SIMD_SET)
#define HAVE_vxoraw_insn (TARGET_SIMD_SET)
#define HAVE_veqw_insn (TARGET_SIMD_SET)
#define HAVE_vlew_insn (TARGET_SIMD_SET)
#define HAVE_vltw_insn (TARGET_SIMD_SET)
#define HAVE_vnew_insn (TARGET_SIMD_SET)
#define HAVE_vmr1aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr1w_insn (TARGET_SIMD_SET)
#define HAVE_vmr2aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr2w_insn (TARGET_SIMD_SET)
#define HAVE_vmr3aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr3w_insn (TARGET_SIMD_SET)
#define HAVE_vmr4aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr4w_insn (TARGET_SIMD_SET)
#define HAVE_vmr5aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr5w_insn (TARGET_SIMD_SET)
#define HAVE_vmr6aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr6w_insn (TARGET_SIMD_SET)
#define HAVE_vmr7aw_insn (TARGET_SIMD_SET)
#define HAVE_vmr7w_insn (TARGET_SIMD_SET)
#define HAVE_vmrb_insn (TARGET_SIMD_SET)
#define HAVE_vh264f_insn (TARGET_SIMD_SET)
#define HAVE_vh264ft_insn (TARGET_SIMD_SET)
#define HAVE_vh264fw_insn (TARGET_SIMD_SET)
#define HAVE_vvc1f_insn (TARGET_SIMD_SET)
#define HAVE_vvc1ft_insn (TARGET_SIMD_SET)
#define HAVE_vbaddw_insn (TARGET_SIMD_SET)
#define HAVE_vbmaxw_insn (TARGET_SIMD_SET)
#define HAVE_vbminw_insn (TARGET_SIMD_SET)
#define HAVE_vbmulaw_insn (TARGET_SIMD_SET)
#define HAVE_vbmulfw_insn (TARGET_SIMD_SET)
#define HAVE_vbmulw_insn (TARGET_SIMD_SET)
#define HAVE_vbrsubw_insn (TARGET_SIMD_SET)
#define HAVE_vbsubw_insn (TARGET_SIMD_SET)
#define HAVE_vasrrwi_insn (TARGET_SIMD_SET)
#define HAVE_vasrsrwi_insn (TARGET_SIMD_SET)
#define HAVE_vasrwi_insn (TARGET_SIMD_SET)
#define HAVE_vasrpwbi_insn (TARGET_SIMD_SET)
#define HAVE_vasrrpwbi_insn (TARGET_SIMD_SET)
#define HAVE_vsr8awi_insn (TARGET_SIMD_SET)
#define HAVE_vsr8i_insn (TARGET_SIMD_SET)
#define HAVE_vmvaw_insn (TARGET_SIMD_SET)
#define HAVE_vmvw_insn (TARGET_SIMD_SET)
#define HAVE_vmvzw_insn (TARGET_SIMD_SET)
#define HAVE_vd6tapf_insn (TARGET_SIMD_SET)
#define HAVE_vmovaw_insn (TARGET_SIMD_SET)
#define HAVE_vmovw_insn (TARGET_SIMD_SET)
#define HAVE_vmovzw_insn (TARGET_SIMD_SET)
#define HAVE_vsr8_insn (TARGET_SIMD_SET)
#define HAVE_vasrw_insn (TARGET_SIMD_SET)
#define HAVE_vsr8aw_insn (TARGET_SIMD_SET)
#define HAVE_vabsaw_insn (TARGET_SIMD_SET)
#define HAVE_vabsw_insn (TARGET_SIMD_SET)
#define HAVE_vaddsuw_insn (TARGET_SIMD_SET)
#define HAVE_vsignw_insn (TARGET_SIMD_SET)
#define HAVE_vexch1_insn (TARGET_SIMD_SET)
#define HAVE_vexch2_insn (TARGET_SIMD_SET)
#define HAVE_vexch4_insn (TARGET_SIMD_SET)
#define HAVE_vupbaw_insn (TARGET_SIMD_SET)
#define HAVE_vupbw_insn (TARGET_SIMD_SET)
#define HAVE_vupsbaw_insn (TARGET_SIMD_SET)
#define HAVE_vupsbw_insn (TARGET_SIMD_SET)
#define HAVE_vdirun_insn (TARGET_SIMD_SET)
#define HAVE_vdorun_insn (TARGET_SIMD_SET)
#define HAVE_vdiwr_insn (TARGET_SIMD_SET)
#define HAVE_vdowr_insn (TARGET_SIMD_SET)
#define HAVE_vrec_insn (TARGET_SIMD_SET)
#define HAVE_vrun_insn (TARGET_SIMD_SET)
#define HAVE_vrecrun_insn (TARGET_SIMD_SET)
#define HAVE_vendrec_insn (TARGET_SIMD_SET)
#define HAVE_vld32wh_insn (TARGET_SIMD_SET)
#define HAVE_vld32wl_insn (TARGET_SIMD_SET)
#define HAVE_vld64w_insn (TARGET_SIMD_SET)
#define HAVE_vld64_insn (TARGET_SIMD_SET)
#define HAVE_vld32_insn (TARGET_SIMD_SET)
#define HAVE_vst16_n_insn (TARGET_SIMD_SET)
#define HAVE_vst32_n_insn (TARGET_SIMD_SET)
#define HAVE_vinti_insn (TARGET_SIMD_SET)
#define HAVE_movqi 1
#define HAVE_movhi 1
#define HAVE_movsi 1
#define HAVE_bic_f_zn 1
#define HAVE_movdi 1
#define HAVE_movsf 1
#define HAVE_movdf 1
#define HAVE_movsicc 1
#define HAVE_movdicc 1
#define HAVE_movsfcc 1
#define HAVE_movdfcc 1
#define HAVE_zero_extendqihi2 1
#define HAVE_zero_extendqisi2 1
#define HAVE_zero_extendhisi2 1
#define HAVE_extendqihi2 1
#define HAVE_extendqisi2 1
#define HAVE_extendhisi2 1
#define HAVE_mulhisi3 (EM_MUL_MPYW)
#define HAVE_umulhisi3 (EM_MUL_MPYW)
#define HAVE_mulsi3 1
#define HAVE_mulsidi3 ((TARGET_ARC700 && !TARGET_NOMPY_SET) \
   || EM_MULTI \
   || TARGET_MUL64_SET \
   || TARGET_MULMAC_32BY16_SET)
#define HAVE_umulsi3_highpart (TARGET_ARC700 || EM_MULTI || (!TARGET_MUL64_SET && !TARGET_MULMAC_32BY16_SET))
#define HAVE_umulsidi3 1
#define HAVE_addsi3 1
#define HAVE_adddi3 1
#define HAVE_subsi3 1
#define HAVE_subdi3 1
#define HAVE_andsi3 1
#define HAVE_ashlsi3 1
#define HAVE_ashrsi3 1
#define HAVE_lshrsi3 1
#define HAVE_cbranchsi4 1
#define HAVE_cstoresi4 1
#define HAVE_cstoresf4 (TARGET_OPTFPE)
#define HAVE_cstoredf4 (TARGET_OPTFPE)
#define HAVE_scc_insn 1
#define HAVE_branch_insn 1
#define HAVE_jump 1
#define HAVE_casesi 1
#define HAVE_call 1
#define HAVE_call_value 1
#define HAVE_clzsi2 (TARGET_NORM)
#define HAVE_ctzsi2 (TARGET_NORM)
#define HAVE_ffssi2 (TARGET_NORM && TARGET_EM)
#define HAVE_sibcall 1
#define HAVE_sibcall_value 1
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_sibcall_epilogue 1
#define HAVE_return (optimize < 0)
#define HAVE_doloop_begin 1
#define HAVE_doloop_end (TARGET_ARC600 || TARGET_ARC700 || TARGET_EM)
#define HAVE_movmemsi 1
#define HAVE_cbranchsf4 (TARGET_OPTFPE)
#define HAVE_cbranchdf4 (TARGET_OPTFPE)
#define HAVE_cmp_float 1
#define HAVE_prefetch (TARGET_EM)
#define HAVE_rotlsi3 (TARGET_EM)
#define HAVE_rotrsi3 1
#define HAVE_adddf3 (TARGET_DPFP)
#define HAVE_muldf3 (TARGET_DPFP)
#define HAVE_subdf3 (TARGET_DPFP)
#define HAVE_movv8hi 1
extern rtx        gen_store_direct                (rtx, rtx);
extern rtx        gen_unary_comparison            (rtx, rtx, rtx, rtx);
extern rtx        gen_addsi_compare_2             (rtx, rtx);
extern rtx        gen_abssi2                      (rtx, rtx);
extern rtx        gen_smaxsi3                     (rtx, rtx, rtx);
extern rtx        gen_sminsi3                     (rtx, rtx, rtx);
extern rtx        gen_mulhisi3_imm                (rtx, rtx, rtx);
extern rtx        gen_mulhisi3_reg                (rtx, rtx, rtx);
extern rtx        gen_umulhisi3_imm               (rtx, rtx, rtx);
extern rtx        gen_umulhisi3_reg               (rtx, rtx, rtx);
extern rtx        gen_umul_600                    (rtx, rtx, rtx, rtx);
extern rtx        gen_mac_600                     (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsi_600                   (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsi3_600_lib              (void);
extern rtx        gen_mulsidi_600                 (rtx, rtx);
extern rtx        gen_umulsidi_600                (rtx, rtx);
extern rtx        gen_mulsi3_700                  (rtx, rtx, rtx);
extern rtx        gen_mul64_600                   (rtx, rtx);
extern rtx        gen_mac64_600                   (rtx, rtx, rtx);
extern rtx        gen_mulsidi3_700                (rtx, rtx, rtx);
extern rtx        gen_mulsi3_highpart             (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart_600_lib_le (void);
extern rtx        gen_umulsi3_highpart_600_lib_be (void);
extern rtx        gen_umulsi3_highpart_int        (rtx, rtx, rtx);
extern rtx        gen_umul64_600                  (rtx, rtx);
extern rtx        gen_umac64_600                  (rtx, rtx, rtx);
extern rtx        gen_umulsidi3_700               (rtx, rtx, rtx);
extern rtx        gen_umulsidi3_600_lib           (void);
extern rtx        gen_add_f                       (rtx, rtx, rtx);
extern rtx        gen_adc                         (rtx, rtx, rtx);
extern rtx        gen_subsi3_insn                 (rtx, rtx, rtx);
extern rtx        gen_subdi3_i                    (rtx, rtx, rtx);
extern rtx        gen_sbc                         (rtx, rtx, rtx, rtx);
extern rtx        gen_sub_f                       (rtx, rtx, rtx);
extern rtx        gen_andsi3_i                    (rtx, rtx, rtx);
extern rtx        gen_iorsi3                      (rtx, rtx, rtx);
extern rtx        gen_xorsi3                      (rtx, rtx, rtx);
extern rtx        gen_negsi2                      (rtx, rtx);
extern rtx        gen_one_cmplsi2                 (rtx, rtx);
extern rtx        gen_one_cmpldi2                 (rtx, rtx);
extern rtx        gen_shift_si3                   (rtx, rtx, rtx, rtx);
extern rtx        gen_shift_si3_loop              (rtx, rtx, rtx, rtx);
extern rtx        gen_cmpsi_cc_insn_mixed         (rtx, rtx);
extern rtx        gen_jump_i                      (rtx);
extern rtx        gen_indirect_jump               (rtx);
extern rtx        gen_casesi_load                 (rtx, rtx, rtx, rtx);
extern rtx        gen_casesi_jump                 (rtx, rtx);
extern rtx        gen_casesi_compact_jump         (rtx, rtx);
extern rtx        gen_call_prof                   (rtx, rtx);
extern rtx        gen_call_value_prof             (rtx, rtx, rtx);
extern rtx        gen_nop                         (void);
extern rtx        gen_flush_icache                (rtx);
extern rtx        gen_norm                        (rtx, rtx);
extern rtx        gen_norm_f                      (rtx, rtx);
extern rtx        gen_normw                       (rtx, rtx);
extern rtx        gen_swap                        (rtx, rtx);
extern rtx        gen_mul64                       (rtx, rtx);
extern rtx        gen_mulu64                      (rtx, rtx);
extern rtx        gen_divaw                       (rtx, rtx, rtx);
extern rtx        gen_flag                        (rtx);
extern rtx        gen_brk                         (rtx);
extern rtx        gen_rtie                        (rtx);
extern rtx        gen_sync                        (rtx);
extern rtx        gen_swi                         (rtx);
extern rtx        gen_sleep                       (rtx);
extern rtx        gen_core_read                   (rtx, rtx);
extern rtx        gen_core_write                  (rtx, rtx);
extern rtx        gen_lr                          (rtx, rtx);
extern rtx        gen_sr                          (rtx, rtx);
extern rtx        gen_trap_s                      (rtx);
extern rtx        gen_unimp_s                     (rtx);
extern rtx        gen_kflag                       (rtx);
extern rtx        gen_clri                        (rtx, rtx);
extern rtx        gen_ffs                         (rtx, rtx);
extern rtx        gen_ffs_f                       (rtx, rtx);
extern rtx        gen_fls                         (rtx, rtx);
extern rtx        gen_seti                        (rtx);
extern rtx        gen_sibcall_prof                (rtx, rtx, rtx);
extern rtx        gen_sibcall_value_prof          (rtx, rtx, rtx, rtx);
extern rtx        gen_simple_return               (void);
extern rtx        gen_p_return_i                  (rtx);
extern rtx        gen_eh_return                   (rtx);
extern rtx        gen_cbranchsi4_scratch          (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_doloop_begin_i              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_doloop_end_i                (rtx, rtx, rtx);
extern rtx        gen_doloop_fallback_m           (rtx, rtx, rtx);
extern rtx        gen_abssf2                      (rtx, rtx);
extern rtx        gen_negsf2                      (rtx, rtx);
extern rtx        gen_bswapsi2                    (rtx, rtx);
extern rtx        gen_prefetch_1                  (rtx, rtx, rtx);
extern rtx        gen_prefetch_2                  (rtx, rtx, rtx, rtx);
extern rtx        gen_prefetch_3                  (rtx, rtx, rtx);
extern rtx        gen_divsi3                      (rtx, rtx, rtx);
extern rtx        gen_udivsi3                     (rtx, rtx, rtx);
extern rtx        gen_modsi3                      (rtx, rtx, rtx);
extern rtx        gen_umodsi3                     (rtx, rtx, rtx);
extern rtx        gen_addsf3                      (rtx, rtx, rtx);
extern rtx        gen_subsf3                      (rtx, rtx, rtx);
extern rtx        gen_mulsf3                      (rtx, rtx, rtx);
extern rtx        gen_cmpsfpx_raw                 (rtx, rtx);
extern rtx        gen_cmpdfpx_raw                 (rtx, rtx);
extern rtx        gen_adddf3_insn                 (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_muldf3_insn                 (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_subdf3_insn                 (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vld128_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_vst128_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_vst64_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_movv8hi_insn                (rtx, rtx);
extern rtx        gen_movti_insn                  (rtx, rtx);
extern rtx        gen_vaddaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vaddw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vavb_insn                   (rtx, rtx, rtx);
extern rtx        gen_vavrb_insn                  (rtx, rtx, rtx);
extern rtx        gen_vdifaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vdifw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmaxaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmaxw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vminaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vminw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmulaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmulfaw_insn                (rtx, rtx, rtx);
extern rtx        gen_vmulfw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmulw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vsubaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vsubw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vsummw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vand_insn                   (rtx, rtx, rtx);
extern rtx        gen_vandaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbic_insn                   (rtx, rtx, rtx);
extern rtx        gen_vbicaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vor_insn                    (rtx, rtx, rtx);
extern rtx        gen_vxor_insn                   (rtx, rtx, rtx);
extern rtx        gen_vxoraw_insn                 (rtx, rtx, rtx);
extern rtx        gen_veqw_insn                   (rtx, rtx, rtx);
extern rtx        gen_vlew_insn                   (rtx, rtx, rtx);
extern rtx        gen_vltw_insn                   (rtx, rtx, rtx);
extern rtx        gen_vnew_insn                   (rtx, rtx, rtx);
extern rtx        gen_vmr1aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr1w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr2aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr2w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr3aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr3w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr4aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr4w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr5aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr5w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr6aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr6w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmr7aw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmr7w_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmrb_insn                   (rtx, rtx, rtx);
extern rtx        gen_vh264f_insn                 (rtx, rtx, rtx);
extern rtx        gen_vh264ft_insn                (rtx, rtx, rtx);
extern rtx        gen_vh264fw_insn                (rtx, rtx, rtx);
extern rtx        gen_vvc1f_insn                  (rtx, rtx, rtx);
extern rtx        gen_vvc1ft_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbaddw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbmaxw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbminw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbmulaw_insn                (rtx, rtx, rtx);
extern rtx        gen_vbmulfw_insn                (rtx, rtx, rtx);
extern rtx        gen_vbmulw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vbrsubw_insn                (rtx, rtx, rtx);
extern rtx        gen_vbsubw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vasrrwi_insn                (rtx, rtx, rtx);
extern rtx        gen_vasrsrwi_insn               (rtx, rtx, rtx);
extern rtx        gen_vasrwi_insn                 (rtx, rtx, rtx);
extern rtx        gen_vasrpwbi_insn               (rtx, rtx, rtx);
extern rtx        gen_vasrrpwbi_insn              (rtx, rtx, rtx);
extern rtx        gen_vsr8awi_insn                (rtx, rtx, rtx);
extern rtx        gen_vsr8i_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmvaw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmvw_insn                   (rtx, rtx, rtx);
extern rtx        gen_vmvzw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vd6tapf_insn                (rtx, rtx, rtx);
extern rtx        gen_vmovaw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vmovw_insn                  (rtx, rtx, rtx);
extern rtx        gen_vmovzw_insn                 (rtx, rtx, rtx);
extern rtx        gen_vsr8_insn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_vasrw_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_vsr8aw_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_vabsaw_insn                 (rtx, rtx);
extern rtx        gen_vabsw_insn                  (rtx, rtx);
extern rtx        gen_vaddsuw_insn                (rtx, rtx);
extern rtx        gen_vsignw_insn                 (rtx, rtx);
extern rtx        gen_vexch1_insn                 (rtx, rtx);
extern rtx        gen_vexch2_insn                 (rtx, rtx);
extern rtx        gen_vexch4_insn                 (rtx, rtx);
extern rtx        gen_vupbaw_insn                 (rtx, rtx);
extern rtx        gen_vupbw_insn                  (rtx, rtx);
extern rtx        gen_vupsbaw_insn                (rtx, rtx);
extern rtx        gen_vupsbw_insn                 (rtx, rtx);
extern rtx        gen_vdirun_insn                 (rtx, rtx, rtx);
extern rtx        gen_vdorun_insn                 (rtx, rtx, rtx);
extern rtx        gen_vdiwr_insn                  (rtx, rtx);
extern rtx        gen_vdowr_insn                  (rtx, rtx);
extern rtx        gen_vrec_insn                   (rtx);
extern rtx        gen_vrun_insn                   (rtx);
extern rtx        gen_vrecrun_insn                (rtx);
extern rtx        gen_vendrec_insn                (rtx);
extern rtx        gen_vld32wh_insn                (rtx, rtx, rtx, rtx);
extern rtx        gen_vld32wl_insn                (rtx, rtx, rtx, rtx);
extern rtx        gen_vld64w_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_vld64_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_vld32_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_vst16_n_insn                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vst32_n_insn                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vinti_insn                  (rtx);
extern rtx        gen_movqi                       (rtx, rtx);
extern rtx        gen_movhi                       (rtx, rtx);
extern rtx        gen_movsi                       (rtx, rtx);
extern rtx        gen_bic_f_zn                    (rtx, rtx, rtx);
extern rtx        gen_movdi                       (rtx, rtx);
extern rtx        gen_movsf                       (rtx, rtx);
extern rtx        gen_movdf                       (rtx, rtx);
extern rtx        gen_movsicc                     (rtx, rtx, rtx, rtx);
extern rtx        gen_movdicc                     (rtx, rtx, rtx, rtx);
extern rtx        gen_movsfcc                     (rtx, rtx, rtx, rtx);
extern rtx        gen_movdfcc                     (rtx, rtx, rtx, rtx);
extern rtx        gen_zero_extendqihi2            (rtx, rtx);
extern rtx        gen_zero_extendqisi2            (rtx, rtx);
extern rtx        gen_zero_extendhisi2            (rtx, rtx);
extern rtx        gen_extendqihi2                 (rtx, rtx);
extern rtx        gen_extendqisi2                 (rtx, rtx);
extern rtx        gen_extendhisi2                 (rtx, rtx);
extern rtx        gen_mulhisi3                    (rtx, rtx, rtx);
extern rtx        gen_umulhisi3                   (rtx, rtx, rtx);
extern rtx        gen_mulsi3                      (rtx, rtx, rtx);
extern rtx        gen_mulsidi3                    (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart            (rtx, rtx, rtx);
extern rtx        gen_umulsidi3                   (rtx, rtx, rtx);
extern rtx        gen_addsi3                      (rtx, rtx, rtx);
extern rtx        gen_adddi3                      (rtx, rtx, rtx);
extern rtx        gen_subsi3                      (rtx, rtx, rtx);
extern rtx        gen_subdi3                      (rtx, rtx, rtx);
extern rtx        gen_andsi3                      (rtx, rtx, rtx);
extern rtx        gen_ashlsi3                     (rtx, rtx, rtx);
extern rtx        gen_ashrsi3                     (rtx, rtx, rtx);
extern rtx        gen_lshrsi3                     (rtx, rtx, rtx);
extern rtx        gen_cbranchsi4                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4                   (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4                   (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4                   (rtx, rtx, rtx, rtx);
extern rtx        gen_scc_insn                    (rtx, rtx);
extern rtx        gen_branch_insn                 (rtx, rtx);
extern rtx        gen_jump                        (rtx);
extern rtx        gen_casesi                      (rtx, rtx, rtx, rtx, rtx);
#define GEN_CALL(A, B, C, D) gen_call ((A), (B))
extern rtx        gen_call                        (rtx, rtx);
#define GEN_CALL_VALUE(A, B, C, D, E) gen_call_value ((A), (B), (C))
extern rtx        gen_call_value                  (rtx, rtx, rtx);
extern rtx        gen_clzsi2                      (rtx, rtx);
extern rtx        gen_ctzsi2                      (rtx, rtx);
extern rtx        gen_ffssi2                      (rtx, rtx);
#define GEN_SIBCALL(A, B, C, D) gen_sibcall ((A), (B), (C))
extern rtx        gen_sibcall                     (rtx, rtx, rtx);
#define GEN_SIBCALL_VALUE(A, B, C, D, E) gen_sibcall_value ((A), (B), (C), (D))
extern rtx        gen_sibcall_value               (rtx, rtx, rtx, rtx);
extern rtx        gen_prologue                    (void);
extern rtx        gen_epilogue                    (void);
extern rtx        gen_sibcall_epilogue            (void);
extern rtx        gen_return                      (void);
extern rtx        gen_doloop_begin                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_doloop_end                  (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_movmemsi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cmp_float                   (rtx, rtx);
extern rtx        gen_prefetch                    (rtx, rtx, rtx);
extern rtx        gen_rotlsi3                     (rtx, rtx, rtx);
extern rtx        gen_rotrsi3                     (rtx, rtx, rtx);
extern rtx        gen_adddf3                      (rtx, rtx, rtx);
extern rtx        gen_muldf3                      (rtx, rtx, rtx);
extern rtx        gen_subdf3                      (rtx, rtx, rtx);
extern rtx        gen_movv8hi                     (rtx, rtx);

#endif /* GCC_INSN_FLAGS_H */
