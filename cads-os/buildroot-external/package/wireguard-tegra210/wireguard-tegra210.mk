################################################################################
#
# wireguard-tegra210
#
################################################################################

WIREGUARD_TEGRA210_VERSION = 1.0.20211208
WIREGUARD_TEGRA210_SITE = https://git.zx2c4.com/wireguard-linux-compat/snapshot
WIREGUARD_TEGRA210_SOURCE = wireguard-linux-compat-$(WIREGUARD_TEGRA210_VERSION).tar.xz
WIREGUARD_TEGRA210_LICENSE = GPL-2.0
WIREGUARD_TEGRA210_LICENSE_FILES = COPYING
WIREGUARD_TEGRA210_DEPENDENCIES = tegra210
WIREGUARD_TEGRA210_MODULE_SUBDIRS = src

define WIREGUARD_TEGRA210_LINUX_CONFIG_FIXUPS
       $(call KCONFIG_ENABLE_OPT,CONFIG_INET)
       $(call KCONFIG_ENABLE_OPT,CONFIG_NET)
       $(call KCONFIG_ENABLE_OPT,CONFIG_NET_FOU)
       $(call KCONFIG_ENABLE_OPT,CONFIG_CRYPTO)
       $(call KCONFIG_ENABLE_OPT,CONFIG_CRYPTO_MANAGER)
endef

# we can't use the standard kernel module rules because we don't build the kernel
define WIREGUARD_TEGRA210_KERNEL_MODULES_BUILD
    $(call MESSAGE,"Building kernel module(s)")
	$(foreach d,$(WIREGUARD_TEGRA210_MODULE_SUBDIRS), \
		$(LINUX_MAKE_ENV) $(MAKE) \
        -C $(@D)/$(d) \
        $(LINUX_MAKE_FLAGS) \
        KERNELRELEASE=4.9.253-tegra \
        KERNELDIR=$(TEGRA210_DIR)/kernel/kernel_headers/linux-headers-4.9.253-tegra-linux_x86_64/kernel-4.9 \
		module$(sep))
endef
WIREGUARD_TEGRA210_POST_BUILD_HOOKS += WIREGUARD_TEGRA210_KERNEL_MODULES_BUILD

define WIREGUARD_TEGRA210_KERNEL_MODULES_INSTALL
    $(call MESSAGE,"Installing kernel module(s)")
	$(foreach d,$(WIREGUARD_TEGRA210_MODULE_SUBDIRS), \
		$(LINUX_MAKE_ENV) $(MAKE) \
        -C $(@D)/$(d) \
        $(LINUX_MAKE_FLAGS) \
        KERNELRELEASE=4.9.253-tegra \
        KERNELDIR=$(TEGRA210_DIR)/kernel/kernel_headers/linux-headers-4.9.253-tegra-linux_x86_64/kernel-4.9 \
		DEPMODBASEDIR=$(TARGET_DIR) \
		module-install$(sep))
endef
WIREGUARD_TEGRA210_POST_INSTALL_TARGET_HOOKS += WIREGUARD_TEGRA210_KERNEL_MODULES_INSTALL

$(eval $(generic-package))
