/* Generated automatically by the program 'build/genpreds'
   from the machine description file '/SCRATCH/akolesov/toolchain/unisrc-4.8/gcc/config/arc/arc.md'.  */

#ifndef GCC_TM_PREDS_H
#define GCC_TM_PREDS_H

#ifdef HAVE_MACHINE_MODES
extern int general_operand (rtx, enum machine_mode);
extern int address_operand (rtx, enum machine_mode);
extern int register_operand (rtx, enum machine_mode);
extern int pmode_register_operand (rtx, enum machine_mode);
extern int scratch_operand (rtx, enum machine_mode);
extern int immediate_operand (rtx, enum machine_mode);
extern int const_int_operand (rtx, enum machine_mode);
extern int const_double_operand (rtx, enum machine_mode);
extern int nonimmediate_operand (rtx, enum machine_mode);
extern int nonmemory_operand (rtx, enum machine_mode);
extern int push_operand (rtx, enum machine_mode);
extern int pop_operand (rtx, enum machine_mode);
extern int memory_operand (rtx, enum machine_mode);
extern int indirect_operand (rtx, enum machine_mode);
extern int ordered_comparison_operator (rtx, enum machine_mode);
extern int comparison_operator (rtx, enum machine_mode);
extern int dest_reg_operand (rtx, enum machine_mode);
extern int mpy_dest_reg_operand (rtx, enum machine_mode);
extern int symbolic_operand (rtx, enum machine_mode);
extern int call_address_operand (rtx, enum machine_mode);
extern int call_operand (rtx, enum machine_mode);
extern int u6_immediate_operand (rtx, enum machine_mode);
extern int short_immediate_operand (rtx, enum machine_mode);
extern int p2_immediate_operand (rtx, enum machine_mode);
extern int long_immediate_operand (rtx, enum machine_mode);
extern int long_immediate_loadstore_operand (rtx, enum machine_mode);
extern int compact_register_operand (rtx, enum machine_mode);
extern int compact_load_memory_operand (rtx, enum machine_mode);
extern int compact_store_memory_operand (rtx, enum machine_mode);
extern int move_src_operand (rtx, enum machine_mode);
extern int move_double_src_operand (rtx, enum machine_mode);
extern int move_dest_operand (rtx, enum machine_mode);
extern int load_update_operand (rtx, enum machine_mode);
extern int store_update_operand (rtx, enum machine_mode);
extern int nonvol_nonimm_operand (rtx, enum machine_mode);
extern int proper_comparison_operator (rtx, enum machine_mode);
extern int equality_comparison_operator (rtx, enum machine_mode);
extern int brcc_nolimm_operator (rtx, enum machine_mode);
extern int cc_register (rtx, enum machine_mode);
extern int cc_set_register (rtx, enum machine_mode);
extern int cc_use_register (rtx, enum machine_mode);
extern int zn_compare_operator (rtx, enum machine_mode);
extern int shift_operator (rtx, enum machine_mode);
extern int shiftl4_operator (rtx, enum machine_mode);
extern int shiftr4_operator (rtx, enum machine_mode);
extern int shift4_operator (rtx, enum machine_mode);
extern int mult_operator (rtx, enum machine_mode);
extern int commutative_operator (rtx, enum machine_mode);
extern int commutative_operator_sans_mult (rtx, enum machine_mode);
extern int noncommutative_operator (rtx, enum machine_mode);
extern int unary_operator (rtx, enum machine_mode);
extern int _2_4_8_operand (rtx, enum machine_mode);
extern int arc_double_register_operand (rtx, enum machine_mode);
extern int shouldbe_register_operand (rtx, enum machine_mode);
extern int vector_register_operand (rtx, enum machine_mode);
extern int vector_register_or_memory_operand (rtx, enum machine_mode);
extern int arc_dpfp_operator (rtx, enum machine_mode);
extern int arc_simd_dma_register_operand (rtx, enum machine_mode);
extern int acc1_operand (rtx, enum machine_mode);
extern int acc2_operand (rtx, enum machine_mode);
extern int mlo_operand (rtx, enum machine_mode);
extern int mhi_operand (rtx, enum machine_mode);
extern int extend_operand (rtx, enum machine_mode);
extern int millicode_store_operation (rtx, enum machine_mode);
extern int millicode_load_operation (rtx, enum machine_mode);
extern int millicode_load_clob_operation (rtx, enum machine_mode);
extern int immediate_usidi_operand (rtx, enum machine_mode);
extern int mpy_reg_operand (rtx, enum machine_mode);
#endif /* HAVE_MACHINE_MODES */

#define CONSTRAINT_NUM_DEFINED_P 1
enum constraint_num
{
  CONSTRAINT__UNKNOWN = 0,
  CONSTRAINT_c,
  CONSTRAINT_Rac,
  CONSTRAINT_w,
  CONSTRAINT_W,
  CONSTRAINT_l,
  CONSTRAINT_x,
  CONSTRAINT_Rgp,
  CONSTRAINT_f,
  CONSTRAINT_b,
  CONSTRAINT_k,
  CONSTRAINT_q,
  CONSTRAINT_e,
  CONSTRAINT_D,
  CONSTRAINT_d,
  CONSTRAINT_v,
  CONSTRAINT_Rsc,
  CONSTRAINT_I,
  CONSTRAINT_K,
  CONSTRAINT_L,
  CONSTRAINT_CnL,
  CONSTRAINT_CmL,
  CONSTRAINT_M,
  CONSTRAINT_N,
  CONSTRAINT_O,
  CONSTRAINT_P,
  CONSTRAINT_C__0,
  CONSTRAINT_Cca,
  CONSTRAINT_CL2,
  CONSTRAINT_CM4,
  CONSTRAINT_Csp,
  CONSTRAINT_C2a,
  CONSTRAINT_C0p,
  CONSTRAINT_C1p,
  CONSTRAINT_Ccp,
  CONSTRAINT_Cux,
  CONSTRAINT_Crr,
  CONSTRAINT_G,
  CONSTRAINT_H,
  CONSTRAINT_T,
  CONSTRAINT_S,
  CONSTRAINT_Usd,
  CONSTRAINT_Usc,
  CONSTRAINT_Us_l,
  CONSTRAINT_Us_g,
  CONSTRAINT_Cbr,
  CONSTRAINT_Cbp,
  CONSTRAINT_Cpc,
  CONSTRAINT_Clb,
  CONSTRAINT_Cal,
  CONSTRAINT_C32,
  CONSTRAINT_Rcq,
  CONSTRAINT_Rcw,
  CONSTRAINT_Rcr,
  CONSTRAINT_Rcb,
  CONSTRAINT_Rck,
  CONSTRAINT_Rs5,
  CONSTRAINT_Rcc,
  CONSTRAINT_Q,
  CONSTRAINT_Cm1,
  CONSTRAINT_Cm2,
  CONSTRAINT_Cm3,
  CONSTRAINT__LIMIT
};

extern enum constraint_num lookup_constraint (const char *);
extern bool constraint_satisfied_p (rtx, enum constraint_num);

static inline size_t
insn_constraint_len (char fc, const char *str ATTRIBUTE_UNUSED)
{
  switch (fc)
    {
    case 'C': return 3;
    case 'R': return 3;
    case 'U': return 3;
    default: break;
    }
  return 1;
}

#define CONSTRAINT_LEN(c_,s_) insn_constraint_len (c_,s_)

extern enum reg_class regclass_for_constraint (enum constraint_num);
#define REG_CLASS_FROM_CONSTRAINT(c_,s_) \
    regclass_for_constraint (lookup_constraint (s_))
#define REG_CLASS_FOR_CONSTRAINT(x_) \
    regclass_for_constraint (x_)

extern bool insn_const_int_ok_for_constraint (HOST_WIDE_INT, enum constraint_num);
#define CONST_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    insn_const_int_ok_for_constraint (v_, lookup_constraint (s_))

#define CONST_DOUBLE_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    constraint_satisfied_p (v_, lookup_constraint (s_))

#define EXTRA_CONSTRAINT_STR(v_,c_,s_) \
    constraint_satisfied_p (v_, lookup_constraint (s_))

extern bool insn_extra_memory_constraint (enum constraint_num);
#define EXTRA_MEMORY_CONSTRAINT(c_,s_) insn_extra_memory_constraint (lookup_constraint (s_))

#define EXTRA_ADDRESS_CONSTRAINT(c_,s_) false

#endif /* tm-preds.h */
