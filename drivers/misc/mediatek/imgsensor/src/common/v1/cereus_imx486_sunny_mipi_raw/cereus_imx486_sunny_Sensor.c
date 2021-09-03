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

#include "cereus_imx486_sunny_Sensor.h"
#include "cam_cal_define.h"

#define PFX "cereus_imx486_sunny_camera_sensor"
#define LOG_DBG(format, args...) pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = CEREUS_IMX486_SUNNY_SENSOR_ID,
	.checksum_value = 0x38EBE79E,

	.pre = {
		.pclk = 168000000,
		.linelength = 2096,
		.framelength = 2670,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2016,
		.grabwindow_height = 1508,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 386000000,
		.linelength = 4192,
		.framelength = 3068,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 3016,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 194000000,
		.linelength = 4192,
		.framelength = 3068,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 3016,
		.mipi_data_lp2hs_settle_dc = 85,
		 .max_framerate = 150,
	},
	.normal_video = {
		.pclk = 386000000,
		.linelength = 4192,
		.framelength = 3068,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 3016,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 194000000,
		.linelength = 2096,
		.framelength = 770,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 168000000,
		.linelength = 2096,
		.framelength = 2670,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2016,
		.grabwindow_height = 1508,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},

	.margin = 10,
	.min_shutter = 4,
	.max_frame_length = 0xFF00,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.frame_time_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 7,

	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	.mclk = 12,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x34, 0x20, 0xFF},
	.i2c_speed = 400
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x14D,
	.gain = 0x0,
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
	{4032, 3016, 0, 0, 4032, 3016, 2016, 1508, 0, 0, 2016, 1508, 0, 0, 2016, 1508},
	// capture
	{4032, 3016, 0, 0, 4032, 3016, 4032, 3016, 0, 0, 4032, 3016, 0, 0, 4032, 3016},
	// video
	{4032, 3016, 0, 0, 4032, 3016, 4032, 3016, 0, 0, 4032, 3016, 0, 0, 4032, 3016},
	// high speed video
	{4032, 3016, 0, 0, 4032, 3016, 1280, 720, 0, 0, 1280, 720, 0, 0, 1280, 720},
	// slim video
	{4032, 3016, 0, 0, 4032, 3016, 2016, 1508, 0, 0, 2016, 1508, 0, 0, 2016, 1508},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 64,
	.i4OffsetY = 16,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 4,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
	.i4BlockNumX = 122,
	.i4BlockNumY = 93,
	.i4PosL = {{67, 21}, {83, 21}, {75, 41}, {91, 41}},
	.i4PosR = {{67, 25}, {83, 25}, {75, 37}, {91, 37}},
	.iMirrorFlip = 0
};

static struct stCAM_CAL_DATAINFO_STRUCT sensor_eeprom_data = {
	.sensorID = CEREUS_IMX486_SUNNY_SENSOR_ID,
	.deviceID = 1,
	.dataLength = 0x1BF6,
	.sensorVendorId = 0x11B140A,
	.vendorByte = {1, 8, 9, 10},
	.dataBuffer = NULL
};

static struct stCAM_CAL_CHECKSUM_STRUCT cereus_imx486_sunny_checksum[] = {
	{MODULE_ITEM, 0x0000, 0x0001, 0x0023, 0x0024, 0x01},
	{SEGMENT_ITEM, 0x0025, 0x0026, 0x003E, 0x003F, 0x01},
	{AF_ITEM, 0x0042, 0x0043, 0x0051, 0x0052, 0x01},
	{AWB_ITEM, 0x0056, 0x0057, 0x0065, 0x0066, 0x01},
	{LSC_ITEM, 0x0082, 0x0083, 0x07D2, 0x07D3, 0x01},
	{PDAF_ITEM, 0x0E62, 0x0E63, 0x13AF, 0x13B0, 0x01},
	{DUALCAM_ITEM, 0x13B3, 0x13B4, 0x1BB4, 0x1BB5, 0x01},
	{TOTAL_ITEM, 0x0000, 0x0000, 0x1BF4, 0x1BF5, 0x01},
	{MAX_ITEM, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x01}
};

#define I2C_BUFFER_LEN 225
static void cereus_imx486_sunny_table_write_cmos_sensor(const kal_uint16 *para, kal_uint32 len) {
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

static int write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para) {
	char pu_send_cmd[3] = {(char) (addr >> 8), (char) (addr & 0xFF), (char) (para & 0xFF)};

	return iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void) {
	write_cmos_sensor_8(0x104, 0x1);
	write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x104, 0x0);
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
			write_cmos_sensor_8(0x104, 0x1);
			write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor_8(0x104, 0x0);
		}
	} else {
		write_cmos_sensor_8(0x104, 0x1);
		write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x104, 0x0);
	}

	// update shutter
	write_cmos_sensor_8(0x104, 0x1);
	write_cmos_sensor_8(0x350, 0x1);
	write_cmos_sensor_8(0x202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x203, shutter & 0xFF);
	write_cmos_sensor_8(0x104, 0x0);

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

	// frame_length and shutter should be even numbers
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk * 10 / (imgsensor.line_length * imgsensor.frame_length);

		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			write_cmos_sensor_8(0x104, 0x1);
			write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor_8(0x104, 0x0);
		}
	} else {
		write_cmos_sensor_8(0x104, 0x1);
		write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x104, 0x0);
	}

	// update shutter
	write_cmos_sensor_8(0x104, 0x1);
	write_cmos_sensor_8(0x350, 0x0);
	write_cmos_sensor_8(0x202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x203, shutter & 0xFF);
	write_cmos_sensor_8(0x104, 0x0);

	LOG_DBG("shutter: %d, framelength: %d\n", shutter, imgsensor.frame_length);
}

#define MAX_GAIN_INDEX (193)
static kal_uint16 sensorGainMapping[MAX_GAIN_INDEX + 1][2] = {
	{64, 0},
	{66, 16},
	{68, 30},
	{70, 44},
	{72, 57},
	{74, 69},
	{76, 81},
	{78, 92},
	{80, 102},
	{82, 112},
	{84, 122},
	{86, 131},
	{88, 140},
	{90, 148},
	{92, 156},
	{94, 163},
	{96, 171},
	{98, 178},
	{100, 184},
	{102, 191},
	{104, 197},
	{106, 203},
	{108, 209},
	{110, 214},
	{112, 219},
	{114, 225},
	{116, 230},
	{118, 234},
	{120, 239},
	{122, 243},
	{124, 248},
	{126, 252},
	{128, 256},
	{130, 260},
	{132, 264},
	{134, 267},
	{136, 271},
	{138, 275},
	{140, 278},
	{142, 281},
	{144, 284},
	{146, 288},
	{148, 291},
	{150, 294},
	{152, 296},
	{154, 299},
	{156, 302},
	{158, 305},
	{160, 307},
	{162, 310},
	{164, 312},
	{166, 315},
	{168, 317},
	{170, 319},
	{172, 321},
	{174, 324},
	{176, 326},
	{178, 328},
	{180, 330},
	{182, 332},
	{184, 334},
	{186, 336},
	{188, 338},
	{191, 340},
	{192, 341},
	{194, 343},
	{196, 345},
	{199, 347},
	{200, 348},
	{202, 350},
	{204, 351},
	{206, 353},
	{207, 354},
	{210, 356},
	{211, 357},
	{214, 359},
	{216, 360},
	{218, 362},
	{220, 363},
	{221, 364},
	{224, 366},
	{226, 367},
	{228, 368},
	{231, 370},
	{232, 371},
	{234, 372},
	{236, 373},
	{237, 374},
	{239, 375},
	{243, 377},
	{245, 378},
	{246, 379},
	{248, 380},
	{250, 381},
	{252, 382},
	{254, 383},
	{256, 384},
	{258, 385},
	{260, 386},
	{262, 387},
	{264, 388},
	{266, 389},
	{269, 390},
	{271, 391},
	{273, 392},
	{275, 393},
	{278, 394},
	{280, 395},
	{283, 396},
	{285, 397},
	{287, 398},
	{290, 399},
	{293, 400},
	{295, 401},
	{298, 402},
	{301, 403},
	{303, 404},
	{306, 405},
	{309, 406},
	{312, 407},
	{315, 408},
	{318, 409},
	{321, 410},
	{324, 411},
	{328, 412},
	{331, 413},
	{334, 414},
	{338, 415},
	{341, 416},
	{345, 417},
	{349, 418},
	{352, 419},
	{356, 420},
	{360, 421},
	{364, 422},
	{368, 423},
	{372, 424},
	{377, 425},
	{381, 426},
	{386, 427},
	{390, 428},
	{395, 429},
	{400, 430},
	{405, 431},
	{410, 432},
	{415, 433},
	{420, 434},
	{426, 435},
	{431, 436},
	{437, 437},
	{443, 438},
	{449, 439},
	{455, 440},
	{462, 441},
	{468, 442},
	{475, 443},
	{482, 444},
	{489, 445},
	{497, 446},
	{504, 447},
	{512, 448},
	{520, 449},
	{529, 450},
	{537, 451},
	{546, 452},
	{555, 453},
	{565, 454},
	{575, 455},
	{585, 456},
	{596, 457},
	{607, 458},
	{618, 459},
	{630, 460},
	{643, 461},
	{655, 462},
	{669, 463},
	{683, 464},
	{697, 465},
	{712, 466},
	{728, 467},
	{745, 468},
	{762, 469},
	{780, 470},
	{799, 471},
	{819, 472},
	{840, 473},
	{862, 474},
	{886, 475},
	{910, 476},
	{936, 477},
	{964, 478},
	{993, 479},
	{1024, 480},
	{1024, 480}
};

static kal_uint16 gain2reg(const kal_uint16 gain) {
	kal_uint8 i;

	for (i = 0; i < MAX_GAIN_INDEX; i++) {
		if (gain <= sensorGainMapping[i][0]) {
			break;
		}
	}

	if (gain != sensorGainMapping[i][0]) {
		LOG_ERR("gain mapping isn't correct gain: %d, mapping: %d\n", gain, sensorGainMapping[i][0]);
	}

	return sensorGainMapping[i][1];
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

	write_cmos_sensor_8(0x104, 0x1);
	write_cmos_sensor_8(0x204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor_8(0x205, reg_gain & 0xFF);
	write_cmos_sensor_8(0x104, 0x0);

	return gain;
}

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain) {
	kal_uint16 realtime_fps, reg_gain;

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

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			write_cmos_sensor_8(0x104, 0x1);
			write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor_8(0x104, 0x0);
		}
	} else {
		write_cmos_sensor_8(0x104, 0x1);
		write_cmos_sensor_8(0x340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x104, 0x0);
	}

	write_cmos_sensor_8(0x104, 0x1);
	write_cmos_sensor_8(0x350, 0x1);
	write_cmos_sensor_8(0x202, (le >> 8) & 0xFF);
	write_cmos_sensor_8(0x203, le & 0xFF);
	write_cmos_sensor_8(0x224, (se >> 8) & 0xFF);
	write_cmos_sensor_8(0x225, se & 0xFF);

	reg_gain = gain2reg(gain);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);

	write_cmos_sensor_8(0x204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor_8(0x205, reg_gain & 0xFF);
	write_cmos_sensor_8(0x104, 0x0);

	LOG_DBG("le: 0x%x, se: 0x%x, gain: 0x%x, auto_extend: %d\n", le, se, gain, read_cmos_sensor(0x350));
}

kal_uint16 addr_data_pair_init_cereus_imx486_sunny[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x4979, 0x0d,
	0x3040, 0x03,
	0x3041, 0x04,
	0x4340, 0x01,
	0x4341, 0x02,
	0x4342, 0x04,
	0x4344, 0x00,
	0x4345, 0x2C,
	0x4346, 0x00,
	0x4347, 0x26,
	0x434C, 0x00,
	0x434D, 0x31,
	0x434E, 0x00,
	0x434F, 0x2C,
	0x4350, 0x00,
	0x4351, 0x0A,
	0x4354, 0x00,
	0x4355, 0x2A,
	0x4356, 0x00,
	0x4357, 0x1F,
	0x435C, 0x00,
	0x435D, 0x2F,
	0x435E, 0x00,
	0x435F, 0x2A,
	0x4360, 0x00,
	0x4361, 0x0A,
	0x4600, 0x2A,
	0x4601, 0x30,
	0x4920, 0x0A,
	0x4926, 0x01,
	0x4929, 0x01,
	0x4938, 0x01,
	0x4959, 0x15,
	0x4A38, 0x0A,
	0x4A39, 0x05,
	0x4A3C, 0x0F,
	0x4A3D, 0x05,
	0x4A3E, 0x05,
	0x4A40, 0x0F,
	0x4A4A, 0x01,
	0x4A4B, 0x01,
	0x4A4C, 0x01,
	0x4A4D, 0x01,
	0x4A4E, 0x01,
	0x4A4F, 0x01,
	0x4A50, 0x01,
	0x4A51, 0x01,
	0x4A76, 0x16,
	0x4A77, 0x16,
	0x4A78, 0x16,
	0x4A79, 0x16,
	0x4A7A, 0x16,
	0x4A7B, 0x16,
	0x4A7C, 0x16,
	0x4A7D, 0x16,
	0x555E, 0x01,
	0x5563, 0x01,
	0x70DA, 0x02
};

static void sensor_init(void) {
	cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_init_cereus_imx486_sunny, sizeof(addr_data_pair_init_cereus_imx486_sunny) / sizeof(kal_uint16));
}

kal_uint16 addr_data_pair_preview_cereus_imx486_sunny[] = {
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x03,
	0x0342, 0x08,
	0x0343, 0x30,
	0x0340, 0x0A,
	0x0341, 0x6E,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0F,
	0x0349, 0xBF,
	0x034A, 0x0B,
	0x034B, 0xC7,
	0x0220, 0x00,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x00,
	0x3130, 0x01,
	0x034C, 0x07,
	0x034D, 0xE0,
	0x034E, 0x05,
	0x034F, 0xE4,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x00,
	0x0307, 0x54,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x00,
	0x030F, 0x54,
	0x0310, 0x01,
	0x0700, 0x01,
	0x0701, 0x50,
	0x0820, 0x07,
	0x0821, 0xE0,
	0x3100, 0x06,
	0x7036, 0x02,
	0x704E, 0x02,
	0x0202, 0x0A,
	0x0203, 0x64,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x0226, 0x01,
	0x0227, 0x00,
	0x0b06, 0x01
};

static void preview_setting(void) {
	cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_preview_cereus_imx486_sunny, sizeof(addr_data_pair_preview_cereus_imx486_sunny) / sizeof(kal_uint16));
}

kal_uint16 addr_data_pair_capture_15fps_cereus_imx486_sunny[] = {
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x03,
	0x0342, 0x10,
	0x0343, 0x60,
	0x0340, 0x0B,
	0x0341, 0xFC,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0F,
	0x0349, 0xBF,
	0x034A, 0x0B,
	0x034B, 0xC7,
	0x0220, 0x00,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x00,
	0x3130, 0x01,
	0x034C, 0x0F,
	0x034D, 0xC0,
	0x034E, 0x0B,
	0x034F, 0xC8,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x00,
	0x0307, 0x61,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x00,
	0x030F, 0x54,
	0x0310, 0x01,
	0x0700, 0x00,
	0x0701, 0x98,
	0x0820, 0x07,
	0x0821, 0xE0,
	0x3100, 0x06,
	0x7036, 0x02,
	0x704E, 0x02,
	0x0202, 0x0B,
	0x0203, 0xF2,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x020F, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x0226, 0x01,
	0x0227, 0x00,
	0x0b06, 0x00
};

kal_uint16 addr_data_pair_capture_24fps_cereus_imx486_sunny[] = {
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x03,
	0x0342, 0x10,
	0x0343, 0x60,
	0x0340, 0x0B,
	0x0341, 0xFC,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0F,
	0x0349, 0xBF,
	0x034A, 0x0B,
	0x034B, 0xC7,
	0x0220, 0x00,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x00,
	0x3130, 0x01,
	0x034C, 0x0F,
	0x034D, 0xC0,
	0x034E, 0x0B,
	0x034F, 0xC8,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x00,
	0x0307, 0x9C,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x00,
	0x030F, 0x84,
	0x0310, 0x01,
	0x0700, 0x00,
	0x0701, 0x40,
	0x0820, 0x0C,
	0x0821, 0x60,
	0x3100, 0x06,
	0x7036, 0x00,
	0x704E, 0x02,
	0x0202, 0x0B,
	0x0203, 0xF2,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x020F, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x0226, 0x01,
	0x0227, 0x00,
	0x0b06, 0x00
};

kal_uint16 addr_data_pair_capture_30fps_cereus_imx486_sunny[] = {
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x03,
	0x0342, 0x10,
	0x0343, 0x60,
	0x0340, 0x0B,
	0x0341, 0xFC,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0F,
	0x0349, 0xBF,
	0x034A, 0x0B,
	0x034B, 0xC7,
	0x0220, 0x00,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x00,
	0x3130, 0x01,
	0x034C, 0x0F,
	0x034D, 0xC0,
	0x034E, 0x0B,
	0x034F, 0xC8,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x00,
	0x0307, 0xC1,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x00,
	0x030F, 0xA4,
	0x0310, 0x01,
	0x0700, 0x00,
	0x0701, 0x50,
	0x0820, 0x0F,
	0x0821, 0x60,
	0x3100, 0x06,
	0x7036, 0x00,
	0x704E, 0x00,
	0x0202, 0x0B,
	0x0203, 0xF2,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x020F, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x0226, 0x01,
	0x0227, 0x00,
	0x0b06, 0x00
};

static void capture_setting(kal_uint16 current_fps) {
	if (current_fps == 150) {
		cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_capture_15fps_cereus_imx486_sunny, sizeof(addr_data_pair_capture_15fps_cereus_imx486_sunny) / sizeof(kal_uint16));
	} else if (current_fps == 240) {
		cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_capture_24fps_cereus_imx486_sunny, sizeof(addr_data_pair_capture_24fps_cereus_imx486_sunny) / sizeof(kal_uint16));
	} else {
		cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_capture_30fps_cereus_imx486_sunny, sizeof(addr_data_pair_capture_30fps_cereus_imx486_sunny) / sizeof(kal_uint16));
	}
}

static void normal_video_setting(void) {
	cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_capture_30fps_cereus_imx486_sunny, sizeof(addr_data_pair_capture_30fps_cereus_imx486_sunny) / sizeof(kal_uint16));
}

kal_uint16 addr_data_pair_hs_video_cereus_imx486_sunny[] = {
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x03,
	0x0342, 0x08,
	0x0343, 0x30,
	0x0340, 0x03,
	0x0341, 0x02,
	0x0344, 0x02,
	0x0345, 0xE0,
	0x0346, 0x03,
	0x0347, 0x10,
	0x0348, 0x0C,
	0x0349, 0xDF,
	0x034A, 0x08,
	0x034B, 0xAF,
	0x0220, 0x00,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x00,
	0x3130, 0x01,
	0x034C, 0x05,
	0x034D, 0x00,
	0x034E, 0x02,
	0x034F, 0xD0,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x00,
	0x0307, 0x61,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x00,
	0x030F, 0x54,
	0x0310, 0x01,
	0x0700, 0x00,
	0x0701, 0x38,
	0x0820, 0x07,
	0x0821, 0xE0,
	0x3100, 0x06,
	0x7036, 0x02,
	0x704E, 0x02,
	0x0202, 0x02,
	0x0203, 0xF8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x0226, 0x01,
	0x0227, 0x00
};

static void hs_video_setting(void) {
	cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_hs_video_cereus_imx486_sunny, sizeof(addr_data_pair_hs_video_cereus_imx486_sunny) / sizeof(kal_uint16));
}

static void slim_video_setting(void) {
	cereus_imx486_sunny_table_write_cmos_sensor(addr_data_pair_preview_cereus_imx486_sunny, sizeof(addr_data_pair_preview_cereus_imx486_sunny) / sizeof(kal_uint16));
}

static kal_uint32 return_sensor_id(void) {
	int retry = 10;

	if (write_cmos_sensor_8(0xA02, 0xB) == 0) {
		write_cmos_sensor_8(0xA00, 0x1);

		while (retry--) {
			if (read_cmos_sensor(0xA01) == 0x1) {
				return (kal_uint16) ((read_cmos_sensor(0xA29) << 4) | (read_cmos_sensor(0xA2A) >> 4));
			}
		}
	}

	return 0x0;
}

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id) {
	kal_uint8 i, retry;
	int size;

	for (i = 0; imgsensor_info.i2c_addr_table[i] != 0xFF; i++) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);

		size = imgSensorReadEepromData(&sensor_eeprom_data, cereus_imx486_sunny_checksum, 11);
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
	if (enable) {
		write_cmos_sensor_8(0x601, 0x2);
	} else {
		write_cmos_sensor_8(0x601, 0x0);
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable) {
	LOG_DBG("%d\n", enable);

	if (enable) {
		write_cmos_sensor_8(0x100, 0x1);
	} else {
		write_cmos_sensor_8(0x100, 0x0);
	}
	mdelay(10);

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
			LOG_DBG("SENSOR_SET_SENSOR_IHDR LE: %d, SE: %d, Gain: %d\n", (UINT16) * feature_data, (UINT16) * (feature_data + 1), (UINT16) * (feature_data + 2));
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

UINT32 CEREUS_IMX486_SUNNY_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc) {
	if (pfFunc != NULL) {
		*pfFunc = &sensor_func;
	}

	return ERROR_NONE;
}
