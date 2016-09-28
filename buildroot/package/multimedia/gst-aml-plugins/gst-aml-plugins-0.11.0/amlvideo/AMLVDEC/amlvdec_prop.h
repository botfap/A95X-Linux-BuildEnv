#ifndef __AMLVDEC_PROP_H__
#define __AMLVDEC_PROP_H__
#include "gstamlsysctl.h"

typedef enum{
    PROP_0 = 0,
    PROP_TRICK_RATE,
    PROP_INTERLACED,
    PROP_DEC_HDL,
    PROP_PCRTOSYSTEMTIME,
    PROP_CONTENTFRAME,
    PROP_PASSTHROUGH,
    PROP_MAX = 0x1FFFFFFF
}AML_VDEC_PROP;

AmlPropType *aml_get_vdec_prop_interface();

#endif
