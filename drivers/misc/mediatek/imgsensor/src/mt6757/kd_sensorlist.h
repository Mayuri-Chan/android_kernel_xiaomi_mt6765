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

extern unsigned int abist_meter(int ID);
extern struct MULTI_SENSOR_FUNCTION_STRUCT2 kd_MultiSensorFunc;
extern int kdCISModulePowerOn(
	enum CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx,
	char *currSensorName,
	BOOL On, char *mode_name);

/* s_add new sensor driver here */
/* export functions */



/*
 * Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This file should be the same as mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT
	kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR+1] = {
	/* ADD sensor driver before this line */
	{0, {0}, NULL},		/* end of list */
};

/* e_add new sensor driver here */


