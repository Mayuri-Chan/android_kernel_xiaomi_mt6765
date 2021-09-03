/*
 * Copyright (C) 2018 MediaTek Inc.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "cactus_s5k3l8_sunny_Sensor.h"
#include "cam_cal_define.h"

#define PFX "cactus_s5k3l8_sunny_camera_sensor"
#define LOG_DBG(format, args...) pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = CACTUS_S5K3L8_SUNNY_SENSOR_ID,
	.checksum_value = 0x49C09F86,

	.pre = {
		.pclk = 560000000,
		.linelength = 5808,
		.framelength = 3206,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2104,
		.grabwindow_height = 1560,
		.mipi_data_lp2hs_settle_dc = 21,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 560000000,
		.linelength = 5808,
		.framelength = 3206,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 21,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 400000000,
		.linelength = 5808,
		.framelength = 4589,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,
	},
	.normal_video = {
		.pclk = 560000000,
		.linelength = 5808,
		.framelength = 3206,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 21,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 560000000,
		.linelength = 5808,
		.framelength = 803,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 560000000,
		.linelength = 5808,
		.framelength = 803,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},

	.margin = 5,
	.min_shutter = 4,
	.max_frame_length = 0xFFFF,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 5,

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x5A, 0x20, 0xFF},
	.i2c_speed = 400
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x200,
	.gain = 0x200,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = KAL_FALSE,
	.i2c_write_id = 0
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[] = {
	// preview
	{4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 2104, 1560, 0, 0, 2104, 1560},
	// capture
	{4208, 3120, 0, 0, 4208, 3120, 4208, 3120, 0, 0, 4208, 3120, 0, 0, 4208, 3120},
	// video
	{4208, 3120, 0, 0, 4208, 3120, 4208, 3120, 0, 0, 4208, 3120, 0, 0, 4208, 3120},
	// high speed video
	{4208, 3120, 184, 120, 3840, 2880, 640, 480, 0, 0, 640, 480, 0, 0, 640, 480},
	// slim video
	{4208, 3120, 184, 480, 3840, 2160, 1280, 720, 0, 0, 1280, 720, 0, 0, 1280, 720}
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 28,
	.i4OffsetY = 31,
	.i4PitchX = 64,
	.i4PitchY = 64,
	.i4PairNum = 16,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
	.i4BlockNumX = 65,
	.i4BlockNumY = 48,
	.i4PosR = {{28, 35}, {80, 35}, {44, 39}, {64, 39}, {32, 47}, {76, 47}, {48, 51}, {60, 51}, {48, 67}, {60, 67}, {32, 71}, {76, 71}, {44, 79}, {64, 79}, {28, 83}, {80, 83}},
	.i4PosL = {{28, 31}, {80, 31}, {44, 35}, {64, 35}, {32, 51}, {76, 51}, {48, 55}, {60, 55}, {48, 63}, {60, 63}, {32, 67}, {76, 67}, {44, 83}, {64, 83}, {28, 87}, {80, 87}},
	.iMirrorFlip = 0
};

static struct stCAM_CAL_DATAINFO_STRUCT sensor_eeprom_data = {
	.sensorID = CACTUS_S5K3L8_SUNNY_SENSOR_ID,
	.deviceID = 1,
	.dataLength = 0x1BF6,
	.sensorVendorId = 0x12E1809,
	.vendorByte = {1, 8, 9, 10},
	.dataBuffer = NULL
};

static struct stCAM_CAL_CHECKSUM_STRUCT cam_cal_checksum[] = {
	{MODULE_ITEM, 0x0000, 0x0001, 0x0023, 0x0024, 0x01},
	{SEGMENT_ITEM, 0x0025, 0x0026, 0x003E, 0x003F, 0x01},
	{AF_ITEM, 0x0042, 0x0043, 0x0051, 0x0052, 0x01},
	{AWB_ITEM, 0x0056, 0x0057, 0x0065, 0x0066, 0x01},
	{LSC_ITEM, 0x0082, 0x0083, 0x07D2, 0x07D3, 0x01},
	{PDAF_ITEM, 0x0E62, 0x0E63, 0x13DF, 0x13E0, 0x01},
	{TOTAL_ITEM, 0x0000, 0x0000, 0x1BF4, 0x1BF5, 0x01},
	{MAX_ITEM, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01}
};

#define I2C_BUFFER_LEN 765
static void write_cmos_sensor_table(const kal_uint16 *para, kal_uint32 len) {
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 to_send = 0, i = 0;
	kal_uint16 data;

	while (i < len) {
		data = para[i++];
		puSendCmd[to_send++] = (char) (data >> 8);
		puSendCmd[to_send++] = (char) (data & 0xFF);

		data = para[i++];
		puSendCmd[to_send++] = (char) (data >> 8);
		puSendCmd[to_send++] = (char) (data & 0xFF);

		if ((I2C_BUFFER_LEN - to_send) < 4 || i == len) {
			iBurstWriteReg_multi(puSendCmd, to_send, imgsensor.i2c_write_id, 4, imgsensor_info.i2c_speed);
			to_send = 0;
		}
	}
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr) {
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char) (addr >> 8), (char) (addr & 0xFF)};

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para) {
	char pu_send_cmd[4] = {(char) (addr >> 8), (char) (addr & 0xFF), (char) (para >> 8), (char) (para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para) {
	char pu_send_cmd[3] = {(char) (addr >> 8), (char) (addr & 0xFF), (char) (para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void) {
	write_cmos_sensor(0x340, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x342, imgsensor.line_length & 0xFFFF);
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en) {
	kal_uint32 frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ?frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = (int) (imgsensor.frame_length - imgsensor.min_frame_length);

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = (int) (imgsensor.frame_length - imgsensor.min_frame_length);
	}

	if (min_framelength_en) {
		imgsensor.min_frame_length = imgsensor.frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);

	set_dummy();
}

static void write_shutter(kal_uint16 shutter) {
	kal_uint16 realtime_fps;

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk * 10 / (imgsensor.line_length * imgsensor.frame_length);

		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			write_cmos_sensor(0x340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		write_cmos_sensor(0x340, imgsensor.frame_length & 0xFFFF);
	}

	// update shutter
	write_cmos_sensor(0x202, shutter & 0xFFFF);

	LOG_DBG("shutter: %d, framelength: %d\n", shutter, imgsensor.frame_length);
}

static void set_shutter(kal_uint16 shutter) {
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}

static kal_uint16 gain2reg(const kal_uint16 gain) {
	return gain / 2;
}

static kal_uint16 set_gain(kal_uint16 gain) {
	kal_uint16 reg_gain;

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain
	   [0:3] = N meams N / 16 X
	   [4:9] = M meams M X
	   Total gain = M + N / 16 X   */
	if (gain < BASEGAIN) {
		gain = BASEGAIN;
	} else if (gain > 16 * BASEGAIN) {
		gain = 16 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);

	LOG_DBG("gain: %d, reg_gain: 0x%x\n", gain, reg_gain);

	write_cmos_sensor(0x204, reg_gain & 0xFFFF);

	return gain;
}

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain) {
	LOG_DBG("le: 0x%x, se: 0x%x, gain: 0x%x\n", le, se, gain);

	if (!imgsensor.ihdr_en) {
		return;
	}

	spin_lock(&imgsensor_drv_lock);
	if (le > imgsensor.min_frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = le + imgsensor_info.margin;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);

	if (le < imgsensor_info.min_shutter) {
		le = imgsensor_info.min_shutter;
	}

	if (se < imgsensor_info.min_shutter) {
		se = imgsensor_info.min_shutter;
	}

	// extend frame length first
	write_cmos_sensor(0x380E, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x380F, imgsensor.frame_length & 0xFF);

	write_cmos_sensor(0x3502, (le << 4) & 0xFF);
	write_cmos_sensor(0x3501, (le >> 4) & 0xFF);
	write_cmos_sensor(0x3500, (le >> 12) & 0x0F);

	write_cmos_sensor(0x3512, (se << 4) & 0xFF);
	write_cmos_sensor(0x3511, (se >> 4) & 0xFF);
	write_cmos_sensor(0x3510, (se >> 12) & 0x0F);

	set_gain(gain);
}

static kal_uint16 addr_data_pair_init[] = {
	0x6028, 0x4000,
	0x6214, 0xFFFF,
	0x6216, 0xFFFF,
	0x6218, 0x0000,
	0x621A, 0x0000,
	0x6028, 0x2000,
	0x602A, 0x2450,
	0x6F12, 0x0448,
	0x6F12, 0x0349,
	0x6F12, 0x0160,
	0x6F12, 0xC26A,
	0x6F12, 0x511A,
	0x6F12, 0x8180,
	0x6F12, 0x00F0,
	0x6F12, 0x48B8,
	0x6F12, 0x2000,
	0x6F12, 0x2588,
	0x6F12, 0x2000,
	0x6F12, 0x16C0,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x10B5,
	0x6F12, 0x00F0,
	0x6F12, 0x5DF8,
	0x6F12, 0x2748,
	0x6F12, 0x4078,
	0x6F12, 0x0028,
	0x6F12, 0x0AD0,
	0x6F12, 0x00F0,
	0x6F12, 0x5CF8,
	0x6F12, 0x2549,
	0x6F12, 0xB1F8,
	0x6F12, 0x1403,
	0x6F12, 0x4200,
	0x6F12, 0x2448,
	0x6F12, 0x4282,
	0x6F12, 0x91F8,
	0x6F12, 0x9610,
	0x6F12, 0x4187,
	0x6F12, 0x10BD,
	0x6F12, 0x70B5,
	0x6F12, 0x0446,
	0x6F12, 0x2148,
	0x6F12, 0x0022,
	0x6F12, 0x4068,
	0x6F12, 0x86B2,
	0x6F12, 0x050C,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0x00F0,
	0x6F12, 0x4CF8,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0x4EF8,
	0x6F12, 0x14F8,
	0x6F12, 0x680F,
	0x6F12, 0x6178,
	0x6F12, 0x40EA,
	0x6F12, 0x4100,
	0x6F12, 0x1749,
	0x6F12, 0xC886,
	0x6F12, 0x1848,
	0x6F12, 0x2278,
	0x6F12, 0x007C,
	0x6F12, 0x4240,
	0x6F12, 0x1348,
	0x6F12, 0xA230,
	0x6F12, 0x8378,
	0x6F12, 0x43EA,
	0x6F12, 0xC202,
	0x6F12, 0x0378,
	0x6F12, 0x4078,
	0x6F12, 0x9B00,
	0x6F12, 0x43EA,
	0x6F12, 0x4000,
	0x6F12, 0x0243,
	0x6F12, 0xD0B2,
	0x6F12, 0x0882,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0xBDE8,
	0x6F12, 0x7040,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x2AB8,
	0x6F12, 0x10B5,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x8701,
	0x6F12, 0x0B48,
	0x6F12, 0x00F0,
	0x6F12, 0x2DF8,
	0x6F12, 0x084C,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x6D01,
	0x6F12, 0x2060,
	0x6F12, 0x0848,
	0x6F12, 0x00F0,
	0x6F12, 0x25F8,
	0x6F12, 0x6060,
	0x6F12, 0x10BD,
	0x6F12, 0x0000,
	0x6F12, 0x2000,
	0x6F12, 0x0550,
	0x6F12, 0x2000,
	0x6F12, 0x0C60,
	0x6F12, 0x4000,
	0x6F12, 0xD000,
	0x6F12, 0x2000,
	0x6F12, 0x2580,
	0x6F12, 0x2000,
	0x6F12, 0x16F0,
	0x6F12, 0x0000,
	0x6F12, 0x2221,
	0x6F12, 0x0000,
	0x6F12, 0x2249,
	0x6F12, 0x42F2,
	0x6F12, 0x351C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x42F2,
	0x6F12, 0xE11C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x40F2,
	0x6F12, 0x077C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x42F2,
	0x6F12, 0x492C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x4BF2,
	0x6F12, 0x453C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x30C8,
	0x6F12, 0x0157,
	0x6F12, 0x0000,
	0x6F12, 0x0003,
	0x6028, 0x2000,
	0x602A, 0x1082,
	0x6F12, 0x8010,
	0x6028, 0x4000,
	0x31CE, 0x0001,
	0x0200, 0x00C6,
	0x3734, 0x0010,
	0x3736, 0x0001,
	0x3738, 0x0001,
	0x37CC, 0x0001,
	0x3744, 0x0100,
	0x3762, 0x0105,
	0x3764, 0x0105,
	0x376A, 0x00F0,
	0x344A, 0x000F,
	0x344C, 0x003D,
	0xF460, 0x0030,
	0xF414, 0x24C2,
	0xF416, 0x0183,
	0xF468, 0x4005,
	0x3424, 0x0A07,
	0x3426, 0x0F07,
	0x3428, 0x0F07,
	0x341E, 0x0804,
	0x3420, 0x0C0C,
	0x3422, 0x2D2D,
	0xF462, 0x003A,
	0x3450, 0x0010,
	0x3452, 0x0010,
	0xF446, 0x0020,
	0xF44E, 0x000C,
	0x31FA, 0x0007,
	0x31FC, 0x0161,
	0x31FE, 0x0009,
	0x3200, 0x000C,
	0x3202, 0x007F,
	0x3204, 0x00A2,
	0x3206, 0x007D,
	0x3208, 0x00A4,
	0x3334, 0x00A7,
	0x3336, 0x00A5,
	0x3338, 0x0033,
	0x333A, 0x0006,
	0x333C, 0x009F,
	0x333E, 0x008C,
	0x3340, 0x002D,
	0x3342, 0x000A,
	0x3344, 0x002F,
	0x3346, 0x0008,
	0x3348, 0x009F,
	0x334A, 0x008C,
	0x334C, 0x002D,
	0x334E, 0x000A,
	0x3350, 0x000A,
	0x320A, 0x007B,
	0x320C, 0x0161,
	0x320E, 0x007F,
	0x3210, 0x015F,
	0x3212, 0x007B,
	0x3214, 0x00B0,
	0x3216, 0x0009,
	0x3218, 0x0038,
	0x321A, 0x0009,
	0x321C, 0x0031,
	0x321E, 0x0009,
	0x3220, 0x0038,
	0x3222, 0x0009,
	0x3224, 0x007B,
	0x3226, 0x0001,
	0x3228, 0x0010,
	0x322A, 0x00A2,
	0x322C, 0x00B1,
	0x322E, 0x0002,
	0x3230, 0x015D,
	0x3232, 0x0001,
	0x3234, 0x015D,
	0x3236, 0x0001,
	0x3238, 0x000B,
	0x323A, 0x0016,
	0x323C, 0x000D,
	0x323E, 0x001C,
	0x3240, 0x000D,
	0x3242, 0x0054,
	0x3244, 0x007B,
	0x3246, 0x00CC,
	0x3248, 0x015D,
	0x324A, 0x007E,
	0x324C, 0x0095,
	0x324E, 0x0085,
	0x3250, 0x009D,
	0x3252, 0x008D,
	0x3254, 0x009D,
	0x3256, 0x007E,
	0x3258, 0x0080,
	0x325A, 0x0001,
	0x325C, 0x0005,
	0x325E, 0x0085,
	0x3260, 0x009D,
	0x3262, 0x0001,
	0x3264, 0x0005,
	0x3266, 0x007E,
	0x3268, 0x0080,
	0x326A, 0x0053,
	0x326C, 0x007D,
	0x326E, 0x00CB,
	0x3270, 0x015E,
	0x3272, 0x0001,
	0x3274, 0x0005,
	0x3276, 0x0009,
	0x3278, 0x000C,
	0x327A, 0x007E,
	0x327C, 0x0098,
	0x327E, 0x0009,
	0x3280, 0x000C,
	0x3282, 0x007E,
	0x3284, 0x0080,
	0x3286, 0x0044,
	0x3288, 0x0163,
	0x328A, 0x0045,
	0x328C, 0x0047,
	0x328E, 0x007D,
	0x3290, 0x0080,
	0x3292, 0x015F,
	0x3294, 0x0162,
	0x3296, 0x007D,
	0x3298, 0x0000,
	0x329A, 0x0000,
	0x329C, 0x0000,
	0x329E, 0x0000,
	0x32A0, 0x0008,
	0x32A2, 0x0010,
	0x32A4, 0x0018,
	0x32A6, 0x0020,
	0x32A8, 0x0000,
	0x32AA, 0x0008,
	0x32AC, 0x0010,
	0x32AE, 0x0018,
	0x32B0, 0x0020,
	0x32B2, 0x0020,
	0x32B4, 0x0020,
	0x32B6, 0x0020,
	0x32B8, 0x0000,
	0x32BA, 0x0000,
	0x32BC, 0x0000,
	0x32BE, 0x0000,
	0x32C0, 0x0000,
	0x32C2, 0x0000,
	0x32C4, 0x0000,
	0x32C6, 0x0000,
	0x32C8, 0x0000,
	0x32CA, 0x0000,
	0x32CC, 0x0000,
	0x32CE, 0x0000,
	0x32D0, 0x0000,
	0x32D2, 0x0000,
	0x32D4, 0x0000,
	0x32D6, 0x0000,
	0x32D8, 0x0000,
	0x32DA, 0x0000,
	0x32DC, 0x0000,
	0x32DE, 0x0000,
	0x32E0, 0x0000,
	0x32E2, 0x0000,
	0x32E4, 0x0000,
	0x32E6, 0x0000,
	0x32E8, 0x0000,
	0x32EA, 0x0000,
	0x32EC, 0x0000,
	0x32EE, 0x0000,
	0x32F0, 0x0000,
	0x32F2, 0x0000,
	0x32F4, 0x000A,
	0x32F6, 0x0002,
	0x32F8, 0x0008,
	0x32FA, 0x0010,
	0x32FC, 0x0020,
	0x32FE, 0x0028,
	0x3300, 0x0038,
	0x3302, 0x0040,
	0x3304, 0x0050,
	0x3306, 0x0058,
	0x3308, 0x0068,
	0x330A, 0x0070,
	0x330C, 0x0080,
	0x330E, 0x0088,
	0x3310, 0x0098,
	0x3312, 0x00A0,
	0x3314, 0x00B0,
	0x3316, 0x00B8,
	0x3318, 0x00C8,
	0x331A, 0x00D0,
	0x331C, 0x00E0,
	0x331E, 0x00E8,
	0x3320, 0x0017,
	0x3322, 0x002F,
	0x3324, 0x0047,
	0x3326, 0x005F,
	0x3328, 0x0077,
	0x332A, 0x008F,
	0x332C, 0x00A7,
	0x332E, 0x00BF,
	0x3330, 0x00D7,
	0x3332, 0x00EF,
	0x3352, 0x00A5,
	0x3354, 0x00AF,
	0x3356, 0x0187,
	0x3358, 0x0000,
	0x335A, 0x009E,
	0x335C, 0x016B,
	0x335E, 0x0015,
	0x3360, 0x00A5,
	0x3362, 0x00AF,
	0x3364, 0x01FB,
	0x3366, 0x0000,
	0x3368, 0x009E,
	0x336A, 0x016B,
	0x336C, 0x0015,
	0x336E, 0x00A5,
	0x3370, 0x00A6,
	0x3372, 0x0187,
	0x3374, 0x0000,
	0x3376, 0x009E,
	0x3378, 0x016B,
	0x337A, 0x0015,
	0x337C, 0x00A5,
	0x337E, 0x00A6,
	0x3380, 0x01FB,
	0x3382, 0x0000,
	0x3384, 0x009E,
	0x3386, 0x016B,
	0x3388, 0x0015,
	0x319A, 0x0005,
	0x1006, 0x0005,
	0x3416, 0x0001,
	0x308C, 0x0008,
	0x307C, 0x0240,
	0x375E, 0x0050,
	0x31CE, 0x0101,
	0x374E, 0x0007,
	0x3460, 0x0001,
	0x3052, 0x0002,
	0x3058, 0x0100,
	0x6028, 0x2000,
	0x602A, 0x108A,
	0x6F12, 0x0359,
	0x6F12, 0x0100,
	0x6028, 0x4000,
	0x1124, 0x4100,
	0x1126, 0x0000,
	0x112C, 0x4100,
	0x112E, 0x0000,
	0x3442, 0x0100
};

static void sensor_init(void) {
	write_cmos_sensor_table(addr_data_pair_init, sizeof(addr_data_pair_init) / sizeof(kal_uint16));
	write_cmos_sensor_8(0xB0A0, 0x7D); // continuous mode
}

static kal_uint16 addr_data_pair_preview[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x0838,
	0x034E, 0x0618,
	0x0900, 0x0112,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0003,
	0x0400, 0x0001,
	0x0404, 0x0020,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x00AF,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x0119,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x0C86,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static void preview_setting(void) {
	write_cmos_sensor_table(addr_data_pair_preview, sizeof(addr_data_pair_preview) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_capture_15fps[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0011,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0400, 0x0000,
	0x0404, 0x0010,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x007D,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x00c8,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x11ED,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static kal_uint16 addr_data_pair_capture_24fps[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0011,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0400, 0x0000,
	0x0404, 0x0010,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x008C,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x0119,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x0C86,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static kal_uint16 addr_data_pair_capture_30fps[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0011,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0400, 0x0000,
	0x0404, 0x0010,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x00AF,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x0119,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x0C86,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static kal_uint16 addr_data_pair_capture_fps[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0011,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0400, 0x0000,
	0x0404, 0x0010,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x007D,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x00c8,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x11ED,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static void capture_setting(kal_uint16 current_fps) {
	if (current_fps == 150) {
		write_cmos_sensor_table(addr_data_pair_capture_15fps, sizeof(addr_data_pair_capture_15fps) / sizeof(kal_uint16));
	} else if (current_fps == 240) {
		write_cmos_sensor_table(addr_data_pair_capture_24fps, sizeof(addr_data_pair_capture_24fps) / sizeof(kal_uint16));
	} else if (current_fps == 300) {
		write_cmos_sensor_table(addr_data_pair_capture_30fps, sizeof(addr_data_pair_capture_30fps) / sizeof(kal_uint16));
	} else {
		write_cmos_sensor_table(addr_data_pair_capture_fps, sizeof(addr_data_pair_capture_fps) / sizeof(kal_uint16));
	}
}

static void normal_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_capture_30fps, sizeof(addr_data_pair_capture_30fps) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_hs_video[] = {
	0x6028, 0x2000,
	0x602A, 0x0F74,
	0x6F12, 0x0040,
	0x6F12, 0x0040,
	0x6028, 0x4000,
	0x0344, 0x00C0,
	0x0346, 0x0080,
	0x0348, 0x0FBF,
	0x034A, 0x0BBF,
	0x034C, 0x0280,
	0x034E, 0x01E0,
	0x0900, 0x0116,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x000B,
	0x0400, 0x0001,
	0x0404, 0x0060,
	0x0114, 0x0300,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0006,
	0x0306, 0x00AF,
	0x0302, 0x0001,
	0x0300, 0x0005,
	0x030C, 0x0006,
	0x030E, 0x0119,
	0x030A, 0x0001,
	0x0308, 0x0008,
	0x0342, 0x16B0,
	0x0340, 0x0323,
	0x0202, 0x0200,
	0x0200, 0x00C6,
	0x0B04, 0x0101,
	0x0B08, 0x0000,
	0x0B00, 0x0007,
	0x316A, 0x00A0
};

static void hs_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_hs_video, sizeof(addr_data_pair_hs_video) / sizeof(kal_uint16));
}

static void slim_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_hs_video, sizeof(addr_data_pair_hs_video) / sizeof(kal_uint16));
}

static kal_uint32 return_sensor_id(void) {
	return (read_cmos_sensor(0x0) << 8) | read_cmos_sensor(0x1);
}

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id) {
	kal_uint8 i, retry;
	int size;

	for (i = 0; imgsensor_info.i2c_addr_table[i] != 0xFF; i++) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);

		size = imgSensorReadEepromData(&sensor_eeprom_data, cam_cal_checksum, 6);
		if (size != sensor_eeprom_data.dataLength) {
			LOG_ERR("get eeprom data failed size: %d, dataLength: %d\n", size, sensor_eeprom_data.dataLength);

			*sensor_id = 0xFFFFFFFF;
			if (sensor_eeprom_data.dataBuffer != NULL) {
				kfree(sensor_eeprom_data.dataBuffer);
				sensor_eeprom_data.dataBuffer = NULL;
			}

			continue;
		} else {
			LOG_INF("get eeprom data success\n");
		}

		for (retry = 2; retry > 0; retry--) {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_DBG("i2c_write_id: 0x%x, sensor_id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				imgSensorSetEepromData(&sensor_eeprom_data);

				return ERROR_NONE;
			}

			LOG_ERR("read sensor id failed i2c_write_id: 0x%x, sensor_id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
		}
	}

	if (*sensor_id != imgsensor_info.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

static kal_uint32 open(void) {
	kal_uint16 sensor_id = 0;
	kal_uint8 i, retry;

	for (i = 0; imgsensor_info.i2c_addr_table[i] != 0xFF; i++) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);

		for (retry = 2; retry > 0; retry--) {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_DBG("i2c_write_id: 0x%x, sensor_id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
		}

		if (sensor_id == imgsensor_info.sensor_id) {
			break;
		}
	}

	if (imgsensor_info.sensor_id != sensor_id) {
		LOG_ERR("open failed sensor_id: 0x%x\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	sensor_init();

	spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 close(void) {
	return ERROR_NONE;
}

static kal_uint32 preview(void) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();
	write_cmos_sensor(0x101, 0x0);

	return ERROR_NONE;
}

static kal_uint32 capture(void) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.autoflicker_en = KAL_FALSE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
	} else {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	write_cmos_sensor(0x101, 0x0);

	return ERROR_NONE;
}

static kal_uint32 normal_video(void) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	normal_video_setting();
	write_cmos_sensor(0x101, 0x0);

	return ERROR_NONE;
}

static kal_uint32 hs_video(void) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	hs_video_setting();
	write_cmos_sensor(0x101, 0x0);

	return ERROR_NONE;
}

static kal_uint32 slim_video(void) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	slim_video_setting();
	write_cmos_sensor(0x101, 0x0);

	return ERROR_NONE;
}

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution) {
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;

	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_INFO_STRUCT *sensor_info, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data) {
	LOG_DBG("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;
	sensor_info->SensorResetActiveHigh = FALSE;
	sensor_info->SensorResetDelayCount = 5;

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = PDAF_SUPPORT_RAW;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;
	sensor_info->SensorPixelClockCount = 3;
	sensor_info->SensorDataLatchCount = 2;

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0; // 0 is default 1x
	sensor_info->SensorHightSampling = 0; // 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;
			break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data) {
	LOG_DBG("scenario_id = %d\n", scenario_id);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			capture();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video();
			break;
		default:
			preview();
			return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}

static void set_video_mode(UINT16 framerate) {
	LOG_DBG("framerate: %d\n", framerate);

	if (framerate == 0) {
		return;
	}

	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 296;
	} else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 146;
	} else {
		imgsensor.current_fps = framerate;
	}
	spin_unlock(&imgsensor_drv_lock);

	set_max_framerate(imgsensor.current_fps, 1);
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate) {
	LOG_DBG("enable: %d, framerate: %d\n", enable, framerate);

	spin_lock(&imgsensor_drv_lock);
	if (enable) {
		imgsensor.autoflicker_en = KAL_TRUE;
	} else {
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static void set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate) {
	kal_uint32 frame_length;

	LOG_DBG("scenario_id: %d, framerate: %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;

			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (int) (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if (framerate == 0) {
				return;
			}

			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;

			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length >imgsensor_info.normal_video.framelength) ? (int) (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}

			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
				frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;

				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (int) (frame_length - imgsensor_info.cap1.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
				imgsensor.min_frame_length = imgsensor.frame_length;
				spin_unlock(&imgsensor_drv_lock);
			} else {
				if (imgsensor.current_fps != imgsensor_info.cap.max_framerate) {
					LOG_DBG("fps %d not supported, using cap %d fps\n", framerate, imgsensor_info.cap.max_framerate / 10);
				}

				frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;

				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length >imgsensor_info.cap.framelength) ? (int) (frame_length - imgsensor_info.cap.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
				imgsensor.min_frame_length = imgsensor.frame_length;
				spin_unlock(&imgsensor_drv_lock);
			}

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;

			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length >imgsensor_info.hs_video.framelength) ? (int) (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;

			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (int) (frame_length - imgsensor_info.slim_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}

			break;
		default:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;

			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (int) (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			LOG_ERR("scenario_id: %d, we use preview scenario\n", scenario_id);

			break;
	}
}

static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate) {
	LOG_DBG("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable) {
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1082);

	if (enable) {
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x3734, 0x0001);
		write_cmos_sensor(0x0600, 0x0308);
	} else {
		write_cmos_sensor(0x6F12, 0x8010);
		write_cmos_sensor(0x3734, 0x0010);
		write_cmos_sensor(0x0600, 0x0300);
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static void wait_for_stream(void) {
	unsigned int i, timeout = (10000 / imgsensor.current_fps) + 1;

	mdelay(3);

	for (i = 0; i < timeout; i++) {
		if (read_cmos_sensor(0x5) != 0xFF) {
			mdelay(1);
		} else {
			break;
		}
	}
}

static kal_uint32 streaming_control(kal_bool enable) {
	LOG_DBG("%d\n", enable);

	if (enable) {
		write_cmos_sensor(0x100, 0x100);
	} else {
		write_cmos_sensor(0x100, 0x0);
		wait_for_stream();
	}

	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id, UINT8 *feature_para, UINT32 *feature_para_len) {
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_DBG("feature_id: %d\n", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:
			set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL) *feature_data_16, *(feature_data_16 + 1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *feature_data, *(feature_data + 1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *(feature_data), (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			LOG_DBG("current fps: %d\n", *feature_data_16);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_16;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			LOG_DBG("ihdr enable: %d\n", *feature_data);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_en = (BOOL) *feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
			LOG_DBG("SENSOR_FEATURE_GET_CROP_INFO scenarioId: %d\n", (UINT32) * feature_data);

			wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *) wininfo, (void *) &imgsensor_winsize_info[1], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *) wininfo, (void *) &imgsensor_winsize_info[2], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *) wininfo, (void *) &imgsensor_winsize_info[3], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *) wininfo, (void *) &imgsensor_winsize_info[4], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *) wininfo, (void *) &imgsensor_winsize_info[0], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
			break;
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_DBG("SENSOR_SET_SENSOR_IHDR LE: %d, SE: %d, Gain: %d\n", (UINT16) *feature_data, (UINT16) *(feature_data + 1), (UINT16) *(feature_data + 2));
			ihdr_write_shutter_gain((UINT16) *feature_data, (UINT16) *(feature_data + 1), (UINT16) *(feature_data + 2));
			break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			PDAFinfo = (struct SET_PD_BLOCK_INFO_T *) (uintptr_t)(*(feature_data+1));

			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info, sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					break;
			}

			break;
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
					break;
			}

			break;
		case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
			*feature_return_para_i32 = 0;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
			streaming_control(KAL_FALSE);
			break;
		case SENSOR_FEATURE_SET_STREAMING_RESUME:
			if (*feature_data != 0) {
				set_shutter(*feature_data);
			}
			streaming_control(KAL_TRUE);
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 CACTUS_S5K3L8_SUNNY_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc) {
	if (pfFunc != NULL) {
		*pfFunc = &sensor_func;
	}

	return ERROR_NONE;
}
