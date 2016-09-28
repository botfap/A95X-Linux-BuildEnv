include $(LOCAL_PATH)/../config.mak

OBJS :=
OBJS-yes :=
MMX-OBJS-yes :=
FFLIBS :=

include $(LOCAL_PATH)/Makefile
OBJS += $(OBJS-yes)

# collect objects
FFNAME := lib$(NAME)
FFLIBS := $(foreach NAME,$(FFLIBS),lib$(NAME))
FFCFLAGS = $(CFLAGS)
FFCFLAGS += -DHAVE_AV_CONFIG_H

ALL_S_FILES := $(wildcard $(LOCAL_PATH)/$(TARGET_ARCH)/*.S)
ALL_S_FILES := $(addprefix $(TARGET_ARCH)/, $(notdir $(ALL_S_FILES)))

ifneq ($(ALL_S_FILES),)
ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
C_OBJS := $(filter-out $(ALL_S_OBJS),$(OBJS)) 
S_OBJS := $(filter $(ALL_S_OBJS),$(OBJS))
else
C_OBJS := $(OBJS)
S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))

FFFILES := $(sort $(S_FILES)) $(sort $(C_FILES))

LOCAL_ARM_MODE := arm
