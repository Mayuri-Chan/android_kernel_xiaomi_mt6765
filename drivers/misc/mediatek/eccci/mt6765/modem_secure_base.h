/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef __MODEM_SECURE_BASE_H__
#define __MODEM_SECURE_BASE_H__

#include <mt-plat/mtk_secure_api.h>

enum MD_REG_ID {
	MD_REG_AP_MDSRC_REQ = 0,
	MD_REG_EN_APB_CLK,
	MD_REG_PC_MONITOR,
	MD_REG_PLL_REG,
	MD_REG_BUS,
	MD_REG_MDMCU_BUSMON,
	MD_REG_MDINFRA_BUSMON,
	MD_REG_ECT,
	MD_REG_TOPSM_REG,
	MD_REG_MD_RGU_REG,
	MD_REG_OST_STATUS,
	MD_REG_CSC_REG,
	MD_REG_ELM_REG,
	MD_REG_USIP,
};

#define mdreg_write32(reg_id, value)		\
	mt_secure_call(MTK_SIP_KERNEL_CCCI_GET_INFO, reg_id, value, 0, 0)

#endif				/* __MODEM_SECURE_BASE_H__ */
