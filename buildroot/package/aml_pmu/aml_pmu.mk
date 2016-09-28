################################################################################
#
# amlogic pmu driver
#
################################################################################

AML_PMU_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_PMU_GIT_VERSION))
AML_PMU_SITE = $(call qstrip,$(BR2_PACKAGE_AML_PMU_GIT_REPO_URL))
define AML_PMU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/amlogic;
	ln -sf $(AML_PMU_DIR) $(LINUX_DIR)/../hardware/amlogic/pmu
endef
$(eval $(generic-package))
