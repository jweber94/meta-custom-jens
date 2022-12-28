SUMMARY = "Custom image by Jens Weber to familiarize with yocto"

IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image
inherit extrausers

# Set rootfs to 200MiB by default
IMAGE_OVERHEAD_FACTOR ?= "1.0" 
# The default is 1.5, so if we want to hard set it with IMAGE_ROOTFS_SIZE, we need to know how big the rootfs is to not violate its demanded size and then add how much space we want to have left over
IMAGE_ROOTFS_SIZE ?= "204800"

# Add i2c and tell the image to install the WIFI driver
IMAGE_INSTALL:append = " python3 i2c-tools get-i2c-data kernel-module-r8188eu linux-firmware-rtl8188 iw wpa-supplicant wireless-regdb-static"
CORE_IMAGE_INSTALL:append = " dhcp-client"

DEPENDS:append = " dtc-native"

# Autoload the wifi driver (no need to modprobe or insmod)
# KERNEL_MODULE_AUTOLOAD += "r8188eu"

# Change root password - this is possible because we inherit extrausers
EXTRA_USERS_PARAMS = "\
	usermod -p '\$5\$67FgRwGQd2BRs5L\$Y2gzsKDEeUKLy/L7eFPfo1fxSp3bb0Pln9fOor4MPi0' root; \
	"

