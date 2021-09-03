#
# Makefile for misc devices that really don't fit anywhere else.
#

obj-$(CONFIG_MTK_LENS_MAIN)   += main/
obj-$(CONFIG_MTK_LENS_MAIN2)   += main2/
ifeq ($(CONFIG_MACH_MT6761), y)
	ifeq ($(CONFIG_EXTREME_LOW_RAM), y)
	else
	obj-$(CONFIG_MTK_LENS_MAIN3)   += main3/
	endif
else
obj-$(CONFIG_MTK_LENS_MAIN3)   += main3/
endif
obj-$(CONFIG_MTK_LENS_SUB)   += sub/
ifeq ($(CONFIG_MACH_MT6761), y)
	ifeq ($(CONFIG_EXTREME_LOW_RAM), y)
	else
	obj-$(CONFIG_MTK_LENS_SUB2)   += sub2/
	endif
else
obj-$(CONFIG_MTK_LENS_SUB2)   += sub2/
endif
