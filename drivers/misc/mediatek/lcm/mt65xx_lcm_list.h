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

#ifndef __MT65XX_LCM_LIST_H__
#define __MT65XX_LCM_LIST_H__

#include <lcm_drv.h>

extern struct LCM_DRIVER hx8394f_hd720_dsi_vdo_tianma_lcm_drv;
extern struct LCM_DRIVER ili9881c_hdp_dsi_vdo_ilitek_rt5081_lcm_drv_boe;
extern struct LCM_DRIVER ili9881c_hdp_dsi_vdo_ilitek_rt5081_lcm_drv_ebbg;

#ifdef BUILD_LK
extern void mdelay(unsigned long msec);
#endif

#endif
