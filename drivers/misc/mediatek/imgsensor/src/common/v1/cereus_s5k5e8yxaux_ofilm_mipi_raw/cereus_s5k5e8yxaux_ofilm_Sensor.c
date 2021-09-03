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

#include "cereus_s5k5e8yxaux_ofilm_Sensor.h"
#include "cam_cal_define.h"

#define PFX "cereus_s5k5e8yxaux_ofilm_camera_sensor"
#define LOG_DBG(format, args...) pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = CEREUS_S5K5E8YXAUX_OFILM_SENSOR_ID,
	.checksum_value = 0x2AE69154,

	.pre = {
		.pclk = 168000000,
		.linelength = 5200,
		.framelength = 1062,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1296,
		.grabwindow_height = 972,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 168000000,
		.linelength = 2856,
		.framelength = 1968,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 84000000,
		.linelength = 2856,
		.framelength = 1967,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,
	},
	.cap2 = {
		.pclk = 137000000,
		.linelength = 2856,
		.framelength = 1967,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
	},
	.normal_video = {
		.pclk = 168000000,
		.linelength = 2856,
		.framelength = 1968,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 168000000,
		.linelength = 2776,
		.framelength = 508,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 168000000,
		.linelength = 5248,
		.framelength = 1066,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},

	.margin = 6,
	.min_shutter = 2,
	.max_frame_length = 0xFFFF,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 1,
	.ae_ispGain_delay_frame = 2,
	.frame_time_delay_frame = 1,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 7,

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.i2c_addr_table = {0x5A, 0x20, 0xFF},
	.i2c_speed = 400
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 0x100,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = KAL_FALSE,
	.i2c_write_id = 0x20
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[] = {
	// preview
	{2592, 1944, 0, 0, 2592, 1944, 1296, 972, 0, 0, 1296, 972, 0, 0, 1296, 972},
	// capture
	{2592, 1944, 0, 0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 0, 2592, 1944},
	// video
	{2592, 1944, 0, 0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 0, 2592, 1944},
	// high speed video
	{2592, 1944, 16, 252, 2560, 1440, 640, 480, 0, 0, 640, 480, 0, 0, 640, 480},
	// slim video
	{2592, 1944, 16, 12, 2560, 1920, 1280, 720, 0, 0, 1280, 720, 0, 0, 1280, 720}
};

static struct stCAM_CAL_DATAINFO_STRUCT sensor_eeprom_data = {
	.sensorID = CEREUS_S5K5E8YXAUX_OFILM_SENSOR_ID,
	.deviceID = 4,
	.dataLength = 0x182,
	.sensorVendorId = 0x72AFFFF,
	.vendorByte = {1, 8, 9, 10},
	.dataBuffer = NULL
};

static struct stCAM_CAL_CHECKSUM_STRUCT cam_cal_checksum[] = {
	{MODULE_ITEM, 0x0000, 0x0000, 0x0017, 0x0018, 0xFF},
	{LSC_ITEM, 0x0019, 0x0019, 0x0180, 0x0181, 0xFF},
	{MAX_ITEM, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFF}
};

#define I2C_BUFFER_LEN 225
static void write_cmos_sensor_table(const kal_uint16 *para, kal_uint32 len) {
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 to_send = 0, i = 0;
	kal_uint16 data;

	while (i < len) {
		data = para[i++];
		puSendCmd[to_send++] = (char) (data >> 8);
		puSendCmd[to_send++] = (char) (data & 0xFF);

		data = para[i++];
		puSendCmd[to_send++] = (char) (data & 0xFF);

		if ((I2C_BUFFER_LEN - to_send) < 3 || i == len) {
			iBurstWriteReg_multi(puSendCmd, to_send, imgsensor.i2c_write_id, 3, imgsensor_info.i2c_speed);
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

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para) {
	char pu_send_cmd[3] = {(char) (addr >> 8), (char) (addr & 0xFF), (char) (para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void read_efuse_id(struct stCAM_CAL_DATAINFO_STRUCT *pData) {
	u8 buf[8] = {0};
	unsigned int i;

	write_cmos_sensor_8(0x0100, 0x00);
	mdelay(10);
	write_cmos_sensor_8(0x0a00, 0x04);
	write_cmos_sensor_8(0x0a02, 0x00);
	write_cmos_sensor_8(0x0a00, 0x01);
	mdelay(100);

	for (i = 0; i < 8; i++) {
		buf[i] = read_cmos_sensor(0xA04 + i);
		LOG_DBG("read_efuse_id[%d] = 0x%2x\n", i, buf[i]);
	}

	write_cmos_sensor_8(0x0a00, 0x04);
	write_cmos_sensor_8(0x0a00, 0x00);
	mdelay(10);
	write_cmos_sensor_8(0x0100, 0x01);

	imgSensorSetDataEfuseID(buf, pData->deviceID, 8);
}

static int get_otp_info(struct stCAM_CAL_DATAINFO_STRUCT *pData) {
	u8 stream_flag, pages;
	u8 vendor_id;

	int i, buf_id = 0;

	if (pData == NULL) {
		LOG_ERR("pData is NULL\n");
		return -1;
	}

	if (pData->dataBuffer == NULL) {
		pData->dataBuffer  = kmalloc(pData->dataLength, GFP_KERNEL);

		if (pData->dataBuffer == NULL) {
			LOG_ERR("dataBuffer malloc failed\n");
			return -1;
		}
	}

	stream_flag = read_cmos_sensor(0x100);
	if (stream_flag == 0x1) {
		write_cmos_sensor_8(0x100, 0x0);
	}

	write_cmos_sensor_8(0xA00, 0x4);
	write_cmos_sensor_8(0xA02, 0xE);
	write_cmos_sensor_8(0xA00, 0x1);

	if (read_cmos_sensor(0xA38) == 0x55) {
		vendor_id = read_cmos_sensor(0xA3A);
		if ((pData->sensorVendorId >> 24) != vendor_id) {
			LOG_ERR("group(%d) failed: 0x%x != 0x%x\n", 3, vendor_id, pData->sensorVendorId >> 24);
			return -1;
		}

		for (i = 0; i < 11 && buf_id < pData->dataLength; i++, buf_id++) {
			pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA39 + i);
		}

		write_cmos_sensor_8(0xA00, 0x4);
		write_cmos_sensor_8(0xA02, 0xF);
		write_cmos_sensor_8(0xA00, 0x1);

		for (i = 0; i < 14 && buf_id < pData->dataLength; i++, buf_id++) {
			pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA04 + i);
		}
	} else if(read_cmos_sensor(0xA1E) == 0x55) {
		vendor_id = read_cmos_sensor(0xA20);
		if ((pData->sensorVendorId >> 24) != vendor_id) {
			LOG_ERR("group(%d) failed: 0x%x != 0x%x\n", 2, vendor_id, pData->sensorVendorId >> 24);
			return -1;
		}

		for (i = 0; i < 25 && buf_id < pData->dataLength; i++, buf_id++) {
			pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA1F + i);
		}
	} else if (read_cmos_sensor(0xA04) == 0x55) {
		vendor_id = read_cmos_sensor(0xA06);
		if ((pData->sensorVendorId >> 24) != vendor_id) {
			LOG_ERR("group(%d) failed: 0x%x != 0x%x\n", 1, vendor_id, pData->sensorVendorId >> 24);
			return -1;
		}

		for (i = 0; i < 25 && buf_id < pData->dataLength; i++, buf_id++) {
			pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA05 + i);
		}
	} else {
		LOG_ERR("unable to find otp data\n");

		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;

		return -1;
	}

	write_cmos_sensor_8(0xA00, 0x4);
	write_cmos_sensor_8(0xA02, 0x4);
	write_cmos_sensor_8(0xA00, 0x1);

	for (i = 0; i < 16 && buf_id < pData->dataLength; i++, buf_id++) {
		pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA34 + i);
	}

	for (pages = 5; pages < 10; pages++){
		write_cmos_sensor_8(0xA00, 0x4);
		write_cmos_sensor_8(0xA02, pages);
		write_cmos_sensor_8(0xA00, 0x1);

		for (i = 0; i < 64 && buf_id < pData->dataLength; i++, buf_id++) {
			pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA04 + i);
		}
	}

	write_cmos_sensor_8(0xA00, 0x4);
	write_cmos_sensor_8(0xA02, 0xA);
	write_cmos_sensor_8(0xA00, 0x1);

	for (i = 0; i < 24 && buf_id < pData->dataLength; i++, buf_id++) {
		pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA04 + i);
	}

	write_cmos_sensor_8(0xA00, 0x4);
	write_cmos_sensor_8(0xA02, 0xF);
	write_cmos_sensor_8(0xA00, 0x1);

	pData->dataBuffer[buf_id] = (kal_uint8) read_cmos_sensor(0xA12);
	buf_id++;

	write_cmos_sensor_8(0x100, stream_flag);
	write_cmos_sensor_8(0xA00, 0x4);
	write_cmos_sensor_8(0xA00, 0x0);

	if (imgSensorCheckEepromData(pData, cam_cal_checksum) != 0) {
		LOG_ERR("OTP checksum failed\n");

		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;

		return -1;
	}

	read_efuse_id(pData);
	LOG_DBG("buf_id: %d\n", buf_id);

	return buf_id;
}

static void set_dummy(void) {
	write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x342, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x343, imgsensor.line_length & 0xFF);
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

	// frame_length and shutter should be even numbers.
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk * 10 / (imgsensor.line_length * imgsensor.frame_length);

		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
		}
	} else {
		write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
	}

	// update shutter
	write_cmos_sensor_8(0x202, shutter >> 8);
	write_cmos_sensor_8(0x203, shutter & 0xFF);

	LOG_DBG("shutter: %d, framelength: %d\n", shutter, imgsensor.frame_length);
}

static void set_shutter(kal_uint16 shutter) {
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length) {
	kal_uint16 realtime_fps;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.shutter = shutter;
	if (frame_length > 1) {
		imgsensor.frame_length = frame_length;
	}

	if (shutter > imgsensor.frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
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
			write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
		}
	} else {
		write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
	}

	// update shutter
	write_cmos_sensor_8(0x202, shutter >> 8);
	write_cmos_sensor_8(0x203, shutter & 0xFF);

	LOG_DBG("shutter: %d, framelength: %d\n", shutter, imgsensor.frame_length);
}

static kal_uint16 set_gain(kal_uint16 gain) {
	kal_uint16 reg_gain = gain >> 1;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);

	LOG_DBG("gain: %d, reg_gain: 0x%x\n", gain, reg_gain);

	write_cmos_sensor_8(0x204, reg_gain >> 8);
	write_cmos_sensor_8(0x205, reg_gain & 0xFF);

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
	write_cmos_sensor_8(0x380E, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x380F, imgsensor.frame_length & 0xFF);

	write_cmos_sensor_8(0x3502, (le << 4) & 0xFF);
	write_cmos_sensor_8(0x3501, (le >> 4) & 0xFF);
	write_cmos_sensor_8(0x3500, (le >> 12) & 0x0F);

	write_cmos_sensor_8(0x3508, (se << 4) & 0xFF);
	write_cmos_sensor_8(0x3507, (se >> 4) & 0xFF);
	write_cmos_sensor_8(0x3506, (se >> 12) & 0x0F);

	set_gain(gain);
}

static kal_uint16 addr_data_pair_init[] = {
	0x0100, 0x00,
	0x3906, 0x7E,
	0x3C01, 0x0F,
	0x3C14, 0x00,
	0x3235, 0x08,
	0x3063, 0x2E,
	0x307A, 0x10,
	0x307B, 0x0E,
	0x3079, 0x20,
	0x3070, 0x05,
	0x3067, 0x06,
	0x3071, 0x62,
	0x3203, 0x43,
	0x3205, 0x43,
	0x320b, 0x42,
	0x323b, 0x02,
	0x3007, 0x00,
	0x3008, 0x14,
	0x3020, 0x58,
	0x300D, 0x34,
	0x300E, 0x17,
	0x3021, 0x02,
	0x3010, 0x59,
	0x3002, 0x01,
	0x3005, 0x01,
	0x3008, 0x04,
	0x300F, 0x70,
	0x3010, 0x69,
	0x3017, 0x10,
	0x3019, 0x19,
	0x300C, 0x62,
	0x3064, 0x10,
	0x3C08, 0x0E,
	0x3C09, 0x10,
	0x3C31, 0x0D,
	0x3C32, 0xAC,
	0x3929, 0x07,
	0x3303, 0x02
};

static void sensor_init(void) {
	write_cmos_sensor_table(addr_data_pair_init, sizeof(addr_data_pair_init) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_preview[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0xA9,
	0x0308, 0x34,
	0x0309, 0x82,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x47,
	0x3C1D, 0x15,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x03,
	0x0821, 0x44,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x08,
	0x0346, 0x00,
	0x0347, 0x08,
	0x0348, 0x0A,
	0x0349, 0x27,
	0x034A, 0x07,
	0x034B, 0x9F,
	0x034C, 0x05,
	0x034D, 0x10,
	0x034E, 0x03,
	0x034F, 0xCC,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x03,
	0x0340, 0x04,
	0x0341, 0x26,
	0x0342, 0x14,
	0x0343, 0x50,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x03,
	0x0203, 0xDE,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x08,
	0x3308, 0x0A,
	0x3309, 0x27,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x08,
	0x3310, 0x07,
	0x3311, 0x9F,
	0x3312, 0x03,
	0x3313, 0x01
};

static void preview_setting(void) {
	write_cmos_sensor_table(addr_data_pair_preview, sizeof(addr_data_pair_preview) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_capture_15fps[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0xA8,
	0x0308, 0x19,
	0x0309, 0x02,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x87,
	0x3C1D, 0x25,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x01,
	0x0821, 0x90,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x08,
	0x0346, 0x00,
	0x0347, 0x08,
	0x0348, 0x0A,
	0x0349, 0x27,
	0x034A, 0x07,
	0x034B, 0x9F,
	0x034C, 0x0A,
	0x034D, 0x20,
	0x034E, 0x07,
	0x034F, 0x98,
	0x0900, 0x00,
	0x0901, 0x00,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x01,
	0x0340, 0x07,
	0x0341, 0xAF,
	0x0342, 0x0B,
	0x0343, 0x28,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x03,
	0x0203, 0xDE,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x08,
	0x3308, 0x0A,
	0x3309, 0x27,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x08,
	0x3310, 0x07,
	0x3311, 0x9F,
	0x3312, 0x01,
	0x3313, 0x01
};

static kal_uint16 addr_data_pair_capture_24fps[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0x89,
	0x0308, 0x32,
	0x0309, 0x02,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x47,
	0x3C1D, 0x15,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x03,
	0x0821, 0x44,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x08,
	0x0346, 0x00,
	0x0347, 0x08,
	0x0348, 0x0A,
	0x0349, 0x27,
	0x034A, 0x07,
	0x034B, 0x9F,
	0x034C, 0x0A,
	0x034D, 0x20,
	0x034E, 0x07,
	0x034F, 0x98,
	0x0900, 0x00,
	0x0901, 0x00,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x01,
	0x0340, 0x07,
	0x0341, 0xAF,
	0x0342, 0x0B,
	0x0343, 0x28,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x03,
	0x0203, 0xDE,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x08,
	0x3308, 0x0A,
	0x3309, 0x27,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x08,
	0x3310, 0x07,
	0x3311, 0x9F,
	0x3312, 0x01,
	0x3313, 0x01
};

static kal_uint16 addr_data_pair_capture_30fps[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0xA9,
	0x0308, 0x34,
	0x0309, 0x82,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x47,
	0x3C1D, 0x15,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x03,
	0x0821, 0x44,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x08,
	0x0346, 0x00,
	0x0347, 0x08,
	0x0348, 0x0A,
	0x0349, 0x27,
	0x034A, 0x07,
	0x034B, 0x9F,
	0x034C, 0x0A,
	0x034D, 0x20,
	0x034E, 0x07,
	0x034F, 0x98,
	0x0900, 0x00,
	0x0901, 0x00,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x01,
	0x0340, 0x07,
	0x0341, 0xB0,
	0x0342, 0x0B,
	0x0343, 0x28,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x03,
	0x0203, 0xDE,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x08,
	0x3308, 0x0A,
	0x3309, 0x27,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x08,
	0x3310, 0x07,
	0x3311, 0x9F,
	0x3312, 0x01,
	0x3313, 0x01
};

static void capture_setting(kal_uint16 current_fps) {
	if (current_fps == 150) {
		write_cmos_sensor_table(addr_data_pair_capture_15fps, sizeof(addr_data_pair_capture_15fps) / sizeof(kal_uint16));
	} else if (current_fps == 240) {
		write_cmos_sensor_table(addr_data_pair_capture_24fps, sizeof(addr_data_pair_capture_24fps) / sizeof(kal_uint16));
	} else {
		write_cmos_sensor_table(addr_data_pair_capture_30fps, sizeof(addr_data_pair_capture_30fps) / sizeof(kal_uint16));
	}
}

static void normal_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_capture_30fps, sizeof(addr_data_pair_capture_30fps) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_hs_video[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0xA9,
	0x0308, 0x34,
	0x0309, 0x82,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x47,
	0x3C1D, 0x15,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x03,
	0x0821, 0x44,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x09,
	0x0349, 0xFF,
	0x034A, 0x07,
	0x034B, 0x7F,
	0x034C, 0x02,
	0x034D, 0x80,
	0x034E, 0x01,
	0x034F, 0xE0,
	0x0900, 0x01,
	0x0901, 0x42,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x07,
	0x0340, 0x01,
	0x0341, 0xFC,
	0x0342, 0x0A,
	0x0343, 0xD8,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x01,
	0x0203, 0xEC,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x00,
	0x3308, 0x09,
	0x3309, 0xFF,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x00,
	0x3310, 0x07,
	0x3311, 0x7F,
	0x3312, 0x07,
	0x3313, 0x01
};

static void hs_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_hs_video, sizeof(addr_data_pair_hs_video) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_slim_video[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x0305, 0x06,
	0x0306, 0x18,
	0x0307, 0xA9,
	0x0308, 0x34,
	0x0309, 0x82,
	0x3C1F, 0x00,
	0x3C17, 0x00,
	0x3C0B, 0x04,
	0x3C1C, 0x47,
	0x3C1D, 0x15,
	0x3C14, 0x04,
	0x3C16, 0x00,
	0x0820, 0x03,
	0x0821, 0x44,
	0x0114, 0x01,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0A,
	0x0349, 0x17,
	0x034A, 0x06,
	0x034B, 0xA3,
	0x034C, 0x05,
	0x034D, 0x00,
	0x034E, 0x02,
	0x034F, 0xD0,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x03,
	0x0340, 0x04,
	0x0341, 0x2A,
	0x0342, 0x14,
	0x0343, 0x80,
	0x0200, 0x00,
	0x0201, 0x00,
	0x0202, 0x03,
	0x0203, 0xDE,
	0x3303, 0x02,
	0x3400, 0x00,
	0x323b, 0x02,
	0x3301, 0x00,
	0x3321, 0x04,
	0x3306, 0x00,
	0x3307, 0x00,
	0x3308, 0x0A,
	0x3309, 0x17,
	0x330A, 0x01,
	0x330B, 0x01,
	0x330E, 0x00,
	0x330F, 0x00,
	0x3310, 0x06,
	0x3311, 0xA3,
	0x3312, 0x03,
	0x3313, 0x01
};

static void slim_video_setting(void) {
	write_cmos_sensor_table(addr_data_pair_slim_video, sizeof(addr_data_pair_slim_video) / sizeof(kal_uint16));
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

		size = get_otp_info(&sensor_eeprom_data);
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
			*sensor_id = return_sensor_id() + 5;
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
			sensor_id = return_sensor_id() + 5;
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
	write_cmos_sensor_8(0x101, 0x0);

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
	} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
	} else {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	write_cmos_sensor_8(0x101, 0x0);

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
	write_cmos_sensor_8(0x101, 0x0);

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
	write_cmos_sensor_8(0x101, 0x0);

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
	write_cmos_sensor_8(0x101, 0x0);

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

	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;
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
			} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
				frame_length = imgsensor_info.cap2.pclk / framerate * 10 / imgsensor_info.cap2.linelength;

				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length > imgsensor_info.cap2.framelength) ? (int) (frame_length - imgsensor_info.cap2.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap2.framelength + imgsensor.dummy_line;
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
	if (enable) {
		write_cmos_sensor_8(0x601, 0x2);
	} else {
		write_cmos_sensor_8(0x601, 0x0);
	}

	write_cmos_sensor_8(0x3200, 0x0);

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
		write_cmos_sensor_8(0x3C16, 0x00);
		write_cmos_sensor_8(0x3C0D, 0x04);
		write_cmos_sensor_8(0x0100, 0x01);
		write_cmos_sensor_8(0x3C22, 0x00);
		write_cmos_sensor_8(0x3C22, 0x00);
		write_cmos_sensor_8(0x3C0D, 0x00);
	} else {
		write_cmos_sensor_8(0x0100, 0x00);
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
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_en = KAL_FALSE;
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
			LOG_DBG("SENSOR_SET_SENSOR_IHDR LE: %d, SE: %d, Gain: %d\n", (UINT16) * feature_data, (UINT16) * (feature_data + 1), (UINT16) * (feature_data + 2));
			ihdr_write_shutter_gain((UINT16) *feature_data, (UINT16) *(feature_data + 1), (UINT16) *(feature_data + 2));
			break;
		case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
			set_shutter_frame_length((UINT16) *feature_data, (UINT16) *(feature_data + 1));
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

UINT32 CEREUS_S5K5E8YXAUX_OFILM_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc) {
	if (pfFunc != NULL) {
		*pfFunc = &sensor_func;
	}

	return ERROR_NONE;
}
