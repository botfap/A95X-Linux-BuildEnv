all:depends.all

	
include Makefile

%.o.depend:
	@$(CC) -M $(CFLAGS) $*.c >$*.o.d


depends.all:$(filter %.o.depend,$(obj-y:%.o=%.o.depend)) 

