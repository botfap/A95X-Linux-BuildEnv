#ifndef __AMLASINK_PROP_H__
#define __AMLASINK_PROP_H__
#include "gstamlsysctl.h"

typedef enum{
    PROP_0 = 0,
    PROP_VOLUME,
    PROP_MUTE,
    PROP_AOUT_HDMI,
    PROP_AOUT_SPDIF,
    PROP_AOUT_OTHER,
    PROP_MAX = 0x1FFFFFFF
}AML_ASINK_PROP;

AmlPropType *aml_get_asink_prop_interface();

#endif

