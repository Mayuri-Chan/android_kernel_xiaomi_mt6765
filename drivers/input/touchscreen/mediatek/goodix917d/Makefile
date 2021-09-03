#
# Makefile for the Goodix gt9xx touchscreen driver.
#
#subdir-ccflags-y += -DDEBUG
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
obj-$(CONFIG_TOUCHSCREEN_GT9XX)		+= gt9xx.o
obj-$(CONFIG_TOUCHSCREEN_GT9XX_UPDATE)	+= gt9xx_update.o
obj-$(CONFIG_TOUCHSCREEN_GT9XX_TOOL)	+= goodix_tool.o