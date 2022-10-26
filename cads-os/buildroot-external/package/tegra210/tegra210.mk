################################################################################
#
# Linux Tegra210 Board Support Package
#
################################################################################

TEGRA210_VERSION = 32.7.1
TEGRA210_SITE = https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/t210
TEGRA210_SOURCE = jetson-210_linux_r$(TEGRA210_VERSION)_aarch64.tbz2
TEGRA210_LICENSE = NVIDIA Software License, GPL-2.0, LGPL, Apache-2.0, MIT
TEGRA210_LICENSE_FILES = \
	bootloader/LICENSE \
	bootloader/LICENSE.chkbdinfo \
	bootloader/LICENSE.mkbctpart \
	bootloader/LICENSE.mkbootimg \
	bootloader/LICENSE.mkgpt \
	bootloader/LICENSE.mksparse \
	bootloader/LICENSE.tegraopenssl \
	bootloader/LICENSE.tos-mon-only.img.arm-trusted-firmware \
	bootloader/LICENSE.u-boot \
	nv_tegra/nvidia_configs/opt/nvidia/l4t-usb-device-mode/LICENSE.filesystem.img \
	nv_tegra/LICENSE.libtegrav4l2 \
	nv_tegra/LICENSE.libnvcam_imageencoder \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libvulkan1 \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvargus \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvv4lconvert \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libtegrav4l2 \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvv4l2 \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.nvdla \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.cypress_wifibt \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.minigbm \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvjpeg \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.brcm_patchram_plus \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvcam_imageencoder \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnvtracebuf \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.realtek_8822ce_wifibt \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.tegra_sensors \
	nv_tegra/nvidia_drivers/usr/share/doc/nvidia-tegra/LICENSE.libnveventlib \
	nv_tegra/LICENSE.libnveventlib \
	nv_tegra/LICENSE.libnvscf \
	nv_tegra/LICENSE.libnvargus \
	nv_tegra/LICENSE.minigbm \
	nv_tegra/LICENSE.wayland-ivi-extension \
	nv_tegra/nv_sample_apps/LICENSE.gst-nvvideo4linux2 \
	nv_tegra/nv_sample_apps/LICENSE.gstvideocuda \
	nv_tegra/nv_sample_apps/LICENSE.gst-openmax \
	nv_tegra/nv_sample_apps/LICENSE.libgstnvdrmvideosink \
	nv_tegra/nv_sample_apps/LICENSE.libgstnvvideosinks \
	nv_tegra/LICENSE \
	nv_tegra/LICENSE.weston-data \
	nv_tegra/LICENSE.libnvtracebuf \
	nv_tegra/LICENSE.weston \
	nv_tegra/LICENSE.nvdla \
	nv_tegra/LICENSE.l4t-usb-device-mode-filesystem.img \
	nv_tegra/LICENSE.brcm_patchram_plus

TEGRA210_REDISTRIBUTE = NO
TEGRA210_INSTALL_IMAGES = YES
TEGRA210_DEPENDENCIES = host-kmod host-python3

define TEGRA210_EXTRACT_NVIDIA_DRIVERS
	@mkdir -p $(@D)/nv_tegra/nvidia_drivers
	$(call suitable-extractor,nvidia_drivers.tbz2) \
		$(@D)/nv_tegra/nvidia_drivers.tbz2 | \
		$(TAR) -C $(@D)/nv_tegra/nvidia_drivers $(TAR_OPTIONS) -
endef

TEGRA210_POST_EXTRACT_HOOKS += TEGRA210_EXTRACT_NVIDIA_DRIVERS

define TEGRA210_EXTRACT_NVIDIA_CONFIGS
	@mkdir -p $(@D)/nv_tegra/nvidia_configs
	$(call suitable-extractor,config.tbz2) \
		$(@D)/nv_tegra/config.tbz2 | \
		$(TAR) -C $(@D)/nv_tegra/nvidia_configs $(TAR_OPTIONS) -
endef

TEGRA210_POST_EXTRACT_HOOKS += TEGRA210_EXTRACT_NVIDIA_CONFIGS

define TEGRA210_EXTRACT_NVIDIA_TOOLS
	@mkdir -p $(@D)/nv_tegra/nvidia_tools
	$(call suitable-extractor,nv_tools.tbz2) \
		$(@D)/nv_tegra/nv_tools.tbz2 | \
		$(TAR) -C $(@D)/nv_tegra/nvidia_tools $(TAR_OPTIONS) -
endef

TEGRA210_POST_EXTRACT_HOOKS += TEGRA210_EXTRACT_NVIDIA_TOOLS

define TEGRA210_EXTRACT_KERNEL_SUPPLEMENTS
	@mkdir -p $(@D)/kernel/kernel_supplements
	$(call suitable-extractor,kernel_supplements.tbz2) \
		$(@D)/kernel/kernel_supplements.tbz2 | \
		$(TAR) -C $(@D)/kernel/kernel_supplements $(TAR_OPTIONS) -
endef

TEGRA210_POST_EXTRACT_HOOKS += TEGRA210_EXTRACT_KERNEL_SUPPLEMENTS

define TEGRA210_EXTRACT_KERNEL_HEADERS
	@mkdir -p $(@D)/kernel/kernel_headers
	$(call suitable-extractor,kernel_headers.tbz2) \
		$(@D)/kernel/kernel_headers.tbz2 | \
		$(TAR) -C $(@D)/kernel/kernel_headers $(TAR_OPTIONS) -
endef

TEGRA210_POST_EXTRACT_HOOKS += TEGRA210_EXTRACT_KERNEL_HEADERS

define TEGRA210_CONFIGURE_CMDS
	cp $(@D)/bootloader/t210ref/nvtboot.bin $(@D)/bootloader/nvtboot.bin
	cp $(@D)/bootloader/t210ref/cboot.bin $(@D)/bootloader/cboot.bin
	cp $(@D)/kernel/dtb/tegra210-p3448-0000-p3449-0000-b00.dtb \
		$(@D)/bootloader/tegra210-p3448-0000-p3449-0000-b00.dtb
	cp $(@D)/bootloader/t210ref/warmboot.bin $(@D)/bootloader/warmboot.bin
	cp $(@D)/bootloader/t210ref/sc7entry-firmware.bin $(@D)/bootloader/sc7entry-firmware.bin
	cp $(@D)/bootloader/t210ref/BCT/P3448_A00_lpddr4_204Mhz_P987.cfg \
		$(@D)/bootloader/P3448_A00_lpddr4_204Mhz_P987.cfg
	cp $(@D)/bootloader/t210ref/cfg/flash_l4t_t210_spi_sd_p3448.xml $(@D)/bootloader/flash.xml

	sed -i -e 's/NXC/NVC/' \
		-e 's/NVCTYPE/bootloader/' \
		-e 's/NVCFILE/nvtboot.bin/' \
		-e 's/VERFILE/qspi_bootblob_ver.txt/g' \
		-e 's/EBTFILE/cboot.bin/' \
		-e 's/TBCFILE/nvtboot_cpu.bin/' \
		-e 's/TXC/TBC/' \
		-e 's/TXS/TOS/' \
		-e 's/TOSFILE/tos-mon-only.img/' \
		-e 's/TBCTYPE/bootloader/' \
		-e "s/DTBFILE/tegra210-p3448-0000-p3449-0000-b00.dtb/" \
		-e 's/WB0TYPE/WB0/' \
		-e 's/WB0FILE/warmboot.bin/' \
		-e 's/BXF/BPF/' \
		-e 's/BPFFILE/sc7entry-firmware.bin/' \
		-e 's/WX0/WB0/' \
		-e 's/DXB/DTB/' \
		-e 's/EXS/EKS/' \
		-e 's/EKSFILE/eks.img/' \
		-e 's/FBTYPE/data/' \
		-e 's/LNXFILE/boot.img/' \
		-e 's/APPUUID//' \
		-e 's/APPFILE/system.img/' \
		-e '/FBFILE/d' \
		-e '/BPFDTB-FILE/d' \
		-e "s/APPSIZE/"$(BR2_TARGET_ROOTFS_EXT2_SIZE)"/" \
		$(@D)/bootloader/flash.xml
endef

define TEGRA210_BUILD_CMDS
	cd $(@D)/bootloader && \
	./mkbootimg --kernel t210ref/p3450-0000/u-boot.bin \
	--ramdisk /dev/null \
	--board mmcblk0p1 \
	--output boot.img \
	--cmdline 'root=/dev/mmcblk0p1 rw rootwait rootfstype=ext4 console=ttyS0,115200n8 console=tty0 fbcon=map:0 net.ifnames=0'

	cd $(@D)/bootloader && \
	./tegraflash.py --bl cboot.bin \
	--bct P3448_A00_lpddr4_204Mhz_P987.cfg \
	--odmdata 0x94000 \
	--bldtb tegra210-p3448-0000-p3449-0000-b00.dtb \
	--applet nvtboot_recovery.bin \
	--cmd "sign" \
	--cfg flash.xml \
	--chip 0x21 \
	--bins "EBT cboot.bin; DTB tegra210-p3448-0000-p3449-0000-b00.dtb"
endef

define TEGRA210_INSTALL_IMAGES_CMDS
	$(INSTALL) -m 0644 $(@D)/bootloader/boot.img $(BINARIES_DIR)/boot.img
	$(INSTALL) -m 0644 $(@D)/bootloader/bmp.blob $(BINARIES_DIR)/bmp.blob
	$(INSTALL) -m 0644 $(@D)/bootloader/rp4.blob $(BINARIES_DIR)/rp4.blob
	$(INSTALL) -m 0644 $(@D)/bootloader/eks.img $(BINARIES_DIR)/eks.img

	$(INSTALL) -m 0644 $(@D)/bootloader/signed/boot.img.encrypt $(BINARIES_DIR)/boot.img.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/cboot.bin.encrypt $(BINARIES_DIR)/cboot.bin.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/flash.xml $(BINARIES_DIR)/flash.xml
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/flash.xml.bin $(BINARIES_DIR)/flash.xml.bin
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/nvtboot.bin.encrypt $(BINARIES_DIR)/nvtboot.bin.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/nvtboot_cpu.bin.encrypt $(BINARIES_DIR)/nvtboot_cpu.bin.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/P3448_A00_lpddr4_204Mhz_P987.bct \
		$(BINARIES_DIR)/P3448_A00_lpddr4_204Mhz_P987.bct
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/sc7entry-firmware.bin.encrypt $(BINARIES_DIR)/sc7entry-firmware.bin.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/tegra210-p3448-0000-p3449-0000-b00.dtb.encrypt \
		$(BINARIES_DIR)/tegra210-p3448-0000-p3449-0000-b00.dtb.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/tos-mon-only.img.encrypt $(BINARIES_DIR)/tos-mon-only.img.encrypt
	$(INSTALL) -m 0644 $(@D)/bootloader/signed/warmboot.bin.encrypt $(BINARIES_DIR)/warmboot.bin.encrypt
endef

TEGRA210_RSYNC = \
	rsync -a --ignore-times $(RSYNC_VCS_EXCLUSIONS) \
		--chmod=u=rwX,go=rX --exclude .empty --exclude '*~' \
		--keep-dirlinks --exclude=ld.so.conf.d

define TEGRA210_INSTALL_TARGET_CMDS
	# install nvidia_drivers
	$(TEGRA210_RSYNC) $(@D)/nv_tegra/nvidia_drivers/ $(TARGET_DIR)/
	# install nvidia_configs
	$(TEGRA210_RSYNC) $(@D)/nv_tegra/nvidia_configs/ $(TARGET_DIR)/
	# install nvidia_tools
	$(TEGRA210_RSYNC) $(@D)/nv_tegra/nvidia_tools/ $(TARGET_DIR)/
	# install kernel_supplements
	$(TEGRA210_RSYNC) $(@D)/kernel/kernel_supplements/ $(TARGET_DIR)/

	$(INSTALL) -D -m 0644 $(@D)/kernel/Image $(TARGET_DIR)/boot/Image
	$(INSTALL) -D -m 0644 $(@D)/bootloader/extlinux.conf $(TARGET_DIR)/boot/extlinux/extlinux.conf
	$(INSTALL) -D -m 0644 $(@D)/bootloader/l4t_initrd.img $(TARGET_DIR)/boot/initrd
	$(INSTALL) -D -m 0644 $(@D)/bootloader/tegra210-p3448-0000-p3449-0000-b00.dtb \
		$(TARGET_DIR)/boot/tegra210-p3448-0000-p3449-0000-b00.dtb
	$(INSTALL) -D -m 0644 $(@D)/bootloader/nv_boot_control.conf $(TARGET_DIR)/etc/nv_boot_control.conf

	sed -i /TNSPEC/d $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i '$$ a TNSPEC 3448-300---1-0-jetson-nano-qspi-sd-mmcblk0p1' $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i /TEGRA_CHIPID/d $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i '$$ a TEGRA_CHIPID 0x21' $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i /TEGRA_OTA_BOOT_DEVICE/d $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i '$$ a TEGRA_OTA_BOOT_DEVICE /dev/mtdblock0' $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i /TEGRA_OTA_GPT_DEVICE/d $(TARGET_DIR)/etc/nv_boot_control.conf
	sed -i '$$ a TEGRA_OTA_GPT_DEVICE /dev/mtdblock0' $(TARGET_DIR)/etc/nv_boot_control.conf
endef

$(eval $(generic-package))
