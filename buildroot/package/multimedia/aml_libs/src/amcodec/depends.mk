all:depends.all


	
include Makefile

%.o.depend:
	@$(CC) -M -I$(CFLAGS) $*.c >$*.o.d
	@echo '	$$(call c_mk,$$<)'	>>$*.o.d


depends.all:$(filter %.o.depend,$(obj-y:%.o=%.o.depend)) 

