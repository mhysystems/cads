################################################################################
#
# cads-config
#
################################################################################

define CADS_CONFIG_INSTALL_INIT_SYSTEMD
	# terminal
	mkdir -p $(TARGET_DIR)/lib/systemd/system/getty.target.wants
	ln -snf ../getty@.service $(TARGET_DIR)/lib/systemd/system/getty.target.wants/getty@tty1.service

	# modem
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/10-modem.link $(TARGET_DIR)/lib/systemd/network/10-modem.link

	# ethernet
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/20-wired.link $(TARGET_DIR)/lib/systemd/network/20-wired.link
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/20-wired.network $(TARGET_DIR)/lib/systemd/network/20-wired.network

	# usb ethernet
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/30-rndis0.network $(TARGET_DIR)/lib/systemd/network/30-rndis0.network
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/30-usb0.network $(TARGET_DIR)/lib/systemd/network/30-usb0.network
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/40-l4tbr0.netdev $(TARGET_DIR)/lib/systemd/network/40-l4tbr0.netdev
	$(INSTALL) -D -m 0644 $(CADS_CONFIG_PKGDIR)/40-l4tbr0.network $(TARGET_DIR)/lib/systemd/network/40-l4tbr0.network
endef

$(eval $(generic-package))
