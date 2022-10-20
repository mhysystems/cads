TOPDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
O ?= $(CURDIR)/output

export BR2_EXTERNAL := $(TOPDIR)buildroot-external

.DEFAULT_GOAL := all

.DEFAULT:
	$(MAKE) O=$(O) -C buildroot $@
