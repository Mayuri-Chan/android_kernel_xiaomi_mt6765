/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This should be the same as
 *     mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_INIT_FUNC_LIST kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(CACTUS_OV13855_OFILM_MIPI_RAW)
	{CACTUS_OV13855_OFILM_SENSOR_ID,
	SENSOR_DRVNAME_CACTUS_OV13855_OFILM_MIPI_RAW,
	CACTUS_OV13855_OFILM_MIPI_RAW_SensorInit},
#endif
#if defined(CACTUS_S5K3L8_SUNNY_MIPI_RAW)
	{CACTUS_S5K3L8_SUNNY_SENSOR_ID,
	SENSOR_DRVNAME_CACTUS_S5K3L8_SUNNY_MIPI_RAW,
	CACTUS_S5K3L8_SUNNY_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_IMX486_SUNNY_MIPI_RAW)
	{CEREUS_IMX486_SUNNY_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_IMX486_SUNNY_MIPI_RAW,
	CEREUS_IMX486_SUNNY_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_OV12A10_OFILM_MIPI_RAW)
	{CEREUS_OV12A10_OFILM_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_OV12A10_OFILM_MIPI_RAW,
	CEREUS_OV12A10_OFILM_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_S5K5E8YXAUX_OFILM_MIPI_RAW)
	{CEREUS_S5K5E8YXAUX_OFILM_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_S5K5E8YXAUX_OFILM_MIPI_RAW,
	CEREUS_S5K5E8YXAUX_OFILM_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_S5K5E8YXAUX_SUNNY_MIPI_RAW)
	{CEREUS_S5K5E8YXAUX_SUNNY_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_S5K5E8YXAUX_SUNNY_MIPI_RAW,
	CEREUS_S5K5E8YXAUX_SUNNY_MIPI_RAW_SensorInit},
#endif
#if defined(CACTUS_HI556_SUNNY_MIPI_RAW)
	{CACTUS_HI556_SUNNY_SENSOR_ID,
	SENSOR_DRVNAME_CACTUS_HI556_SUNNY_MIPI_RAW,
	CACTUS_HI556_SUNNY_MIPI_RAW_SensorInit},
#endif
#if defined(CACTUS_S5K5E8YX_OFILM_MIPI_RAW)
	{CACTUS_S5K5E8YX_OFILM_SENSOR_ID,
	SENSOR_DRVNAME_CACTUS_S5K5E8YX_OFILM_MIPI_RAW,
	CACTUS_S5K5E8YX_OFILM_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_S5K5E8YX_OFILM_MIPI_RAW)
	{CEREUS_S5K5E8YX_OFILM_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_S5K5E8YX_OFILM_MIPI_RAW,
	CEREUS_S5K5E8YX_OFILM_MIPI_RAW_SensorInit},
#endif
#if defined(CEREUS_S5K5E8YX_SUNNY_MIPI_RAW)
	{CEREUS_S5K5E8YX_SUNNY_SENSOR_ID,
	SENSOR_DRVNAME_CEREUS_S5K5E8YX_SUNNY_MIPI_RAW,
	CEREUS_S5K5E8YX_SUNNY_MIPI_RAW_SensorInit},
#endif
	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};
/* e_add new sensor driver here */
