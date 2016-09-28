all:build-in.o extern-in.o


include Makefile

include $(filter %.o.d,$(obj-y:%.o=%.o.d))

include $(SRCTREE)/rules.mk