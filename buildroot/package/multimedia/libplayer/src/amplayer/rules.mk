##


dir_mk=$(Q)echo "MAKE $(1)";\
	$(MAKE) -C ${1} -f ${SRCTREE}/depends.mk;\
	$(MAKE) -C ${1} -f ${SRCTREE}/dir.mk

c_mk=$(Q)echo "CC  $(1)";\
		$(CC) $(CFLAGS)  $(EXT_CFLAGS) -c  $(1) 

AFLAGS=-D__ASSEMBLY__ $(CFLAGS)
s_mk=$(Q)echo "ASM $(1)";\
		$(CC) $(AFLAGS) $(EXT_CFLAGS) -c $(1) 
	

ar_mk=$(Q)echo "LD $(1)";\
		$(LD) -r -o build-in.o $(2:%-dir=%/build-in.o)


ld_mk=$(Q)echo "LD $(1) $(2)  $(3)";\
		$(CC)   	$(LDFLAGS)\
		$(2:%-dir=%/build-in.o) \
		$(3:%-dir=%/build-in.o) \
		-o $(1) 	
				
clr_mk=$(Q)echo "CLEAN  $(1)";\
		$(MAKE) -C ${1} -f ${SRCTREE}/clean.mk
		
copy_mk=$(Q)echo "CBJCOPY  $(1) $(2)";\
	$(OBJCOPY) -O binary -R .note -R .comment -S $(1) $(2)


clean_mk=$(Q)rm -f $(1) $(2)

	
%.o:%.S
	$(call s_mk,$<)

%.o:%.c
	$(call c_mk,$<)

%-dir:
	$(call dir_mk,$*)
	
%-clrdir:
	$(call clr_mk,$*)	
	
build-in.o:$(obj-y:%/=%-dir)
	$(call ar_mk,$@,$(obj-y:%/=%-dir))	

	
extern-in.o:$(extern-y)
	
	
