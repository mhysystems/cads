################################################################################
#
# Support package for CADS system
#
################################################################################

define CADS_SUPPORT_INSTALL_TARGET_CMDS
	ln -s /lib/systemd/system/getty@.service $(TARGET_DIR)/etc/systemd/system/getty.target.wants/getty@tty1.service
endef

$(eval $(generic-package))
