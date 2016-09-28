#############################################################
#
# aml_libs
#
#############################################################
AML_LIBS_VERSION:=0.4.0
AML_LIBS_SITE=$(TOPDIR)/package/multimedia/aml_libs/src
AML_LIBS_SITE_METHOD=local
#AML_LIBS_DIR=$(BUILD_DIR)/aml_libs
#AML_LIBS_SOURCE=src
AML_LIBS_DEPENDENCIES=zlib alsa-lib
export AML_LIBS_BUILD_DIR = $(BUILD_DIR)
export AML_LIBS_STAGING_DIR = $(STAGING_DIR)
export AML_LIBS_TARGET_DIR = $(TARGET_DIR)

define AML_LIBS_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) all
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

$(eval $(generic-package))
#$(AML_LIBS_DIR)/.unpacked:
#	mkdir -p $(AML_LIBS_DIR)
#	cp -arf ./package/multimedia/aml_libs/src/* $(AML_LIBS_DIR)
#	touch $(AML_LIBS_DIR)/.unpacked
#
#$(AML_LIBS_DIR)/aml_libs: $(AML_LIBS_DIR)/.unpacked
#	$(MAKE) CC=$(TARGET_CC) -C $(AML_LIBS_DIR) all
#	$(MAKE) CC=$(TARGET_CC) -C $(AML_LIBS_DIR) install
#
#aml_libs:$(DEPENDS) $(AML_LIBS_DIR)/aml_libs
#
#aml_libs-source: $(DL_DIR)/$(AML_LIBS_SOURCE)
#
#aml_libs-clean:
#	-$(MAKE) -C $(AML_LIBS_DIR) clean
#
#aml_libs-dirclean:
#	rm -rf $(AML_LIBS_DIR)
#
##before_cmd:depends
#
##depends:
##	@if [   "${DEPENDS}" != "" ]; then \
#                cd ${TOPDIR};make ${DEPENDS};    \
#        fi
##############################################################
##
## Toplevel Makefile options
##
##############################################################
#ifeq ($(BR2_PACKAGE_AML_LIBS),y)
#TARGETS+=aml_libs
#endif
