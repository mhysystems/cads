################################################################################
#
# tegra210-usb-gadget
#
################################################################################

TEGRA210_USB_GADGET_DEPENDENCIES += systemd tegra210

define TEGRA210_USB_GADGET_INSTALL_COMMON
	$(RM) $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants/nv-l4t-usb-device-mode.service
	$(RM) $(TARGET_DIR)/etc/systemd/system/nv-l4t-usb-device-mode.service
	$(RM) $(TARGET_DIR)/etc/systemd/system/nv-l4t-usb-device-mode-runtime.service
	$(INSTALL) -D -m 0755 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget-start.sh \
		$(TARGET_DIR)/usr/libexec/usb-gadget/usb-gadget-start.sh
	$(INSTALL) -D -m 0755 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget-stop.sh \
		$(TARGET_DIR)/usr/libexec/usb-gadget/usb-gadget-stop.sh
endef

define TEGRA210_USB_GADGET_INSTALL_INIT_SYSTEMD
	$(TEGRA210_USB_GADGET_INSTALL_COMMON)
	$(INSTALL) -D -m 0644 $(TEGRA210_USB_GADGET_PKGDIR)/usb-gadget.service \
		$(TARGET_DIR)/lib/systemd/system/usb-gadget.service
endef

$(eval $(generic-package))
