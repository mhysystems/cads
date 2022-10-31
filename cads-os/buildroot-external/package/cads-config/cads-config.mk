################################################################################
#
# cads-config
#
################################################################################

define CADS_CONFIG_INSTALL_INIT_SYSTEMD
	# terminal
	mkdir -p $(TARGET_DIR)/lib/systemd/system/getty.target.wants
	ln -snf ../getty@.service $(TARGET_DIR)/lib/systemd/system/getty.target.wants/getty@tty1.service

	# networks
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)10-modem.link $(TARGET_DIR)/lib/systemd/network/10-modem.link
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)10-wired.link $(TARGET_DIR)/lib/systemd/network/10-wired.link
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)20-wired.network $(TARGET_DIR)/lib/systemd/network/20-wired.network
endef

$(eval $(generic-package))
