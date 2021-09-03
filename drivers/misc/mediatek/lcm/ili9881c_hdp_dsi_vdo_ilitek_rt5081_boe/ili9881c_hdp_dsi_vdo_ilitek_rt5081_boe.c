/*
 * Copyright (C) 2015 MediaTek Inc.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <linux/string.h>
#endif

#include "lcm_drv.h"
#include <linux/delay.h>

// local Constants
#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1440)

// physical size in um
#define LCM_PHYSICAL_WIDTH  (61880)
#define LCM_PHYSICAL_HEIGHT (123770)

#define REGFLAG_DELAY        0xFFFC
#define REGFLAG_END_OF_TABLE 0xFFFD

// local variables
static struct LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))

#define UDELAY(n) udelay(n)
#define MDELAY(n) mdelay(n)

#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x00}},
	{0x28, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {}}
};

static struct LCM_setting_table init_setting_cmd[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x03}}
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x00}},
	{0x11, 0x01, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0xFF, 0x03, {0x98, 0x81, 0x04}},
	{0x0B, 0x01, {0x90}},
	{0x0E, 0x01, {0x0B}},
	{0x35, 0x01, {0x17}},
	{0x3A, 0x01, {0x92}},
	{0xB5, 0x01, {0x27}},
	{0x3B, 0x01, {0x98}},
	{0x31, 0x01, {0x75}},
	{0x30, 0x01, {0x03}},
	{0x38, 0x01, {0x01}},
	{0x39, 0x01, {0x00}},
	{0xFF, 0x03, {0x98, 0x81, 0x05}},
	{0x85, 0x01, {0x40}},
	{0xFF, 0x03, {0x98, 0x81, 0x02}},
	{0x06, 0x01, {0x30}},
	{0x07, 0x01, {0x07}},
	{0xFF, 0x03, {0x98, 0x81, 0x00}},
	{0x35, 0x01, {0x00}},
	{0x53, 0x01, {0x2C}},
	{0x55, 0x01, {0x00}},
	{0x29, 0x01, {0x00}},
	{REGFLAG_DELAY, 20, {}}
};

static struct LCM_setting_table bl_level[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x00}},
	{0x51, 0x02, {0x0F, 0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(void *cmdq, struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	for (i = 0; i < count; i++) {
		unsigned int cmd;

		cmd = table[i].cmd;
		switch (cmd) {
			case REGFLAG_DELAY:
				MDELAY(table[i].count);
				break;
			case REGFLAG_END_OF_TABLE:
				break;
			default:
				lcm_util.dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
				break;
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH / 1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT / 1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;

	params->dsi.switch_mode_enable = 0;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;

	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	/* The following defines the format of data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 18;
	params->dsi.vertical_frontporch = 40;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 22;
	params->dsi.horizontal_backporch = 80;
	params->dsi.horizontal_frontporch = 100;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_range = 4;
	params->dsi.ssc_disable = 0;

	params->dsi.PLL_CLOCK = 266;
	params->dsi.PLL_CK_CMD = 220;
	params->dsi.PLL_CK_VDO = 266;

	params->dsi.esd_check_enable = 1;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x09;
	params->dsi.lcm_esd_check_table[0].count = 3;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x03;
	params->dsi.lcm_esd_check_table[0].para_list[2] = 0x06;

	params->bias_voltage = 6000000;
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO2_LCM_1V8_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO2_LCM_1V8_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO2_LCM_1V8_EN, GPIO_OUT_ONE);
	MDELAY(2);

	mt_set_gpio_mode(GPIO25_LCM_PO_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO25_LCM_PO_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO25_LCM_PO_EN, GPIO_OUT_ONE);
	MDELAY(2);

	mt_set_gpio_mode(GPIO26_LCM_NO_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO26_LCM_NO_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO26_LCM_NO_EN, GPIO_OUT_ONE);

	MDELAY(20);
#else
	display_bias_enable();
	MDELAY(15);
#endif
}

static void lcm_suspend_power(void)
{
#ifdef BUILD_LK
	mt_set_gpio_out(GPIO2_LCM_1V8_EN, GPIO_OUT_ZERO);
	MDELAY(2);

	mt_set_gpio_out(GPIO26_LCM_NO_EN, GPIO_OUT_ZERO);
	MDELAY(2);

	mt_set_gpio_out(GPIO25_LCM_PO_EN, GPIO_OUT_ZERO);
#else
	SET_RESET_PIN(0);
	MDELAY(2);
	display_bias_disable();
#endif
}

static void lcm_resume_power(void)
{
	display_bias_enable();
	MDELAY(15);
}

static void lcm_init(void)
{
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO45_LCM_RESET, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO45_LCM_RESET, GPIO_DIR_OUT);

	mt_set_gpio_out(GPIO45_LCM_RESET, GPIO_OUT_ONE);
	MDELAY(1);
	mt_set_gpio_out(GPIO45_LCM_RESET, GPIO_OUT_ZERO);
	MDELAY(1);
	mt_set_gpio_out(GPIO45_LCM_RESET, GPIO_OUT_ONE);
	MDELAY(20);
#else
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(15);

	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
	}
#endif
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id, version_id;
	unsigned char buffer[2];
	unsigned int array[16];

	struct LCM_setting_table switch_table_page0[] = {
		{0xFF, 0x03, {0x98, 0x81, 0x00}}
	};

	struct LCM_setting_table switch_table_page1[] = {
		{0xFF, 0x03, {0x98, 0x81, 0x01}}
	};


	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	push_table(NULL, switch_table_page1, sizeof(switch_table_page1) / sizeof(struct LCM_setting_table), 1);

	array[0] = 0x23700; // read two bytes, version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0, buffer, 1);
	id = buffer[0]; // we only need ID

	read_reg_v2(1, buffer, 1);
	version_id = buffer[0];

	pr_debug("[%s] boe_ili9881c id: 0x%08x, version_id: 0x%x\n", __func__, id, version_id);
	push_table(NULL, switch_table_page0, sizeof(switch_table_page0) / sizeof(struct LCM_setting_table), 1);

	if (id == 0x98 && version_id == 0x81) {
		return 1;
	}

	return 0;
}


static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	pr_debug("[%s] boe_ili9881c backlight level: %d\n", __func__, level);

	// from 12 bits to 8 bits
	bl_level[1].para_list[0] = (level & 0xF0) >> 4;
	bl_level[1].para_list[1] = (level & 0x0F) << 4;

	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

/* return 1: need to recovery
   return 0: no need to recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x13700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
	pr_debug("[%s] boe_ili9881c 0x0A: 0x%02x\n", __func__, buffer[0]);

	if (buffer[0] != 0x9C) {
		return 1;
	}
#endif

	return 0;
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK
	lcm_resume_power();

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(120);

	pr_debug("[%s] boe_ili9881c lcm dsi mode: %d\n", __func__, lcm_dsi_mode);
	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
	}

	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
#endif

	return 0;
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	unsigned int ret;
#ifndef BUILD_LK
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = (x0 >> 8) & 0xFF;
	unsigned char x0_LSB = x0 & 0xFF;
	unsigned char x1_MSB = (x1 >> 8) & 0xFF;
	unsigned char x1_LSB = x1 & 0xFF;

	unsigned int data_array[3];
	unsigned char read_buf[4];

	pr_debug("[%s] boe_ili9881c ata check size: 0x%x, 0x%x, 0x%x, 0x%x\n", __func__, x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x5390A; // HS packet
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x43700; // read two bytes, version and id
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);
	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB) && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB)) {
		ret = 1;
	} else {
		ret = 0;
	}

	x0_MSB = 0;
	x0_LSB = 0;

	x1 = FRAME_WIDTH - 1;
	x1_MSB = (x1 >> 8) & 0xFF;
	x1_LSB = x1 & 0xFF;

	data_array[0] = 0x5390A; // HS packet
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
#endif

	return ret;
}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
	static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

	if (mode == 0) { // V2C
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;   // mode control addr
		lcm_switch_mode_cmd.val[0] = 0x13; // enabel GRAM firstly, ensure writing one frame to GRAM
		lcm_switch_mode_cmd.val[1] = 0x10; // disable video mode secondly
	} else { // C2V
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03; // disable GRAM and enable video mode
	}

	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

struct LCM_DRIVER ili9881c_hdp_dsi_vdo_ilitek_rt5081_lcm_drv_boe = {
	.name = "ili9881c_hdp_dsi_vdo_ilitek_rt5081_drv_boe",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_init,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.switch_mode = lcm_switch_mode
};
