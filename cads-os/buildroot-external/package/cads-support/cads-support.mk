################################################################################
#
# Support package for CADS system
#
################################################################################

define CADS_SUPPORT_INSTALL_INIT_SYSTEMD
	mkdir -p $(TARGET_DIR)/lib/systemd/system/getty.target.wants
	ln -snf ../getty@.service $(TARGET_DIR)/lib/systemd/system/getty.target.wants/getty@tty1.service
endef

$(eval $(generic-package))
