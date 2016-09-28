clean:clean_all



include Makefile
clean_all:$(filter %-clrdir,$(obj-y:%/=%-clrdir))
	$(call clean_mk,*.o)
	$(call clean_mk,*.o.d)
	$(call clean_mk, build-in.o)
   

include $(SRCTREE)/rules.mk