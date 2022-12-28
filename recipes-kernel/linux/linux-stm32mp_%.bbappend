FILESEXTRAPATHS:prepend := "${THISDIR}:${THISDIR}/files:"

# Update the device tree
SRC_URI:append = " file://0023-add-i2c5-userspace-dts.patch"

# Apply the default configuration from the defconfig file (out of meunconfig)
KERNEL_DEFCONFIG_stm32mp1 = "defconfig"

# Autoload the wifi driver (no need to modprobe or insmod)
KERNEL_MODULE_AUTOLOAD += "r8188eu"
