################################################################################
#
# tegra210-wireguard
#
################################################################################

TEGRA210_WIREGUARD_VERSION = 1.0.20211208
TEGRA210_WIREGUARD_SITE = https://git.zx2c4.com/wireguard-linux-compat/snapshot
TEGRA210_WIREGUARD_SOURCE = wireguard-linux-compat-$(TEGRA210_WIREGUARD_VERSION).tar.xz
TEGRA210_WIREGUARD_LICENSE = GPL-2.0
TEGRA210_WIREGUARD_LICENSE_FILES = COPYING
TEGRA210_WIREGUARD_DEPENDENCIES = tegra210
TEGRA210_WIREGUARD_MODULE_SUBDIRS = src

define TEGRA210_WIREGUARD_LINUX_CONFIG_FIXUPS
       $(call KCONFIG_ENABLE_OPT,CONFIG_INET)
       $(call KCONFIG_ENABLE_OPT,CONFIG_NET)
       $(call KCONFIG_ENABLE_OPT,CONFIG_NET_FOU)
       $(call KCONFIG_ENABLE_OPT,CONFIG_CRYPTO)
       $(call KCONFIG_ENABLE_OPT,CONFIG_CRYPTO_MANAGER)
endef

# we can't use the standard kernel module rules because we don't build the kernel
define TEGRA210_WIREGUARD_KERNEL_MODULES_BUILD
    $(call MESSAGE,"Building kernel module(s)")
	$(foreach d,$(TEGRA210_WIREGUARD_MODULE_SUBDIRS), \
		$(LINUX_MAKE_ENV) $(MAKE) \
        -C $(@D)/$(d) \
        $(LINUX_MAKE_FLAGS) \
        KERNELRELEASE=4.9.253-tegra \
        KERNELDIR=$(TEGRA210_DIR)/kernel/kernel_headers/linux-headers-4.9.253-tegra-linux_x86_64/kernel-4.9 \
		module$(sep))
endef
TEGRA210_WIREGUARD_POST_BUILD_HOOKS += TEGRA210_WIREGUARD_KERNEL_MODULES_BUILD

define TEGRA210_WIREGUARD_KERNEL_MODULES_INSTALL
    $(call MESSAGE,"Installing kernel module(s)")
	$(foreach d,$(TEGRA210_WIREGUARD_MODULE_SUBDIRS), \
		$(LINUX_MAKE_ENV) $(MAKE) \
        -C $(@D)/$(d) \
        $(LINUX_MAKE_FLAGS) \
        KERNELRELEASE=4.9.253-tegra \
        KERNELDIR=$(TEGRA210_DIR)/kernel/kernel_headers/linux-headers-4.9.253-tegra-linux_x86_64/kernel-4.9 \
		DEPMODBASEDIR=$(TARGET_DIR) \
		module-install$(sep))
endef
TEGRA210_WIREGUARD_POST_INSTALL_TARGET_HOOKS += TEGRA210_WIREGUARD_KERNEL_MODULES_INSTALL

$(eval $(generic-package))
