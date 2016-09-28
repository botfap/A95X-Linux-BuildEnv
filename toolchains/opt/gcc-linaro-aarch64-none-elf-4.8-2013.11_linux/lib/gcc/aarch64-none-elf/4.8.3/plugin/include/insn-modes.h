/* Generated automatically from machmode.def and config/aarch64/aarch64-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  VOIDmode,                /* machmode.def:172 */
  BLKmode,                 /* machmode.def:176 */
  CCmode,                  /* machmode.def:204 */
  CCFPmode,                /* config/aarch64/aarch64-modes.def:21 */
  CCFPEmode,               /* config/aarch64/aarch64-modes.def:22 */
  CC_SWPmode,              /* config/aarch64/aarch64-modes.def:23 */
  CC_ZESWPmode,            /* config/aarch64/aarch64-modes.def:24 */
  CC_SESWPmode,            /* config/aarch64/aarch64-modes.def:25 */
  CC_NZmode,               /* config/aarch64/aarch64-modes.def:26 */
  BImode,                  /* machmode.def:179 */
  QImode,                  /* machmode.def:184 */
  HImode,                  /* machmode.def:185 */
  SImode,                  /* machmode.def:186 */
  DImode,                  /* machmode.def:187 */
  TImode,                  /* machmode.def:188 */
  EImode,                  /* config/aarch64/aarch64-modes.def:39 */
  OImode,                  /* config/aarch64/aarch64-modes.def:35 */
  CImode,                  /* config/aarch64/aarch64-modes.def:40 */
  XImode,                  /* config/aarch64/aarch64-modes.def:41 */
  QQmode,                  /* machmode.def:207 */
  HQmode,                  /* machmode.def:208 */
  SQmode,                  /* machmode.def:209 */
  DQmode,                  /* machmode.def:210 */
  TQmode,                  /* machmode.def:211 */
  UQQmode,                 /* machmode.def:213 */
  UHQmode,                 /* machmode.def:214 */
  USQmode,                 /* machmode.def:215 */
  UDQmode,                 /* machmode.def:216 */
  UTQmode,                 /* machmode.def:217 */
  HAmode,                  /* machmode.def:219 */
  SAmode,                  /* machmode.def:220 */
  DAmode,                  /* machmode.def:221 */
  TAmode,                  /* machmode.def:222 */
  UHAmode,                 /* machmode.def:224 */
  USAmode,                 /* machmode.def:225 */
  UDAmode,                 /* machmode.def:226 */
  UTAmode,                 /* machmode.def:227 */
  SFmode,                  /* machmode.def:199 */
  DFmode,                  /* machmode.def:200 */
  TFmode,                  /* config/aarch64/aarch64-modes.def:54 */
  SDmode,                  /* machmode.def:239 */
  DDmode,                  /* machmode.def:240 */
  TDmode,                  /* machmode.def:241 */
  CQImode,                 /* machmode.def:235 */
  CHImode,                 /* machmode.def:235 */
  CSImode,                 /* machmode.def:235 */
  CDImode,                 /* machmode.def:235 */
  CTImode,                 /* machmode.def:235 */
  CEImode,                 /* machmode.def:235 */
  COImode,                 /* machmode.def:235 */
  CCImode,                 /* machmode.def:235 */
  CXImode,                 /* machmode.def:235 */
  SCmode,                  /* machmode.def:236 */
  DCmode,                  /* machmode.def:236 */
  TCmode,                  /* machmode.def:236 */
  V8QImode,                /* config/aarch64/aarch64-modes.def:29 */
  V4HImode,                /* config/aarch64/aarch64-modes.def:29 */
  V2SImode,                /* config/aarch64/aarch64-modes.def:29 */
  V16QImode,               /* config/aarch64/aarch64-modes.def:30 */
  V8HImode,                /* config/aarch64/aarch64-modes.def:30 */
  V4SImode,                /* config/aarch64/aarch64-modes.def:30 */
  V2DImode,                /* config/aarch64/aarch64-modes.def:30 */
  V32QImode,               /* config/aarch64/aarch64-modes.def:44 */
  V16HImode,               /* config/aarch64/aarch64-modes.def:44 */
  V8SImode,                /* config/aarch64/aarch64-modes.def:44 */
  V4DImode,                /* config/aarch64/aarch64-modes.def:44 */
  V2TImode,                /* config/aarch64/aarch64-modes.def:44 */
  V48QImode,               /* config/aarch64/aarch64-modes.def:47 */
  V24HImode,               /* config/aarch64/aarch64-modes.def:47 */
  V12SImode,               /* config/aarch64/aarch64-modes.def:47 */
  V6DImode,                /* config/aarch64/aarch64-modes.def:47 */
  V3TImode,                /* config/aarch64/aarch64-modes.def:47 */
  V2EImode,                /* config/aarch64/aarch64-modes.def:47 */
  V64QImode,               /* config/aarch64/aarch64-modes.def:50 */
  V32HImode,               /* config/aarch64/aarch64-modes.def:50 */
  V16SImode,               /* config/aarch64/aarch64-modes.def:50 */
  V8DImode,                /* config/aarch64/aarch64-modes.def:50 */
  V4TImode,                /* config/aarch64/aarch64-modes.def:50 */
  V2OImode,                /* config/aarch64/aarch64-modes.def:50 */
  V2SFmode,                /* config/aarch64/aarch64-modes.def:31 */
  V4SFmode,                /* config/aarch64/aarch64-modes.def:32 */
  V2DFmode,                /* config/aarch64/aarch64-modes.def:32 */
  V8SFmode,                /* config/aarch64/aarch64-modes.def:45 */
  V4DFmode,                /* config/aarch64/aarch64-modes.def:45 */
  V12SFmode,               /* config/aarch64/aarch64-modes.def:48 */
  V6DFmode,                /* config/aarch64/aarch64-modes.def:48 */
  V16SFmode,               /* config/aarch64/aarch64-modes.def:51 */
  V8DFmode,                /* config/aarch64/aarch64-modes.def:51 */
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = VOIDmode,
  MAX_MODE_RANDOM = BLKmode,

  MIN_MODE_CC = CCmode,
  MAX_MODE_CC = CC_NZmode,

  MIN_MODE_INT = QImode,
  MAX_MODE_INT = XImode,

  MIN_MODE_PARTIAL_INT = VOIDmode,
  MAX_MODE_PARTIAL_INT = VOIDmode,

  MIN_MODE_FRACT = QQmode,
  MAX_MODE_FRACT = TQmode,

  MIN_MODE_UFRACT = UQQmode,
  MAX_MODE_UFRACT = UTQmode,

  MIN_MODE_ACCUM = HAmode,
  MAX_MODE_ACCUM = TAmode,

  MIN_MODE_UACCUM = UHAmode,
  MAX_MODE_UACCUM = UTAmode,

  MIN_MODE_FLOAT = SFmode,
  MAX_MODE_FLOAT = TFmode,

  MIN_MODE_DECIMAL_FLOAT = SDmode,
  MAX_MODE_DECIMAL_FLOAT = TDmode,

  MIN_MODE_COMPLEX_INT = CQImode,
  MAX_MODE_COMPLEX_INT = CXImode,

  MIN_MODE_COMPLEX_FLOAT = SCmode,
  MAX_MODE_COMPLEX_FLOAT = TCmode,

  MIN_MODE_VECTOR_INT = V8QImode,
  MAX_MODE_VECTOR_INT = V2OImode,

  MIN_MODE_VECTOR_FRACT = VOIDmode,
  MAX_MODE_VECTOR_FRACT = VOIDmode,

  MIN_MODE_VECTOR_UFRACT = VOIDmode,
  MAX_MODE_VECTOR_UFRACT = VOIDmode,

  MIN_MODE_VECTOR_ACCUM = VOIDmode,
  MAX_MODE_VECTOR_ACCUM = VOIDmode,

  MIN_MODE_VECTOR_UACCUM = VOIDmode,
  MAX_MODE_VECTOR_UACCUM = VOIDmode,

  MIN_MODE_VECTOR_FLOAT = V2SFmode,
  MAX_MODE_VECTOR_FLOAT = V8DFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_SIZE const
#define CONST_MODE_BASE_ALIGN const
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#endif /* insn-modes.h */
