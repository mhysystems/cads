################################################################################
#
# tegra210-usb-gadget
#
################################################################################

TEGRA210_USB_GADGET_DEPENDENCIES += systemd

define TEGRA210_USB_GADGET_INSTALL_COMMON
	$(INSTALL) -D -m 0755 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget-start.sh \
		$(TARGET_DIR)/usr/libexec/usb-gadget/usb-gadget-start.sh
	$(INSTALL) -D -m 0755 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget-stop.sh \
		$(TARGET_DIR)/usr/libexec/usb-gadget/usb-gadget-stop.sh
endef

define TEGRA210_USB_GADGET_INSTALL_INIT_SYSTEMD
	$(INSTALL) -D -m 0644 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget.service \
		$(TARGET_DIR)/lib/systemd/system/usb-gadget.service
	$(INSTALL) -D -m 0644 $(TEGRA210_USB_GADGET_PKGDIR)/20-usb-gadget.network \
		$(TARGET_DIR)/lib/systemd/network/20-usb-gadget.network
endef

$(eval $(generic-package))
