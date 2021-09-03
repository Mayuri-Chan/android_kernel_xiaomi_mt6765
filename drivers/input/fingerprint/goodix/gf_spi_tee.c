/*
 * Copyright (c) 2013-2016, Shenzhen Huiding Technology Co., Ltd.
 * Copyright (c) 2020 XiaoMi, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include <net/sock.h>
#include <linux/fb.h>
#include <linux/of_irq.h>
#include <linux/spi/spi.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>

#include "teei_fp.h"
#include "tee_client_api.h"

#include "gf_spi_tee.h"

#define GF_DEV_NAME "goodix_fp"
#define GF_SPI_SPEED 1000000;

#define GF_NETLINK_ROUTE 29
#define MAX_NL_MSG_LEN 16

#define INPUT_KEY_EVENTS 0

#define ROUND_UP(x, align) ((x + align - 1) &~ (align - 1))

bool goodix_fp_exist = false;
extern bool fpc1022_fp_exist;

struct spi_device *spi_fingerprint;
struct TEEC_UUID uuid_ta_gf = {0x8888C03F, 0xC30C, 0x4DD0, {0xA3, 0x19, 0xEA, 0x29, 0x64, 0x3D, 0x4D, 0x4B}};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static struct wakeup_source fp_wakesrc;
static atomic_t clk_ref = ATOMIC_INIT(0);

static unsigned int bufsiz = (25 * 1024);
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "maximum data bytes for SPI message");

static const struct of_device_id gf_of_match[] = {{.compatible = "goodix,goodix-fp"}};
MODULE_DEVICE_TABLE(of, gf_of_match);

int gf_spi_read_bytes(struct gf_device *gf_dev, u16 addr, u32 data_len, u8 *rx_buf);
struct regulator *buck;

/* for netlink use */
static unsigned int pid = 0;
static u8 g_vendor_id = 0;

static ssize_t gf_debug_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t gf_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUSR, gf_debug_show, gf_debug_store);

static struct attribute *gf_debug_attrs[] = {
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group gf_debug_attr_group = {
	.attrs = gf_debug_attrs,
	.name = "debug"
};

/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration								  */
/* -------------------------------------------------------------------- */
static long gf_get_gpio_dts_info(struct gf_device *gf_dev) {
	long ret;
	unsigned int virq;

	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;

	gf_debug(DEBUG_LOG, "%s, from dts pinctrl\n", __func__);

	node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
	gf_debug(ERR_LOG, "[gf][goodix_test] %s node = %p\n", __func__, node);
	if (node) {
		virq = irq_of_parse_and_map(node, 0);
		gf_debug(ERR_LOG, "[gf][goodix_test] %s virq = %d\n", __func__,
		         virq);
		irq_set_irq_wake(virq, 1);
		gf_debug(ERR_LOG,
		         "[gf][goodix_test] %s CONFIG_MTK_EIC define\n",
		         __func__);
		pdev = of_find_device_by_node(node);
		if (pdev) {
			gf_dev->pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(gf_dev->pinctrl_gpios)) {
				ret = PTR_ERR(gf_dev->pinctrl_gpios);
				gf_debug(ERR_LOG,
				         "%s can't find fingerprint pinctrl\n",
				         __func__);
				return ret;
			}
		} else {
			gf_debug(ERR_LOG, "%s platform device is null\n",
			         __func__);
		}
	} else {
		gf_debug(ERR_LOG, "%s device node is null\n", __func__);
	}

	gf_dev->pins_irq =
		pinctrl_lookup_state(gf_dev->pinctrl_gpios, "default");
	if (IS_ERR(gf_dev->pins_irq)) {
		ret = PTR_ERR(gf_dev->pins_irq);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl irq\n",
		         __func__);
		return ret;
	}

	gf_debug(ERR_LOG, "[gf][goodix_test] %s goodix_irq found\n", __func__);

	gf_dev->pins_reset_high =
		pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_high");
	if (IS_ERR(gf_dev->pins_reset_high)) {
		ret = PTR_ERR(gf_dev->pins_reset_high);
		gf_debug(ERR_LOG,
		         "%s can't find fingerprint pinctrl reset_high\n",
		         __func__);
		return ret;
	}

	gf_dev->pins_reset_low =
		pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_low");
	if (IS_ERR(gf_dev->pins_reset_low)) {
		ret = PTR_ERR(gf_dev->pins_reset_low);
		gf_debug(ERR_LOG,
		         "%s can't find fingerprint pinctrl reset_low\n",
		         __func__);
		return ret;
	}

	gf_debug(DEBUG_LOG, "%s, get pinctrl success!\n", __func__);

	return 0;
}

static int gf_get_sensor_dts_info(void) {
	struct device_node *node = NULL;
	int value;

	node = of_find_compatible_node(NULL, NULL, "goodix,goodix-fp");
	if (node) {
		of_property_read_u32(node, "netlink-event", &value);
		gf_debug(DEBUG_LOG, "%s, get netlink event[%d] from dts\n",
		         __func__, value);
	} else {
		gf_debug(ERR_LOG, "%s failed to get device node!\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static void gf_hw_power_enable(struct gf_device *gf_dev, u8 onoff) {
	static int enable = 1;
	pr_err("%s\n", __func__);
	if (onoff && enable) {
		pr_err("%s %d now reset sensor,pull low\n", __func__, __LINE__);

		enable = 0;
		pinctrl_select_state(gf_dev->pinctrl_gpios,
		                     gf_dev->pins_reset_low);
		mdelay(15);
		pr_err("%s %d now reset sensor pull high\n", __func__,
		       __LINE__);
		pinctrl_select_state(gf_dev->pinctrl_gpios,
		                     gf_dev->pins_reset_high);
	} else if (!onoff && !enable) {
		pr_err("%s %d do nothing\n", __func__, __LINE__);
		enable = 1;
	}
}

static void gf_spi_clk_enable(struct gf_device *gf_dev, u8 bonoff) {
	if (bonoff) {
		if (atomic_read(&clk_ref) == 0) {
			pr_err("%s : ree control spi clk, enable spi clk\n",
			       __func__);
			mt_spi_enable_master_clk(gf_dev->spi);
		}

		atomic_inc(&clk_ref);
		gf_debug(DEBUG_LOG, "%s : increase spi clk ref to %d\n", __func__, atomic_read(&clk_ref));
	} else if (bonoff == 0) {
		atomic_dec(&clk_ref);
		gf_debug(DEBUG_LOG, "%s : decrease spi clk ref to %d\n", __func__, atomic_read(&clk_ref));

		if (atomic_read(&clk_ref) == 0) {
			pr_err("%s :ree control spi clk, disable spi clk\n",
			       __func__);
			mt_spi_disable_master_clk(gf_dev->spi);
		}
	}
}

static void gf_irq_gpio_cfg(struct gf_device *gf_dev) {
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
	if (node) {
		gf_dev->irq_num = irq_of_parse_and_map(node, 0);
		gf_debug(INFO_LOG, "%s, gf_irq = %d\n", __func__,
		         gf_dev->irq_num);
		gf_dev->irq = gf_dev->irq_num;
	} else
		gf_debug(ERR_LOG, "%s can't find compatible node\n", __func__);
}

static void gf_reset_gpio_cfg(struct gf_device *gf_dev) {
	pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);
}

/* delay ms after reset */
static void gf_hw_reset(struct gf_device *gf_dev, u8 delay) {
	pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_low);
	mdelay(5);
	pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);

	if (delay) {
		/* delay is configurable */
		mdelay(delay);
	}
}

static void gf_enable_irq(struct gf_device *gf_dev) {
	if (1 == gf_dev->irq_count) {
		gf_debug(ERR_LOG, "%s, irq already enabled\n", __func__);
	} else {
		enable_irq(gf_dev->irq);
		gf_dev->irq_count = 1;
		gf_debug(DEBUG_LOG, "%s enable interrupt!\n", __func__);
	}
}

static void gf_disable_irq(struct gf_device *gf_dev) {
	if (0 == gf_dev->irq_count) {
		gf_debug(ERR_LOG, "%s, irq already disabled\n", __func__);
	} else {
		disable_irq(gf_dev->irq);
		gf_dev->irq_count = 0;
		gf_debug(DEBUG_LOG, "%s disable interrupt!\n", __func__);
	}
}

/* -------------------------------------------------------------------- */
/* netlink functions                 */
/* -------------------------------------------------------------------- */
static void gf_netlink_send(struct gf_device *gf_dev, const int command) {
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *skb = NULL;
	int ret;

	gf_debug(INFO_LOG, "[%s] : enter, send command %d\n", __func__,
	         command);
	if (NULL == gf_dev->nl_sk) {
		gf_debug(ERR_LOG, "[%s] : invalid socket\n", __func__);
		return;
	}

	if (0 == pid) {
		gf_debug(ERR_LOG, "[%s] : invalid native process pid\n",
		         __func__);
		return;
	}

	/*alloc data buffer for sending to native */
	/*malloc data space at least 1500 bytes, which is ethernet data length */
	skb = alloc_skb(MAX_NL_MSG_LEN, GFP_ATOMIC);
	if (skb == NULL) {
		return;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, MAX_NL_MSG_LEN, 0);
	if (!nlh) {
		gf_debug(ERR_LOG, "[%s] : nlmsg_put failed\n", __func__);
		kfree_skb(skb);
		return;
	}

	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;

	*(char *) NLMSG_DATA(nlh) = command;
	ret = netlink_unicast(gf_dev->nl_sk, skb, pid, MSG_DONTWAIT);
	if (ret == 0) {
		gf_debug(ERR_LOG, "[%s] : send failed\n", __func__);
		return;
	}

	gf_debug(INFO_LOG, "[%s] : send done, data length is %d\n", __func__,
	         ret);
}

static void gf_netlink_recv(struct sk_buff *_skb) {
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	char str[128];

	gf_debug(INFO_LOG, "[%s] : enter \n", __func__);

	skb = skb_get(_skb);
	if (skb == NULL) {
		gf_debug(ERR_LOG, "[%s] : skb_get return NULL\n", __func__);
		return;
	}

	/* presume there is 5byte payload at leaset */
	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pid = nlh->nlmsg_pid;
		gf_debug(INFO_LOG, "[%s] : pid: %d, msg: %s\n", __func__, pid,
		         str);
	} else {
		gf_debug(ERR_LOG, "[%s] : not enough data length\n", __func__);
	}

	kfree_skb(skb);
}

static int gf_netlink_init(struct gf_device *gf_dev) {
	struct netlink_kernel_cfg cfg;

	memset(&cfg, 0, sizeof(struct netlink_kernel_cfg));
	cfg.input = gf_netlink_recv;

	gf_dev->nl_sk =
		netlink_kernel_create(&init_net, GF_NETLINK_ROUTE, &cfg);
	if (gf_dev->nl_sk == NULL) {
		gf_debug(ERR_LOG, "[%s] : netlink create failed\n", __func__);
		return -1;
	}

	gf_debug(INFO_LOG, "[%s] : netlink create success\n", __func__);
	return 0;
}

static int gf_netlink_destroy(struct gf_device *gf_dev) {
	if (gf_dev->nl_sk != NULL) {
		netlink_kernel_release(gf_dev->nl_sk);
		gf_dev->nl_sk = NULL;
		return 0;
	}

	gf_debug(ERR_LOG, "[%s] : no netlink socket yet\n", __func__);
	return -1;
}

static int gf_fb_notifier_callback(struct notifier_block *self,
                                   unsigned long event, void *data) {
	struct gf_device *gf_dev = NULL;
	struct fb_event *evdata = data;
	unsigned int blank;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK) {
		return 0;
	}

	gf_dev = container_of(self, struct gf_device, notifier);

	if (evdata && evdata->data && gf_dev) {
		blank = *(int *) evdata->data;

		gf_debug(INFO_LOG, "[%s] : enter, blank=0x%x\n", __func__, blank);

		switch (blank) {
			case FB_BLANK_UNBLANK:
				gf_debug(INFO_LOG, "[%s] : lcd on notify\n", __func__);
				gf_netlink_send(gf_dev, GF_NETLINK_SCREEN_ON);
				break;

			case FB_BLANK_POWERDOWN:
				gf_debug(INFO_LOG, "[%s] : lcd off notify\n", __func__);
				gf_netlink_send(gf_dev, GF_NETLINK_SCREEN_OFF);
				break;

			default:
				gf_debug(INFO_LOG, "[%s] : other notifier, ignore\n", __func__);
				break;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------- */
/* file operation function                                              */
/* -------------------------------------------------------------------- */
static ssize_t gf_read(struct file *filp, char __user *buf, size_t count,
                       loff_t *f_pos) {

	return 0;
}

static ssize_t gf_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *f_pos) {
	gf_debug(ERR_LOG, "%s: Not support write opertion in TEE mode\n",
	         __func__);
	return -EFAULT;
}

static irqreturn_t gf_irq(int irq, void *handle) {
	struct gf_device *gf_dev = (struct gf_device *) handle;

	__pm_wakeup_event(&fp_wakesrc, 3000);
	gf_netlink_send(gf_dev, GF_NETLINK_IRQ);
	gf_dev->sig_count++;

	return IRQ_HANDLED;
}

static long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	struct gf_device *gf_dev = NULL;
	gf_nav_event_t nav_event = GF_NAV_NONE;
	uint32_t nav_input = 0;
	int retval = 0;
	u8 buf = 0;
	u8 netlink_route = GF_NETLINK_ROUTE;
	struct gf_ioc_chip_info info;
#if INPUT_KEY_EVENTS
	struct gf_key gf_key;
	uint32_t key_input;
#endif

	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC) {
		return -EINVAL;
	}

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ) {
		retval =
			!access_ok(VERIFY_WRITE, (void __user *)arg,
			           _IOC_SIZE(cmd));
	}

	if (retval == 0 && _IOC_DIR(cmd) & _IOC_WRITE) {
		retval =
			!access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if (retval) {
		return -EINVAL;
	}

	gf_dev = (struct gf_device *) filp->private_data;
	if (!gf_dev) {
		gf_debug(ERR_LOG, "%s: gf_dev IS NULL ======\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
		case GF_IOC_INIT:
			gf_debug(INFO_LOG, "%s: GF_IOC_INIT gf init======\n", __func__);

			if (copy_to_user
				((void __user *) arg, (void *) &netlink_route, sizeof(u8))) {
				retval = -EFAULT;
				break;
			}

			if (gf_dev->system_status) {
				gf_debug(INFO_LOG, "%s: system re-started======\n",
				         __func__);
				break;
			}
			gf_irq_gpio_cfg(gf_dev);
			retval = request_threaded_irq(gf_dev->irq, NULL, gf_irq,
			                              IRQF_TRIGGER_RISING |
			                              IRQF_ONESHOT, "goodix_fp_irq",
			                              gf_dev);
			if (!retval)
				gf_debug(INFO_LOG, "%s irq thread request success!\n",
				         __func__);
			else
				gf_debug(ERR_LOG,
				         "%s irq thread request failed, retval=%d\n",
				         __func__, retval);

			gf_dev->irq_count = 1;
			gf_disable_irq(gf_dev);

			/* register screen on/off callback */
			gf_dev->notifier.notifier_call = gf_fb_notifier_callback;
			fb_register_client(&gf_dev->notifier);

			gf_dev->sig_count = 0;
			gf_dev->system_status = 1;

			gf_debug(INFO_LOG, "%s: gf init finished======\n", __func__);
			break;

		case GF_IOC_CHIP_INFO:
			if (copy_from_user
				(&info, (struct gf_ioc_chip_info *) arg,
				 sizeof(struct gf_ioc_chip_info))) {
				retval = -EFAULT;
				break;
			}
			g_vendor_id = info.vendor_id;

			gf_debug(INFO_LOG, "%s: vendor_id 0x%x\n", __func__,
			         g_vendor_id);
			gf_debug(INFO_LOG, "%s: mode 0x%x\n", __func__, info.mode);
			gf_debug(INFO_LOG, "%s: operation 0x%x\n", __func__,
			         info.operation);
			break;

		case GF_IOC_EXIT:
			gf_debug(INFO_LOG, "%s: GF_IOC_EXIT ======\n", __func__);
			gf_disable_irq(gf_dev);
			if (gf_dev->irq) {
				free_irq(gf_dev->irq, gf_dev);
				gf_dev->irq_count = 0;
				gf_dev->irq = 0;
			}

			fb_unregister_client(&gf_dev->notifier);

			gf_dev->system_status = 0;
			gf_debug(INFO_LOG, "%s: gf exit finished ======\n", __func__);
			break;

		case GF_IOC_RESET:
			printk("%s: chip reset command\n", __func__);
			gf_hw_reset(gf_dev, 60);
			break;

		case GF_IOC_ENABLE_IRQ:
			gf_debug(INFO_LOG, "%s: GF_IOC_ENABLE_IRQ ======\n", __func__);
			gf_enable_irq(gf_dev);
			break;

		case GF_IOC_DISABLE_IRQ:
			gf_debug(INFO_LOG, "%s: GF_IOC_DISABLE_IRQ ======\n", __func__);
			gf_disable_irq(gf_dev);
			break;

		case GF_IOC_ENABLE_SPI_CLK:
			printk("%s: GF_IOC_ENABLE_SPI_CLK ======\n", __func__);
			gf_spi_clk_enable(gf_dev, 1);
			break;

		case GF_IOC_DISABLE_SPI_CLK:
			printk("%s: GF_IOC_DISABLE_SPI_CLK ======\n", __func__);
			gf_spi_clk_enable(gf_dev, 0);
			break;

		case GF_IOC_ENABLE_POWER:
			gf_debug(INFO_LOG, "%s: GF_IOC_ENABLE_POWER ======\n",
			         __func__);
			gf_hw_power_enable(gf_dev, 1);
			break;

		case GF_IOC_DISABLE_POWER:
			gf_debug(INFO_LOG, "%s: GF_IOC_DISABLE_POWER ======\n",
			         __func__);
			gf_hw_power_enable(gf_dev, 0);
			break;

		case GF_IOC_INPUT_KEY_EVENT:
#if INPUT_KEY_EVENTS
			if (copy_from_user
				(&gf_key, (struct gf_key *) arg, sizeof(struct gf_key))) {
				gf_debug(ERR_LOG,
				         "Failed to copy input key event from user to kernel\n");
				retval = -EFAULT;
				break;
			}

			if (GF_KEY_HOME == gf_key.key) {
				key_input = GF_KEY_INPUT_HOME;
			} else if (GF_KEY_POWER == gf_key.key) {
				key_input = GF_KEY_INPUT_POWER;
			} else if (GF_KEY_CAMERA == gf_key.key) {
				key_input = GF_KEY_INPUT_CAMERA;
			} else {
				/* add special key define */
				key_input = gf_key.key;
			}
			gf_debug(INFO_LOG,
			         "%s: received key event[%d], key=%d, value=%d\n",
			         __func__, key_input, gf_key.key, gf_key.value);

			if ((GF_KEY_POWER == gf_key.key || GF_KEY_CAMERA == gf_key.key)
			    && (gf_key.value == 1)) {
				input_report_key(gf_dev->input, key_input, 1);
				input_sync(gf_dev->input);
				input_report_key(gf_dev->input, key_input, 0);
				input_sync(gf_dev->input);
			}

			if (GF_KEY_HOME == gf_key.key) {
				input_report_key(gf_dev->input, key_input, gf_key.value);
				input_sync(gf_dev->input);
			}
#endif

			break;

		case GF_IOC_NAV_EVENT:
			gf_debug(ERR_LOG, "nav event");
			if (copy_from_user
				(&nav_event, (gf_nav_event_t *) arg,
				 sizeof(gf_nav_event_t))) {
				gf_debug(ERR_LOG,
				         "Failed to copy nav event from user to kernel\n");
				retval = -EFAULT;
				break;
			}

			switch (nav_event) {
				case GF_NAV_FINGER_DOWN:
					gf_debug(ERR_LOG, "nav finger down");
					break;

				case GF_NAV_FINGER_UP:
					gf_debug(ERR_LOG, "nav finger up");
					break;

				case GF_NAV_DOWN:
					nav_input = GF_NAV_INPUT_DOWN;
					gf_debug(ERR_LOG, "nav down");
					break;

				case GF_NAV_UP:
					nav_input = GF_NAV_INPUT_UP;
					gf_debug(ERR_LOG, "nav up");
					break;

				case GF_NAV_LEFT:
					nav_input = GF_NAV_INPUT_LEFT;
					gf_debug(ERR_LOG, "nav left");
					break;

				case GF_NAV_RIGHT:
					nav_input = GF_NAV_INPUT_RIGHT;
					gf_debug(ERR_LOG, "nav right");
					break;

				case GF_NAV_CLICK:
					nav_input = GF_NAV_INPUT_CLICK;
					gf_debug(ERR_LOG, "nav click");
					break;

				case GF_NAV_HEAVY:
					nav_input = GF_NAV_INPUT_HEAVY;
					break;

				case GF_NAV_LONG_PRESS:
					nav_input = GF_NAV_INPUT_LONG_PRESS;
					break;

				case GF_NAV_DOUBLE_CLICK:
					nav_input = GF_NAV_INPUT_DOUBLE_CLICK;
					break;

				default:
					gf_debug(INFO_LOG,
					         "%s: not support nav event nav_event: %d ======\n",
					         __func__, nav_event);
					break;
			}

			if ((nav_event != GF_NAV_FINGER_DOWN)
			    && (nav_event != GF_NAV_FINGER_UP)) {
				input_report_key(gf_dev->input, nav_input, 1);
				input_sync(gf_dev->input);
				input_report_key(gf_dev->input, nav_input, 0);
				input_sync(gf_dev->input);
			}
			break;

		case GF_IOC_ENTER_SLEEP_MODE:
			gf_debug(INFO_LOG, "%s: GF_IOC_ENTER_SLEEP_MODE ======\n",
			         __func__);
			break;

		case GF_IOC_GET_FW_INFO:
			gf_debug(INFO_LOG, "%s: GF_IOC_GET_FW_INFO ======\n", __func__);
			buf = gf_dev->need_update;

			gf_debug(DEBUG_LOG, "%s: firmware info  0x%x\n", __func__, buf);
			if (copy_to_user((void __user *) arg, (void *) &buf, sizeof(u8))) {
				gf_debug(ERR_LOG, "Failed to copy data to user\n");
				retval = -EFAULT;
			}

			break;
		case GF_IOC_REMOVE:
			gf_debug(INFO_LOG, "%s: GF_IOC_REMOVE ======\n", __func__);

			gf_netlink_destroy(gf_dev);

			mutex_lock(&gf_dev->release_lock);
			if (gf_dev->input == NULL) {
				mutex_unlock(&gf_dev->release_lock);
				break;
			}
			input_unregister_device(gf_dev->input);
			gf_dev->input = NULL;
			mutex_unlock(&gf_dev->release_lock);

			cdev_del(&gf_dev->cdev);
			sysfs_remove_group(&gf_dev->spi->dev.kobj,
			                   &gf_debug_attr_group);
			device_destroy(gf_dev->class, gf_dev->devno);
			list_del(&gf_dev->device_entry);
			unregister_chrdev_region(gf_dev->devno, 1);
			class_destroy(gf_dev->class);
			gf_hw_power_enable(gf_dev, 0);
			gf_spi_clk_enable(gf_dev, 0);

			mutex_lock(&gf_dev->release_lock);
			if (gf_dev->spi_buffer != NULL) {
				kfree(gf_dev->spi_buffer);
				gf_dev->spi_buffer = NULL;
			}
			mutex_unlock(&gf_dev->release_lock);

			spi_set_drvdata(gf_dev->spi, NULL);
			gf_dev->spi = NULL;
			mutex_destroy(&gf_dev->buf_lock);
			mutex_destroy(&gf_dev->release_lock);

			break;

		default:
			gf_debug(ERR_LOG, "gf doesn't support this command(%x)\n", cmd);
			break;
	}

	return retval;
}

static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	long retval;

	retval = filp->f_op->unlocked_ioctl(filp, cmd, arg);

	return retval;
}

static unsigned int gf_poll(struct file *filp, struct poll_table_struct *wait) {
	gf_debug(ERR_LOG, "Not support poll opertion in TEE version\n");
	return -EFAULT;
}

/* -------------------------------------------------------------------- */
/* devfs                                                              */
/* -------------------------------------------------------------------- */

static ssize_t gf_debug_show(struct device *dev,
                             struct device_attribute *attr, char *buf) {
	return sprintf(buf, "vendor id 0x%x\n", g_vendor_id);
}

static ssize_t gf_debug_store(struct device *dev,
                              struct device_attribute *attr, const char *buf,
                              size_t count) {
	struct gf_device *gf_dev = dev_get_drvdata(dev);
	int retval;
	unsigned char rx_test[10] = {0};

	if (!strncmp(buf, "-8", 2)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -8, enable spi clock test===============\n",
		         __func__);
		mt_spi_enable_master_clk(gf_dev->spi);
	} else if (!strncmp(buf, "-9", 2)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -9, disable spi clock test===============\n",
		         __func__);
		mt_spi_disable_master_clk(gf_dev->spi);
	} else if (!strncmp(buf, "-10", 3)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -10, gf init start===============\n",
		         __func__);

		gf_irq_gpio_cfg(gf_dev);
		retval = request_threaded_irq(gf_dev->irq, NULL, gf_irq,
		                              IRQF_TRIGGER_RISING |
		                              IRQF_ONESHOT,
		                              dev_name(&(gf_dev->spi->dev)),
		                              gf_dev);
		if (!retval)
			gf_debug(INFO_LOG, "%s irq thread request success!\n",
			         __func__);
		else
			gf_debug(ERR_LOG,
			         "%s irq thread request failed, retval=%d\n",
			         __func__, retval);

		gf_dev->irq_count = 1;
		gf_disable_irq(gf_dev);

		/* register screen on/off callback */
		gf_dev->notifier.notifier_call = gf_fb_notifier_callback;
		fb_register_client(&gf_dev->notifier);

		gf_dev->sig_count = 0;

		gf_debug(INFO_LOG, "%s: gf init finished======\n", __func__);
	} else if (!strncmp(buf, "-11", 3)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -11, enable irq===============\n",
		         __func__);
		gf_enable_irq(gf_dev);
	} else if (!strncmp(buf, "-12", 3)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -12, GPIO test===============\n",
		         __func__);
		gf_reset_gpio_cfg(gf_dev);

		pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_miso_pulllow);
		gf_debug(INFO_LOG, "%s: set miso PIN to low\n", __func__);
	} else if (!strncmp(buf, "-13", 3)) {
		gf_debug(INFO_LOG,
		         "%s: parameter is -13, Vendor ID test --> 0x%x\n",
		         __func__, g_vendor_id);
		gf_spi_read_bytes(gf_dev, 0x0000, 4, rx_test);
		printk("%s rx_test chip id:0x%x 0x%x 0x%x 0x%x \n", __func__,
		       rx_test[0], rx_test[1], rx_test[2], rx_test[3]);
	} else {
		gf_debug(ERR_LOG, "%s: wrong parameter!===============\n",
		         __func__);
	}

	return count;
}

/* -------------------------------------------------------------------- */
/* device function								  */
/* -------------------------------------------------------------------- */
static int gf_open(struct inode *inode, struct file *filp) {
	struct gf_device *gf_dev = NULL;
	int status = -ENXIO;

	mutex_lock(&device_list_lock);
	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devno == inode->i_rdev) {
			gf_debug(INFO_LOG, "%s, Found\n", __func__);
			status = 0;
			break;
		}
	}
	mutex_unlock(&device_list_lock);

	if (status == 0) {
		filp->private_data = gf_dev;
		nonseekable_open(inode, filp);
		gf_debug(INFO_LOG, "%s, Success to open device. irq = %d\n",
		         __func__, gf_dev->irq);
	} else {
		gf_debug(ERR_LOG, "%s, No device for minor %d\n", __func__,
		         iminor(inode));
	}

	return status;
}

static int gf_release(struct inode *inode, struct file *filp) {
	struct gf_device *gf_dev = NULL;

	gf_dev = filp->private_data;
	if (gf_dev->irq) {
		gf_disable_irq(gf_dev);
	}
	gf_dev->need_update = 0;

	return 0;
}

static const struct file_operations gf_fops = {
	.owner = THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.     It'll simplify things
	 * too, except for the locking.
	 */
	.write = gf_write,
	.read = gf_read,
	.unlocked_ioctl = gf_ioctl,
	.compat_ioctl = gf_compat_ioctl,
	.open = gf_open,
	.release = gf_release,
	.poll = gf_poll,
};

/*-------------------------------------------------------------------------*/

int gf_spi_read_bytes(struct gf_device *gf_dev, u16 addr, u32 data_len,
                      u8 *rx_buf) {
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;
	u8 * tmp_buf = NULL;
	u32 package, reminder, retry;

	package = (data_len + 2) / 1024;
	reminder = (data_len + 2) % 1024;

	if ((package > 0) && (reminder != 0)) {
		xfer = kzalloc(sizeof(*xfer) * 4, GFP_KERNEL);
		retry = 1;
	} else {
		xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
		retry = 0;
	}
	if (xfer == NULL) {
		gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__);
		return -ENOMEM;
	}

	tmp_buf = gf_dev->spi_buffer;

	spi_message_init(&msg);
	*tmp_buf = 0xF0;
	*(tmp_buf + 1) = (u8) ((addr >> 8) & 0xFF);
	*(tmp_buf + 2) = (u8) (addr & 0xFF);
	xfer[0].tx_buf = tmp_buf;
	xfer[0].len = 3;

	xfer[0].speed_hz = GF_SPI_SPEED;
	gf_debug(INFO_LOG, "%s %d, now spi-clock:%d\n",
	         __func__, __LINE__, xfer[0].speed_hz);

	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(gf_dev->spi, &msg);

	spi_message_init(&msg);
	/* memset((tmp_buf + 4), 0x00, data_len + 1); */
	/* 4 bytes align */
	*(tmp_buf + 4) = 0xF1;
	xfer[1].tx_buf = tmp_buf + 4;
	xfer[1].rx_buf = tmp_buf + 4;

	if (retry) {
		xfer[1].len = package * 1024;
	} else {
		xfer[1].len = data_len + 1;
	}

	xfer[1].speed_hz = GF_SPI_SPEED;
	xfer[1].delay_usecs = 5;
	spi_message_add_tail(&xfer[1], &msg);
	spi_sync(gf_dev->spi, &msg);

	/* copy received data */
	if (retry) {
		memcpy(rx_buf, (tmp_buf + 5), (package * 1024 - 1));
	} else {
		memcpy(rx_buf, (tmp_buf + 5), data_len);
	}

	/* send reminder SPI data */
	if (retry) {
		addr = addr + package * 1024 - 2;
		spi_message_init(&msg);

		*tmp_buf = 0xF0;
		*(tmp_buf + 1) = (u8) ((addr >> 8) & 0xFF);
		*(tmp_buf + 2) = (u8) (addr & 0xFF);
		xfer[2].tx_buf = tmp_buf;
		xfer[2].len = 3;
		xfer[2].speed_hz = GF_SPI_SPEED;
		xfer[2].delay_usecs = 5;
		spi_message_add_tail(&xfer[2], &msg);
		spi_sync(gf_dev->spi, &msg);

		spi_message_init(&msg);
		*(tmp_buf + 4) = 0xF1;
		xfer[3].tx_buf = tmp_buf + 4;
		xfer[3].rx_buf = tmp_buf + 4;
		xfer[3].len = reminder + 1;
		xfer[2].speed_hz = GF_SPI_SPEED;
		xfer[3].delay_usecs = 5;
		spi_message_add_tail(&xfer[3], &msg);
		spi_sync(gf_dev->spi, &msg);

		memcpy((rx_buf + package * 1024 - 1), (tmp_buf + 6),
		       (reminder - 1));
	}

	kfree(xfer);
	xfer = NULL;

	return 0;
}

static int gf_probe(struct spi_device *spi);
static int gf_remove(struct spi_device *spi);

static struct spi_driver gf_spi_driver = {
	.driver = {
		.name = GF_DEV_NAME,
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
		.of_match_table = gf_of_match
	},
	.probe = gf_probe,
	.remove = gf_remove
};

static int gf_probe(struct spi_device *spi) {
	struct gf_device *gf_dev = NULL;
	int status = -EINVAL;
	unsigned char rx_test[10] = {0};

	/* Allocate driver data */
	gf_dev = kzalloc(sizeof(struct gf_device), GFP_KERNEL);
	if (!gf_dev) {
		status = -ENOMEM;
		goto err;
	}

	spin_lock_init(&gf_dev->spi_lock);
	mutex_init(&gf_dev->buf_lock);
	mutex_init(&gf_dev->release_lock);

	INIT_LIST_HEAD(&gf_dev->device_entry);

	gf_dev->device_count = 0;
	gf_dev->probe_finish = 0;
	gf_dev->system_status = 0;
	gf_dev->need_update = 0;

	/*setup gf configurations. */
	gf_debug(INFO_LOG, " Setting gf device configuration==========\n");

	/* Initialize the driver data */
	gf_dev->spi = spi;

	/* setup SPI parameters */
	/* CPOL=CPHA=0, speed 1MHz */
	gf_dev->spi->mode = SPI_MODE_0;
	gf_dev->spi->bits_per_word = 8;
	gf_dev->spi->max_speed_hz = 1 * 1000 * 1000;
	gf_dev->irq = 0;

	/* allocate buffer for SPI transfer */
	gf_dev->spi_buffer = kzalloc(bufsiz, GFP_KERNEL);
	if (gf_dev->spi_buffer == NULL) {
		status = -ENOMEM;
		goto err_buf;
	}

	buck = regulator_get(NULL, "irtx_ldo");
	if (buck == NULL) {
		gf_debug(INFO_LOG, "regulator_get fail");
		goto err_buf;
	}
	status = regulator_set_voltage(buck, 2800000, 2800000);
	if (status < 0) {
		goto err_buf;
	}
	status = regulator_enable(buck);
	if (status < 0) {
		gf_debug(ERR_LOG, "%s, regulator_enable fail!!\n", __func__);
		goto err_buf;
	}

	/* get gpio info from dts or defination */
	gf_get_gpio_dts_info(gf_dev);
	gf_get_sensor_dts_info();

	/*enable the power */
	pr_err("%s %d now get dts info done!", __func__, __LINE__);
	gf_hw_power_enable(gf_dev, 1);

	pr_err("%s %d now enable spi clk API", __func__, __LINE__);
	gf_spi_clk_enable(gf_dev, 1);

	pr_err("%s %d now enable spi clk Done", __func__, __LINE__);

	mdelay(1);
	gf_spi_read_bytes(gf_dev, 0x0000, 4, rx_test);
	printk("%s rx_test chip id:0x%x 0x%x 0x%x 0x%x \n", __func__,
	       rx_test[0], rx_test[1], rx_test[2], rx_test[3]);

	if ((rx_test[0] != 0x0f && rx_test[0] != 0x0d) || (rx_test[3] != 0x22)) {
		goodix_fp_exist = false;
		gf_debug(ERR_LOG,
		         "%s, get goodix FP sensor chipID fail!!\n",
		         __func__);

		pr_err("%s cannot find the sensor,now exit\n",
		       __func__);
		spi_fingerprint = spi;
		gf_hw_power_enable(gf_dev, 0);
		gf_spi_clk_enable(gf_dev, 0);
		kfree(gf_dev->spi_buffer);
		mutex_destroy(&gf_dev->buf_lock);
		mutex_destroy(&gf_dev->release_lock);
		spi_set_drvdata(spi, NULL);
		gf_dev->spi = NULL;
		kfree(gf_dev);
		gf_dev = NULL;
		return 0;
	} else {
		goodix_fp_exist = true;
		memcpy(&uuid_fp, &uuid_ta_gf, sizeof(struct TEEC_UUID));
		pr_err("%s %d Goodix fingerprint sensor detected\n",
		       __func__, __LINE__);
		gf_debug(ERR_LOG, "[gf][goodix_test] %s, get chipid\n",
		         __func__);
	}

	/* create class */
	gf_dev->class = class_create(THIS_MODULE, "goodix_fp");
	if (IS_ERR(gf_dev->class)) {
		gf_debug(ERR_LOG, "%s, Failed to create class.\n", __func__);
		status = -ENODEV;
		goto err_class;
	}

	status = alloc_chrdev_region(&gf_dev->devno, gf_dev->device_count++, 1, GF_DEV_NAME);
	if (status < 0) {
		gf_debug(ERR_LOG, "%s, Failed to alloc devno.\n", __func__);
		goto err_devno;
	} else {
		gf_debug(INFO_LOG, "%s, major=%d, minor=%d\n", __func__,
		         MAJOR(gf_dev->devno), MINOR(gf_dev->devno));
	}

	/* create device */
	gf_dev->device =
		device_create(gf_dev->class, &spi->dev, gf_dev->devno, gf_dev,
		              GF_DEV_NAME);
	if (IS_ERR(gf_dev->device)) {
		gf_debug(ERR_LOG, "%s, Failed to create device.\n", __func__);
		status = -ENODEV;
		goto err_device;
	} else {
		mutex_lock(&device_list_lock);
		list_add(&gf_dev->device_entry, &device_list);
		mutex_unlock(&device_list_lock);
		gf_debug(INFO_LOG, "%s, device create success.\n", __func__);
	}

	/* create sysfs */
	status = sysfs_create_group(&spi->dev.kobj, &gf_debug_attr_group);
	if (status) {
		gf_debug(ERR_LOG, "%s, Failed to create sysfs file.\n",
		         __func__);
		status = -ENODEV;
		goto err_sysfs;
	} else {
		gf_debug(INFO_LOG, "%s, Success create sysfs file.\n",
		         __func__);
	}

	/* cdev init and add */
	cdev_init(&gf_dev->cdev, &gf_fops);
	gf_dev->cdev.owner = THIS_MODULE;
	status = cdev_add(&gf_dev->cdev, gf_dev->devno, 1);
	if (status) {
		gf_debug(ERR_LOG, "%s, Failed to add cdev.\n", __func__);
		goto err_cdev;
	}

	/*register device within input system. */
	gf_dev->input = input_allocate_device();
	if (gf_dev->input == NULL) {
		gf_debug(ERR_LOG, "%s, Failed to allocate input device.\n",
		         __func__);
		status = -ENOMEM;
		goto err_input;
	}

	__set_bit(EV_KEY, gf_dev->input->evbit);
#if INPUT_KEY_EVENTS
	__set_bit(GF_KEY_INPUT_HOME, gf_dev->input->keybit);

	__set_bit(GF_KEY_INPUT_MENU, gf_dev->input->keybit);
	__set_bit(GF_KEY_INPUT_BACK, gf_dev->input->keybit);
	__set_bit(GF_KEY_INPUT_POWER, gf_dev->input->keybit);
	__set_bit(GF_KEY_INPUT_CAMERA, gf_dev->input->keybit);
#endif

	__set_bit(GF_NAV_INPUT_UP, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_DOWN, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_RIGHT, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_LEFT, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_CLICK, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_DOUBLE_CLICK, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_LONG_PRESS, gf_dev->input->keybit);
	__set_bit(GF_NAV_INPUT_HEAVY, gf_dev->input->keybit);

	gf_dev->input->name = "uinput-goodix";
	if (input_register_device(gf_dev->input)) {
		gf_debug(ERR_LOG, "%s, Failed to register input device.\n",
		         __func__);
		status = -ENODEV;
		goto err_input_2;
	}

	status = gf_netlink_init(gf_dev);
	if (status == -1) {
		mutex_lock(&gf_dev->release_lock);
		input_unregister_device(gf_dev->input);
		gf_dev->input = NULL;
		mutex_unlock(&gf_dev->release_lock);
		goto err_input;
	}

	wakeup_source_init(&fp_wakesrc, "fp_wakesrc");

	gf_dev->probe_finish = 1;
	gf_dev->is_sleep_mode = 0;
	gf_debug(INFO_LOG, "%s probe finished\n", __func__);
	pr_err("%s %d now disable spi clk API", __func__, __LINE__);
	gf_spi_clk_enable(gf_dev, 0);

	gf_debug(ERR_LOG, "[gf][goodix_test] %s, probe success\n", __func__);

	return 0;

err_input_2:
	mutex_lock(&gf_dev->release_lock);
	input_free_device(gf_dev->input);
	gf_dev->input = NULL;
	mutex_unlock(&gf_dev->release_lock);

err_input:
	cdev_del(&gf_dev->cdev);

err_cdev:
	sysfs_remove_group(&spi->dev.kobj, &gf_debug_attr_group);

err_sysfs:
	device_destroy(gf_dev->class, gf_dev->devno);
	list_del(&gf_dev->device_entry);

err_device:
	unregister_chrdev_region(gf_dev->devno, 1);

err_devno:
	class_destroy(gf_dev->class);

err_class:
	pr_err("%s cannot find the sensor,now exit\n", __func__);
	gf_hw_power_enable(gf_dev, 0);
	gf_spi_clk_enable(gf_dev, 0);
	kfree(gf_dev->spi_buffer);

err_buf:
	mutex_destroy(&gf_dev->buf_lock);
	mutex_destroy(&gf_dev->release_lock);

	gf_dev->spi = NULL;
	kfree(gf_dev);
	gf_dev = NULL;
err:
	gf_debug(ERR_LOG, "[gf][goodix_test] %s, probe fail\n", __func__);

	return status;
}

static int gf_remove(struct spi_device *spi) {
	struct gf_device *gf_dev = spi_get_drvdata(spi);

	wakeup_source_trash(&fp_wakesrc);

	if (gf_dev->irq) {
		free_irq(gf_dev->irq, gf_dev);
		gf_dev->irq_count = 0;
		gf_dev->irq = 0;
	}

	fb_unregister_client(&gf_dev->notifier);

	mutex_lock(&gf_dev->release_lock);
	if (gf_dev->input == NULL) {
		kfree(gf_dev);
		mutex_unlock(&gf_dev->release_lock);
		return 0;
	}

	input_unregister_device(gf_dev->input);
	gf_dev->input = NULL;
	mutex_unlock(&gf_dev->release_lock);

	mutex_lock(&gf_dev->release_lock);
	if (gf_dev->spi_buffer != NULL) {
		kfree(gf_dev->spi_buffer);
		gf_dev->spi_buffer = NULL;
	}
	mutex_unlock(&gf_dev->release_lock);

	gf_netlink_destroy(gf_dev);
	cdev_del(&gf_dev->cdev);
	sysfs_remove_group(&spi->dev.kobj, &gf_debug_attr_group);
	device_destroy(gf_dev->class, gf_dev->devno);
	list_del(&gf_dev->device_entry);

	unregister_chrdev_region(gf_dev->devno, 1);
	class_destroy(gf_dev->class);
	gf_hw_power_enable(gf_dev, 0);
	gf_spi_clk_enable(gf_dev, 0);

	spin_lock_irq(&gf_dev->spi_lock);
	spi_set_drvdata(spi, NULL);
	gf_dev->spi = NULL;
	spin_unlock_irq(&gf_dev->spi_lock);

	mutex_destroy(&gf_dev->buf_lock);
	mutex_destroy(&gf_dev->release_lock);

	kfree(gf_dev);

	return 0;
}

static int __init gf_init(void) {
	int status;

	pr_err("%s %d\n", __func__, __LINE__);

	status = spi_register_driver(&gf_spi_driver);
	if (status < 0) {
		gf_debug(ERR_LOG, "%s, Failed to register SPI driver.\n", __func__);
		return -EINVAL;
	}

	return status;
}
late_initcall(gf_init);

static void __exit gf_exit(void) {
	spi_unregister_driver(&gf_spi_driver);
}
module_exit(gf_exit);

MODULE_AUTHOR("goodix");
MODULE_DESCRIPTION("Goodix Fingerprint chip GF316M/GF318M/GF3118M/GF518M/GF5118M/GF516M/GF816M/GF3208/GF5206/GF5216/GF5208 TEE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:gf_spi");
