#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

#
# Makefile for misc devices that really don't fit anywhere else.
#

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_CUSTOM_PATH := $(srctree)/drivers/misc/mediatek

subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/main/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/timer/timesync/

ifeq ($(CONFIG_MTK_LENS_USE_MODULE), y)
MAIN_CFILES += main_lens.c

ifeq ($(CONFIG_MTK_LENS_CACTUS_DW9714AF_OFILM_SUPPORT), y)
MAIN_CFILES += common/cactus_dw9714af_ofilm/CACTUS_DW9714AF_OFILM.c
endif
ifeq ($(CONFIG_MTK_LENS_CACTUS_FP5510E2AF_SUNNY_SUPPORT), y)
MAIN_CFILES += common/cactus_fp5510e2af_sunny/CACTUS_FP5510E2AF_SUNNY.c
endif
ifeq ($(CONFIG_MTK_LENS_CEREUS_DW9714AF_OFILM_SUPPORT), y)
MAIN_CFILES += common/cereus_dw9714af_ofilm/CEREUS_DW9714AF_OFILM.c
endif
ifeq ($(CONFIG_MTK_LENS_CEREUS_DW9714AF_SUNNY_SUPPORT), y)
MAIN_CFILES += common/cereus_dw9714af_sunny/CEREUS_DW9714AF_SUNNY.c
endif

obj-m := mainaf.o
mainaf-objs := $(MAIN_CFILES:.c=.o)
else
obj-y += main_lens.o

obj-$(CONFIG_MTK_LENS_CACTUS_DW9714AF_OFILM_SUPPORT) += common/cactus_dw9714af_ofilm/
obj-$(CONFIG_MTK_LENS_CACTUS_FP5510E2AF_SUNNY_SUPPORT) += common/cactus_fp5510e2af_sunny/
obj-$(CONFIG_MTK_LENS_CEREUS_DW9714AF_OFILM_SUPPORT) += common/cereus_dw9714af_ofilm/
obj-$(CONFIG_MTK_LENS_CEREUS_DW9714AF_SUNNY_SUPPORT) += common/cereus_dw9714af_sunny/
endif
