#ifndef __AMLVSINK_PROP_H__
#define __AMLVSINK_PROP_H__
#include "gstamlsysctl.h"

typedef enum{
    PROP_0 = 0,
    PROP_SILENT,
    PROP_ASYNC,
    PROP_TVMODE,
    PROP_PLANE,
    PROP_RECTANGLE,
    PROP_FLSH_RPT_FRM,
    PROP_CURT_PTS,
    PROP_INTER_FRM_DELY,
    PROP_SLOW_FRM_RATE,
    PROP_STEP_FRM,
    PROP_MUTE,
    PROP_MAX = 0x1FFFFFFF
}AML_VSINK_PROP;

AmlPropType *aml_get_vsink_prop_interface();

#endif
