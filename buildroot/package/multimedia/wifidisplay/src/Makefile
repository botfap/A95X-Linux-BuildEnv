TOPDIR := .

MODULES += foundation utils libcorkscrew stagefright wifi-display wifi-player interface
MODULES_DEMO := example
MODULES_INCLUDE := interface

submodule_make = $(MAKE) -C $(TOPDIR)/$(1);
submodule_clean = $(MAKE) clean -C $(TOPDIR)/$(1);
submodule_install = $(MAKE) install -C $(TOPDIR)/$(1);
submodule_install_include = $(MAKE) install_include -C $(TOPDIR)/$(1);

.PHONY :  all clean modules cleanmodules example install uninstall

modules :
	@ $(foreach n,$(MODULES),$(call submodule_make,$(n)))

cleanmodules :
	@ $(foreach n,$(MODULES),$(call submodule_clean,$(n)))

all: modules

clean: cleanmodules
	@ echo CLEAN DONE

example:
	$(MAKE) -C $(TOPDIR)/$(MODULES_DEMO)
	$(MAKE) install -C $(TOPDIR)/$(MODULES_DEMO)

install:
	if [ ! -d $(STAGING) ]; then \
		mkdir -p $(STAGING); \
    fi
	@ $(foreach n,$(MODULES),$(call submodule_install,$(n)))

install_include:
	if [ ! -d $(STAGING_INC) ]; then \
		mkdir -p $(STAGING_INC); \
	fi
	if [ ! -d $(PREFIX_INC) ]; then \
		mkdir -p $(PREFIX_INC); \
	fi
	@ $(foreach n,$(MODULES_INCLUDE),$(call submodule_install_include,$(n)))
	
uninstall:
	if [ -d $(STAGING) ]; then \
		rm -rf $(STAGING); \
		rm -rf $(STAGING_INC); \
		rm -rf $(PREFIX_INC); \
		rm $(INSTALL_STAG)/wfd; \
		rm $(INSTALL_PREFIX)/wfd; \
    fi
