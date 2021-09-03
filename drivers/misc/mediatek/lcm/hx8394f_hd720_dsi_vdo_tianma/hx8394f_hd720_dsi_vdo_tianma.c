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

#ifndef BUILD_LK
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

#define REGFLAG_DELAY        0xFE
#define REGFLAG_END_OF_TABLE 0xFF

// local variables
static struct LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))

#define UDELAY(n) udelay(n)
#define MDELAY(n) mdelay(n)


struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	/* set password */
	{0xB9, 3, {0xFF, 0x83, 0x94}},

	/* sleep out */
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0xBD, 1, {0x01}},
	{0xB1, 1, {0x00}},
	{0xBD, 1, {0x00}},
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{0x51, 1, {0x00}},
	{REGFLAG_DELAY, 5, {}},

	/* write pwm frequence */
	{0xC9, 9, {0x13, 0x00, 0x21, 0x1E, 0x31, 0x1E, 0x00, 0x91, 0x00}},
	{REGFLAG_DELAY, 5, {}},
	{0x55, 1, {0x00}},
	{REGFLAG_DELAY, 5, {}},
	{0x53, 1, {0x24}},
	{REGFLAG_DELAY, 5, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	/* Sleep Mode On */
	{0x28, 0, {}},
	{REGFLAG_DELAY, 50, {}},

	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF}},
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

	/* enable tearing-free */
	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;

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
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 15;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 30;
	params->dsi.horizontal_backporch = 30;
	params->dsi.horizontal_frontporch = 30;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.cont_clock = 1;
	params->dsi.PLL_CLOCK = 228;

	params->dsi.esd_check_enable = 1;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.customization_esd_check_enable = 1;

	params->dsi.lcm_esd_check_table[0].cmd = 0x09;
	params->dsi.lcm_esd_check_table[0].count = 3;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[0].para_list[2] = 0x04;

	params->dsi.lcm_esd_check_table[1].cmd = 0xD9;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;

	params->bias_voltage = 5700000;
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	pr_debug("[%s] hx8394f backlight level: %d\n", __func__, level);

	bl_level[0].para_list[0] = level;
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_init_power(void)
{
	display_bias_enable();
	MDELAY(15);
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	MDELAY(1);
	display_bias_disable();
}

static void lcm_resume_power(void)
{
	display_bias_enable();
	MDELAY(15);
}

static void lcm_init(void)
{
	SET_RESET_PIN(0);
	MDELAY(2);
	SET_RESET_PIN(1);
	MDELAY(50);

	push_table(NULL, lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK
	lcm_resume_power();
	lcm_init();
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
#endif

	return 0;
}

struct LCM_DRIVER hx8394f_hd720_dsi_vdo_tianma_lcm_drv = {
	.name = "hx8394f_hd720_dsi_vdo_tianma",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_recover = lcm_esd_recover,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_init,
	.set_backlight_cmdq = lcm_setbacklight_cmdq
};
