#############################################################
#
# wifidisplay
#
#############################################################
WIFIDISPLAY_VERSION:=0.1.0
WIFIDISPLAY_DIR=$(BUILD_DIR)/wifidisplay
WIFIDISPLAY_SOURCE=src
WIFIDISPLAY_SITE=.

DEPENDS=libplayer

WIFIDISPLAY_ENV_OPT="STAGING=$(STAGING_DIR)/usr/lib/wifidisplay \
		    PREFIX=$(TARGET_DIR)/usr/lib \
		    STAGING_INC=$(STAGING_DIR)/usr/include/wifidisplay \
		    PREFIX_INC=$(TARGET_DIR)/usr/include/wifidisplay \
		    WFD_LIB_DIR=$(STAGING_DIR)/usr/lib \
		    WFD_INC_DIR=$(STAGING_DIR)/usr/include \
		    INSTALL_STAG=$(STAGING_DIR)/usr/bin \
		    INSTALL_PREFIX=$(TARGET_DIR)/usr/bin"

$(WIFIDISPLAY_DIR)/.unpacked: wifidisplay-unpacked

wifidisplay-unpacked:
	-rm -rf $(WIFIDISPLAY_DIR)
	mkdir -p $(WIFIDISPLAY_DIR)
	cp -arf ./package/multimedia/wifidisplay/src/* $(WIFIDISPLAY_DIR)
	touch $(WIFIDISPLAY_DIR)/.unpacked

$(WIFIDISPLAY_DIR)/.installed: $(WIFIDISPLAY_DIR)/wifidisplay

$(WIFIDISPLAY_DIR)/wifidisplay: $(WIFIDISPLAY_DIR)/.unpacked
	$(WIFIDISPLAY_ENV_OPT) $(MAKE) -C $(WIFIDISPLAY_DIR)
	$(WIFIDISPLAY_ENV_OPT) $(MAKE) install -C $(WIFIDISPLAY_DIR) 
ifeq ($(BR2_PACKAGE_WIFIDISPLAY_DEMO),y)
	$(WIFIDISPLAY_ENV_OPT) $(MAKE) example -C $(WIFIDISPLAY_DIR)
endif
	$(WIFIDISPLAY_ENV_OPT) $(MAKE) install_include -C $(WIFIDISPLAY_DIR)
	touch $(WIFIDISPLAY_DIR)/.installed

wifidisplay:$(DEPENDS) $(WIFIDISPLAY_DIR)/.installed

wifidisplay-source: $(DL_DIR)/$(WIFIDISPLAY_SOURCE)

wifidisplay-clean:
	-$(MAKE) clean -C $(WIFIDISPLAY_DIR)
	-$(MAKE) uninstall -C $(WIFIDISPLAY_DIR)

wifidisplay-dirclean:
	rm -rf $(WIFIDISPLAY_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(BR2_PACKAGE_WIFIDISPLAY),y)
TARGETS+=wifidisplay
endif
