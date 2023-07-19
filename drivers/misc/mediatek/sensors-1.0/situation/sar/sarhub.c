/* sarhub motion sensor driver
 *
 * Copyright (C) 2016 MediaTek Inc.
 * Copyright (C) 2021 XiaoMi, Inc.
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

#define pr_fmt(fmt) "[sarhub] " fmt

#include <hwmsensor.h>
#include "sarhub.h"
#include <situation.h>
#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "scp_helper.h"
#include "scp_helper.h"
#include "sar_factory.h"

static struct situation_init_info sarhub_init_info;
static DEFINE_SPINLOCK(calibration_lock);
struct sarhub_ipi_data {
	bool factory_enable;
	atomic_t  trace ;

	int32_t cali_data[3];
	int8_t cali_status;
	struct completion calibration_done;
};
static struct sarhub_ipi_data *obj_ipi_data;


static int sar_factory_enable_sensor(bool enabledisable,
					 int64_t sample_periods_ms)
{
	int err = 0;
	struct sarhub_ipi_data *obj = obj_ipi_data;

	if (enabledisable == true)
		WRITE_ONCE(obj->factory_enable, true);
	else
		WRITE_ONCE(obj->factory_enable, false);
	if (enabledisable == true) {
		err = sensor_set_delay_to_hub(ID_SAR,
					      sample_periods_ms);
		if (err) {
			pr_err("sensor_set_delay_to_hub failed!\n");
			return -1;
		}
	}
	err = sensor_enable_to_hub(ID_SAR, enabledisable);
	if (err) {
		pr_err("sensor_enable_to_hub failed!\n");
		return -1;
	}
	return 0;
}

static int sar_factory_get_data(int32_t sensor_data[3])
{
	int err = 0;
	struct data_unit_t data;

	err = sensor_get_data_from_hub(ID_SAR, &data);
	if (err < 0) {
		pr_err_ratelimited("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	sensor_data[0] = data.sar_event.data[0];
	sensor_data[1] = data.sar_event.data[1];
	sensor_data[2] = data.sar_event.data[2];

	return err;
}

static int sar_factory_enable_calibration(void)
{
	return sensor_calibration_to_hub(ID_SAR);
}

static int sar_factory_get_cali(int32_t data[3])
{
	int err = 0;
	struct sarhub_ipi_data *obj = obj_ipi_data;
	int8_t status = 0;

	err = wait_for_completion_timeout(&obj->calibration_done,
					  msecs_to_jiffies(3000));
	if (!err) {
		pr_err("sar factory get cali fail!\n");
		return -1;
	}
	spin_lock(&calibration_lock);
	data[0] = obj->cali_data[0];
	data[1] = obj->cali_data[1];
	data[2] = obj->cali_data[2];
	status = obj->cali_status;
	spin_unlock(&calibration_lock);
	if (status != 0) {
		pr_debug("sar cali fail!\n");
		return -2;
	}
	return 0;
}


static struct sar_factory_fops sarhub_factory_fops = {
	.enable_sensor = sar_factory_enable_sensor,
	.get_data = sar_factory_get_data,
	.enable_calibration = sar_factory_enable_calibration,
	.get_cali = sar_factory_get_cali,
};

static struct sar_factory_public sarhub_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &sarhub_factory_fops,
};

static int sar_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_SAR, &data);
	if (err < 0) {
		pr_err_ratelimited("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp		= data.time_stamp;
	*probability	= data.sar_event.data[0];
	return 0;
}
static int sar_open_report_data(int open)
{
	int ret = 0;
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_SAR, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	ret = sensor_enable_to_hub(ID_SAR, open);
	return ret;
}
static int sar_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_SAR,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int sar_flush(void)
{
	return sensor_flush_to_hub(ID_SAR);
}

static int sar_recv_data(struct data_unit_t *event, void *reserved)
{
	struct sarhub_ipi_data *obj = obj_ipi_data;
	int32_t value[3] = {0};
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
		err = situation_flush_report(ID_SAR);
	else if (event->flush_action == DATA_ACTION) {
		value[0] = event->sar_event.data[0];
		value[1] = event->sar_event.data[1];
		value[2] = event->sar_event.data[2];
		err = sar_data_report(value);
	} else if (event->flush_action == CALI_ACTION) {
		spin_lock(&calibration_lock);
		obj->cali_data[0] =
			event->sar_event.x_bias;
		obj->cali_data[1] =
			event->sar_event.y_bias;
		obj->cali_data[2] =
			event->sar_event.z_bias;
		obj->cali_status =
			(int8_t)event->sar_event.status;
		spin_unlock(&calibration_lock);
		complete(&obj->calibration_done);
	}
	return err;
}

/*2020.4.11 longcheer liushuwen add start for sar TX_POWER*/
static ssize_t sar_show_trace(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t res = 0;
	struct sarhub_ipi_data *obj = obj_ipi_data;

	if (!obj_ipi_data) {
		pr_err("obj_ipi_data is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t sar_store_trace(struct device *dev,
	struct device_attribute *attr, const char *buf,size_t count)
{
	int trace = 0;
	struct sarhub_ipi_data *obj = obj_ipi_data;
	int res = 0;
	int ret = 0;

	if (!obj) {
		pr_err("obj_ipi_data is null!!\n");
		return 0;
	}

	ret = sscanf(buf, "%d", &trace);
	if (ret != 1) {
		pr_err("invalid content: '%s', length = %zu\n", buf, count);
		return count;
	}
	atomic_set(&obj->trace, trace);
	res = sensor_set_cmd_to_hub(ID_SAR,
				CUST_ACTION_SET_TRACE, &trace);
	if (res < 0) {
		pr_err("sensor_set_cmd_to_hub fail,(ID: %d),(action: %d)\n",
				ID_SAR, CUST_ACTION_SET_TRACE);
		return count;
	}
	return count;
}

DEVICE_ATTR(sar_trace, 0644, sar_show_trace, sar_store_trace);

static int sarhub_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev;

	pr_err("sarhub_probe\n");

	dev = &pdev->dev;

	ret = device_create_file(dev, &dev_attr_sar_trace);
	if(ret != 0) {
		pr_err("device create fail\n");
		return ret;
	}
	pr_err("sarhub_probe ok\n");

	return 0;
}

static int sarhub_remove(struct platform_device *pdev)
{
	device_remove_file(&(pdev->dev), &dev_attr_sar_trace);

	return 0;
}

static struct platform_device sarhub_device = {
	.name = "sar_hub_s",
	.id = -1,
};

static struct platform_driver sarhub_driver = {
        .probe = sarhub_probe,
        .remove = sarhub_remove,
        .driver = {
                .name = "sar_hub_s",
        },
};
/*2020.4.11 longcheer liushuwen add end for sar TX_POWER*/

static int sarhub_local_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	struct sarhub_ipi_data *obj;

	pr_err("%s\n", __func__);
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		return -ENOMEM;
		//goto exit;
	}

	memset(obj, 0, sizeof(*obj));
	obj_ipi_data = obj;
	WRITE_ONCE(obj->factory_enable, false);

	init_completion(&obj->calibration_done);

	ctl.open_report_data = sar_open_report_data;
	ctl.batch = sar_batch;
	ctl.flush = sar_flush;
	ctl.is_support_wake_lock = true;
	ctl.is_support_batch = false;
	err = situation_register_control_path(&ctl, ID_SAR);
	if (err) {
		pr_err("register sar control path err\n");
		goto exit;
	}

	data.get_data = sar_get_data;
	err = situation_register_data_path(&data, ID_SAR);
	if (err) {
		pr_err("register sar data path err\n");
		goto exit;
	}

	err = sar_factory_device_register(&sarhub_factory_device);
	if (err) {
		pr_err("sar_factory_device register failed\n");
		goto exit;
	}

	err = scp_sensorHub_data_registration(ID_SAR,
		sar_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit;
	}
/*2020 4.11 longcheer liushuwen add statrt for sar  TX_POWER*/
	err = platform_driver_register(&sarhub_driver);
	if (err) {
		pr_err("sarhub_local_init add driver error\n");
		goto exit;
	}
/*2020 4.11 longcheer liushuwen add end for sar TX_POWER*/
	return 0;
exit:
	kfree(obj);
	obj_ipi_data = NULL;

	return err;
}
static int sarhub_local_uninit(void)
{
	platform_driver_unregister(&sarhub_driver);
	return 0;
}

static struct situation_init_info sarhub_init_info = {
	.name = "sar_hub",
	.init = sarhub_local_init,
	.uninit = sarhub_local_uninit,
};

static int __init sarhub_init(void)
{
	pr_err("sarhub_initl!!\n");
/*2020 4.11 longcheer liushuwen add statrt for sar TX_POWER*/
	if (platform_device_register(&sarhub_device)) {
		pr_err("sar platform device error\n");
		return -1;
	}
/*2020 4.11 longcheer liushuwen add end for sar TX_POWER*/

	situation_driver_add(&sarhub_init_info, ID_SAR);
	return 0;
}

static void __exit sarhub_exit(void)
{
	pr_debug("%s\n", __func__);
}

module_init(sarhub_init);
module_exit(sarhub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SAR_HUB driver");
MODULE_AUTHOR("Jashon.zhang@mediatek.com");
