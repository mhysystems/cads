################################################################################
#
# NVIDIA hardware packages
#
################################################################################

# $(BUILD_DIR)/hardware/nvidia maps to Linux_for_Tegra/sources/hardware/nvidia
NV_BUILD_KERNEL_DTS_ROOT = $(BUILD_DIR)/hardware/nvidia

include $(sort $(wildcard $(BR2_EXTERNAL_DEFAULT_PATH)/package/hardware-nvidia/*/*.mk))
