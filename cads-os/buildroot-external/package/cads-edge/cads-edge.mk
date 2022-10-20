################################################################################
#
# cads-edge
#
################################################################################

CADS_EDGE_VERSION = 0.0.1
CADS_EDGE_SITE = $(TOPDIR)/../cads-edge
CADS_EDGE_SITE_METHOD = local
CADS_EDGE_CXXFLAGS = $(TARGET_CXXFLAGS)

$(eval $(meson-package))
