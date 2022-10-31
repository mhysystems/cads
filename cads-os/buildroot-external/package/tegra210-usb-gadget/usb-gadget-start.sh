#!/bin/bash

# Copyright (c) 2022, MHY Systems Pty Ltd.  All rights reserved.
# Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -e

# Generate unique data
if [ -f /proc/device-tree/serial-number ]; then
    random="$(md5sum /proc/device-tree/serial-number|cut -c1-12)"
else
    random="$(echo "no-serial"|md5sum|cut -c1-12)"
fi
# Extract 6 bytes
b1="$(echo "${random}"|cut -c1-2)"
b2="$(echo "${random}"|cut -c3-4)"
b3="$(echo "${random}"|cut -c5-6)"
b4="$(echo "${random}"|cut -c7-8)"
b5="$(echo "${random}"|cut -c9-10)"
b6="$(echo "${random}"|cut -c11-12)"
# Clear broadcast/multicast, set locally administered bits
b1="$(printf "%02x" "$(("0x${b1}" & 0xfe | 0x02))")"
# Set 4 LSBs to unique value per interface
b6_rndis_h="$(printf "%02x" "$(("0x${b6}" & 0xfc | 0x00))")"
b6_rndis_d="$(printf "%02x" "$(("0x${b6}" & 0xfc | 0x01))")"
b6_ecm_h="$(printf "%02x" "$(("0x${b6}" & 0xfc | 0x02))")"
b6_ecm_d="$(printf "%02x" "$(("0x${b6}" & 0xfc | 0x03))")"
# Construct complete MAC per interface
mac_rndis_h="${b1}:${b2}:${b3}:${b4}:${b5}:${b6_rndis_h}"
mac_rndis_d="${b1}:${b2}:${b3}:${b4}:${b5}:${b6_rndis_d}"
mac_ecm_h="${b1}:${b2}:${b3}:${b4}:${b5}:${b6_ecm_h}"
mac_ecm_d="${b1}:${b2}:${b3}:${b4}:${b5}:${b6_ecm_d}"

mkdir -p /sys/kernel/config/usb_gadget/l4t
cd /sys/kernel/config/usb_gadget/l4t

# If this script is modified outside NVIDIA, the idVendor and idProduct values
# MUST be replaced with appropriate vendor-specific values.
echo 0x0955 > idVendor
echo 0x7020 > idProduct
# BCD value. Each nibble should be 0..9. 0x1234 represents version 12.3.4.
echo 0x0002 > bcdDevice

# Informs Windows that this device is a composite device, i.e. it implements
# multiple separate protocols/devices.
echo 0xEF > bDeviceClass
echo 0x02 > bDeviceSubClass
echo 0x01 > bDeviceProtocol

mkdir -p strings/0x409
if [ -f /proc/device-tree/serial-number ]; then
    serialnumber="$(cat /proc/device-tree/serial-number|tr -d '\000')"
else
    serialnumber=no-serial
fi
echo "${serialnumber}" > strings/0x409/serialnumber
# If this script is modified outside NVIDIA, the manufacturer and product values
# MUST be replaced with appropriate vendor-specific values.
echo "NVIDIA" > strings/0x409/manufacturer
echo "Linux for Tegra" > strings/0x409/product

cfg=configs/c.1
mkdir -p "${cfg}"
cfg_str=""

# Note: RNDIS must be the first function in the configuration, or Windows'
# RNDIS support will not operate correctly.
cfg_str="${cfg_str}+RNDIS"
func=functions/rndis.usb0
mkdir -p "${func}"
echo "${mac_rndis_h}" > "${func}/host_addr"
echo "${mac_rndis_d}" > "${func}/dev_addr"
ln -sf "${func}" "${cfg}"

# Informs Windows that this device is compatible with the built-in RNDIS
# driver. This allows automatic driver installation without any need for
# a .inf file or manual driver selection.
echo 1 > os_desc/use
echo 0xcd > os_desc/b_vendor_code
echo MSFT100 > os_desc/qw_sign
echo RNDIS > "${func}/os_desc/interface.rndis/compatible_id"
echo 5162001 > "${func}/os_desc/interface.rndis/sub_compatible_id"
ln -sf "${cfg}" os_desc

# If two USB configs are created, and the second contains RNDIS and ACM, then
# Windows will ignore at the ACM function in that config. Consequently, this
# script creates only a single USB config.
cfg_str="${cfg_str}+ACM"
func=functions/acm.GS0
mkdir -p "${func}"
ln -sf "${func}" "${cfg}"

cfg_str="${cfg_str}+NCM"
func=functions/ncm.usb0
mkdir -p "${func}"
echo "${mac_ecm_h}" > "${func}/host_addr"
echo "${mac_ecm_d}" > "${func}/dev_addr"
ln -sf "${func}" "${cfg}"

mkdir -p "${cfg}/strings/0x409"
# :1 in the variable expansion strips the first character from the value. This
# removes the unwanted leading + sign. This simplifies the logic to construct
# $cfg_str above; it can always add a leading delimiter rather than only doing
# so unless the string is previously empty.
echo "${cfg_str:1}" > "${cfg}/strings/0x409/configuration"

echo 700d0000.xudc > UDC

exit 0
