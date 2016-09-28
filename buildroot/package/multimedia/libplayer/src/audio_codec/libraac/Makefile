DW_NAME		= raac
M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
INSTALL_DIR=$(LIB_DIR)/libplayer
SRC_PATH		= ./
SRC_FILES		=  $(wildcard $(SRC_PATH)*.c)
EXPORT_INCLUDE_FILES	= $(wildcard $(SRC_PATH)*.h) 
 
USING_LIBS		= 
USING_INCLUDES_PATH	= -I$(SRC_PATH)/include

USING_LIBS_PATH		= 
DEBUG  		= YES
DEFINES		= 

CROSS=$(CC:%gcc=%)
G++		= $(CROSS)g++
GCC		= $(CROSS)gcc
LD 		= $(CROSS)ld

DEBUG_CFLAGS     := -c -Wall -shared -fPIC -Wno-format -g -DDEBUG -fexceptions -fnon-call-exceptions
RELEASE_CFLAGS   := -c -Wall -shared -fPIC -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions

DEBUG_LDFLAGS    := -g -shared -fPIC 
RELEASE_LDFLAGS  := -shared -fPIC


ifeq (YES, ${DEBUG})
	CFLAGS       	:= ${DEBUG_CFLAGS}
	LDFLAGS      	:= ${DEBUG_LDFLAGS}
	OUTPUT_PATH	:= $(INSTALL_DIR)
else
	CFLAGS      	:= ${RELEASE_CFLAGS}
	LDFLAGS      	:= ${RELEASE_LDFLAGS}
	OUTPUT_PATH	:= $(INSTALL_DIR)
endif

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS		+= $(DEFINES)
CFLAGS		+= $(USING_INCLUDES_PATH)

# $(call make-depend,source-file,object-file,depend-file)
define make-depend
	$(GCC) -MM -MF $3 -MP -MT $2 $(CFLAGS) $1
endef

OBJS 		= $(addsuffix .o,$(basename $(SRC_FILES)))
DEPENDS 		= $(addsuffix .d,$(basename $(SRC_FILES)))
OUTPUT 			= lib$(DW_NAME).so

EXPORT_INCLUDE_FILES := $(addprefix $(SRC_PATH),$(EXPORT_INCLUDE_FILES))


all:$(OUTPUT)
	
$(OUTPUT):$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(USING_LIBS_PATH) $(USING_LIBS)


%.o:%.c
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@$(GCC) $(CFLAGS) $< -o $@

	
%.o:%.cpp
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@$(G++) $(CFLAGS) $< -o $@


-include $(DEPENDS)

install:
	-install -m 555 ${OUTPUT} $(INSTALL_DIR)
	
rebuild:clean all

clean:
	@rm -f *.o
	@rm -f *.d







