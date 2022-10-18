################################################################################
#
# cpr
#
################################################################################

CPR_VERSION = 1.9.2
CPR_SITE = $(call github,libcpr,cpr,$(CPR_VERSION))
CPR_LICENSE = MIT
CPR_INSTALL_STAGING = YES
CPR_CONF_OPTS = -DCPR_BUILD_TESTS=OFF -DCPR_FORCE_USE_SYSTEM_CURL=ON

$(eval $(cmake-package))
