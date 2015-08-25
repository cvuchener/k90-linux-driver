/*
 * HID driver for Corsair Vengeance K90 Keyboard
 * Copyright (c) 2015 Clément Vuchener
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/leds.h>

#define USB_VENDOR_ID_CORSAIR     0x1b1c
#define USB_DEVICE_ID_CORSAIR_K90 0x1b02

struct k90_led {
	struct led_classdev cdev;
	int brightness;
	struct work_struct work;
};

struct k90_profile {
	struct device *dev;
	int profile;
};

struct k90_drvdata {
	int current_profile;
	int macro_mode;
	int meta_locked;
	struct k90_led backlight;
	struct k90_led record_led;
	struct k90_profile profile[3];
};

#define K90_GKEY_COUNT	18

static int k90_usage_to_gkey(unsigned int usage) {
	/* G1 (0xd0) to G16 (0xdf) */
	if (usage >= 0xd0 && usage <= 0xdf) {
		return usage - 0xd0 + 1;
	}
	/* G17 (0xe8) to G18 (0xe9) */
	if (usage >= 0xe8 && usage <= 0xe9) {
		return usage - 0xe8 + 17;
	}
	return 0;
}

static unsigned short k90_gkey_map[K90_GKEY_COUNT] = {
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	BTN_MISC+0,
	BTN_MISC+1,
	BTN_MISC+2,
	BTN_MISC+3,
	BTN_MISC+4,
	BTN_MISC+5,
};
module_param_array_named(gkey_codes, k90_gkey_map, ushort, NULL, S_IRUGO);

#define K90_USAGE_SPECIAL_MIN 0xf0
#define K90_USAGE_SPECIAL_MAX 0xff

#define K90_USAGE_MACRO_RECORD_START 0xf6
#define K90_USAGE_MACRO_RECORD_STOP 0xf7

#define K90_USAGE_PROFILE 0xf1
#define K90_USAGE_M1 0xf1
#define K90_USAGE_M2 0xf2
#define K90_USAGE_M3 0xf3
#define K90_USAGE_PROFILE_MAX 0xf3

#define K90_USAGE_META_OFF 0xf4
#define K90_USAGE_META_ON  0xf5

#define K90_USAGE_LIGHT 0xfa
#define K90_USAGE_LIGHT_OFF 0xfa
#define K90_USAGE_LIGHT_DIM 0xfb
#define K90_USAGE_LIGHT_MEDIUM 0xfc
#define K90_USAGE_LIGHT_BRIGHT 0xfd
#define K90_USAGE_LIGHT_MAX 0xfd

/* USB control protocol */

#define K90_REQUEST_BRIGHTNESS 49
#define K90_REQUEST_MACRO_MODE 2
#define K90_REQUEST_STATUS 4
#define K90_REQUEST_GET_MODE 5
#define K90_REQUEST_PROFILE 20

#define K90_REQUEST_PROFILE_BINDINGS 16
#define K90_REQUEST_PROFILE_KEYS 22
#define K90_REQUEST_PROFILE_DATA 18

#define K90_BINDINGS_MAX_LENGTH 128
#define K90_KEYS_MAX_LENGTH 64
#define K90_DATA_MAX_LENGTH 4096 // May be higher but that is the maximum I tested

#define K90_MACRO_MODE_SW 0x0030
#define K90_MACRO_MODE_HW 0x0001

#define K90_MACRO_LED_ON  0x0020
#define K90_MACRO_LED_OFF 0x0040

/* 
 * LED class devices
 */

#define K90_BACKLIGHT_LED_SUFFIX ":blue:backlight"
#define K90_RECORD_LED_SUFFIX ":red:record"

static enum led_brightness k90_brightness_get(struct led_classdev *led_cdev)
{
	struct k90_led *led = container_of (led_cdev, struct k90_led, cdev);
	return led->brightness;
}

static void k90_brightness_set(struct led_classdev *led_cdev,
		enum led_brightness brightness)
{
	struct k90_led *led = container_of (led_cdev, struct k90_led, cdev);
	led->brightness = brightness;
	schedule_work (&led->work);
}

static void k90_backlight_work(struct work_struct *work) {
	int ret;
	struct k90_led *led = container_of (work, struct k90_led, work);
	struct device *dev = led->cdev.dev->parent;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_BRIGHTNESS,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			led->brightness, 0, NULL, 0, USB_CTRL_SET_TIMEOUT))) {
		printk (KERN_WARNING "Failed to set backlight brightness (error: %d).\n", ret);
	}
}

static void k90_record_led_work(struct work_struct *work) {
	int ret;
	struct k90_led *led = container_of (work, struct k90_led, work);
	struct device *dev = led->cdev.dev->parent;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int value;

	if (led->brightness > 0) {
		value = K90_MACRO_LED_ON;
	}
	else {
		value = K90_MACRO_LED_OFF;
	}

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_MACRO_MODE,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			value, 0, NULL, 0, USB_CTRL_SET_TIMEOUT))) {
		printk (KERN_WARNING "Failed to set record LED state (error: %d).\n", ret);
	}
}

/*
 * Keyboard attributes
 */

static ssize_t k90_show_macro_mode(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct k90_drvdata *drvdata = dev_get_drvdata (dev);
	if (!drvdata) {
		return -ENOSYS;
	}
	return snprintf (buf, PAGE_SIZE, "%s\n", (drvdata->macro_mode ? "HW" : "SW"));
}

static ssize_t k90_store_macro_mode(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	struct k90_drvdata *drvdata = dev_get_drvdata (dev);
	__u16 value;
	
	if (strncmp(buf, "SW", 2) == 0) {
		value = K90_MACRO_MODE_SW;
	}
	else if (strncmp(buf, "HW", 2) == 0) {
		value = K90_MACRO_MODE_HW;
	}
	else {
		return -EINVAL;
	}

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_MACRO_MODE,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			value, 0, NULL, 0, USB_CTRL_SET_TIMEOUT))) {
		return ret;
	}
	drvdata->macro_mode = (value == K90_MACRO_MODE_HW);

	return count;
}

static ssize_t k90_show_current_profile(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct k90_drvdata *drvdata = dev_get_drvdata (dev);
	if (!drvdata) {
		return -ENOSYS;
	}
	return snprintf (buf, PAGE_SIZE, "%d\n", drvdata->current_profile);
}

static ssize_t k90_store_current_profile(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	struct k90_drvdata *drvdata = dev_get_drvdata (dev);
	int profile;

	if (kstrtoint(buf, 10, &profile)) {
		return -EINVAL;
	}
	if (profile < 1 || profile > 3) {
		return -EINVAL;
	}

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_PROFILE,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			profile, 0, NULL, 0, USB_CTRL_SET_TIMEOUT))) {
		return ret;
	}
	drvdata->current_profile = profile;

	return count;
}

static DEVICE_ATTR(macro_mode, 0644, k90_show_macro_mode, k90_store_macro_mode);
static DEVICE_ATTR(current_profile, 0644, k90_show_current_profile, k90_store_current_profile);

static struct attribute *k90_attrs[] = {
	&dev_attr_macro_mode.attr,
	&dev_attr_current_profile.attr,
	NULL
};

static const struct attribute_group k90_attr_group = {
	.attrs = k90_attrs,
};

/*
 * Profile device class and attributes
 */

static struct class *k90_profile_class;

static ssize_t k90_profile_show_profile_number(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct k90_profile *data = dev_get_drvdata(dev);
	return snprintf (buf, PAGE_SIZE, "%d\n", data->profile);

}

static ssize_t k90_profile_write_bindings(struct file *fp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	int ret;
	struct device *pdev = container_of (kobj, struct device, kobj);
	struct k90_profile *data = dev_get_drvdata(pdev);
	struct usb_interface *usbif = to_usb_interface(pdev->parent->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	if (count > K90_BINDINGS_MAX_LENGTH)
		return -EMSGSIZE;

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_PROFILE_BINDINGS,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			0, data->profile, buf, count, USB_CTRL_SET_TIMEOUT))) {
		return ret;
	}
	return count;
}

static ssize_t k90_profile_write_keys(struct file *fp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	int ret;
	struct device *pdev = container_of (kobj, struct device, kobj);
	struct k90_profile *data = dev_get_drvdata(pdev);
	struct usb_interface *usbif = to_usb_interface(pdev->parent->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	if (count > K90_KEYS_MAX_LENGTH)
		return -EMSGSIZE;

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_PROFILE_KEYS,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			0, data->profile, buf, count, USB_CTRL_SET_TIMEOUT))) {
		return ret;
	}
	return count;
}

static ssize_t k90_profile_write_data(struct file *fp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	int ret;
	struct device *pdev = container_of (kobj, struct device, kobj);
	struct k90_profile *data = dev_get_drvdata(pdev);
	struct usb_interface *usbif = to_usb_interface(pdev->parent->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	if (count > K90_DATA_MAX_LENGTH)
		return -EMSGSIZE;

	if (0 != (ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
			K90_REQUEST_PROFILE_DATA,
			USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			0, data->profile, buf, count, USB_CTRL_SET_TIMEOUT))) {
		return ret;
	}
	return count;
}


static DEVICE_ATTR(profile_number, 0444, k90_profile_show_profile_number, NULL);
static BIN_ATTR(bindings, 0200, NULL, k90_profile_write_bindings, 0);
static BIN_ATTR(keys, 0200, NULL, k90_profile_write_keys, 0);
static BIN_ATTR(data, 0200, NULL, k90_profile_write_data, 0);

static struct attribute *k90_profile_attrs[] = {
	&dev_attr_profile_number.attr,
	NULL
};

static struct bin_attribute *k90_profile_bin_attrs[] = {
	&bin_attr_bindings,
	&bin_attr_keys,
	&bin_attr_data,
	NULL
};

static const struct attribute_group k90_profile_attr_group = {
	.attrs = k90_profile_attrs,
	.bin_attrs = k90_profile_bin_attrs,
};

static const struct attribute_group *k90_profile_attr_groups[] = {
	&k90_profile_attr_group,
	NULL
};

/*
 * Driver functions
 */

static int k90_init_special_functions(struct hid_device *dev)
{
	int ret, i;
	struct usb_interface *usbif = to_usb_interface(dev->dev.parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	char data[8];
	struct k90_drvdata *drvdata = kzalloc (sizeof (struct k90_drvdata), GFP_KERNEL);
	size_t name_sz;
	char *name;
	struct k90_led *led;

	if (!drvdata) {
		ret = -ENOMEM;
		goto fail_drvdata;
	}
	hid_set_drvdata (dev, drvdata);

	/* Get current status */
	if ((ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0),
			K90_REQUEST_STATUS,
			USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			0, 0, data, 8, USB_CTRL_SET_TIMEOUT)) < 0) {
		printk (KERN_WARNING "Failed to get K90 initial state (error %d).\n", ret);
		drvdata->backlight.brightness = 0;
		drvdata->current_profile = 1;
	}
	else {
		drvdata->backlight.brightness = data[4];
		drvdata->current_profile = data[7];
	}
	/* Get current mode */
	if ((ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0),
			K90_REQUEST_GET_MODE,
			USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE,
			0, 0, data, 2, USB_CTRL_SET_TIMEOUT)) < 0) {
		printk (KERN_WARNING "Failed to get K90 initial mode (error %d).\n", ret);
	}
	else {
		switch (data[0]) {
		case K90_MACRO_MODE_HW:
			drvdata->macro_mode = 1;
			break;
		case K90_MACRO_MODE_SW:
			drvdata->macro_mode = 0;
			break;
		default:
			printk (KERN_WARNING "K90 in unknown mode: %02x.\n", data[0]);
		}
	}
	/* Init LED device for backlight */
	name_sz = strlen (dev_name (&dev->dev)) + sizeof (K90_BACKLIGHT_LED_SUFFIX);
	name = devm_kzalloc (&dev->dev, name_sz, GFP_KERNEL);
	if (!name) {
		ret = -ENOMEM;
		goto fail_backlight;
	}
	snprintf (name, name_sz, "%s" K90_BACKLIGHT_LED_SUFFIX, dev_name (&dev->dev));
	led = &drvdata->backlight;
	led->cdev.name = name;
	led->cdev.max_brightness = 3;
	led->cdev.brightness_set = k90_brightness_set;
	led->cdev.brightness_get = k90_brightness_get;
	INIT_WORK (&led->work, k90_backlight_work);
	if (0 != (ret = led_classdev_register (&dev->dev, &led->cdev))) {
		goto fail_backlight;
	}
	/* Init LED device for record LED */
	name_sz = strlen (dev_name (&dev->dev)) + sizeof (K90_RECORD_LED_SUFFIX);
	name = devm_kzalloc (&dev->dev, name_sz, GFP_KERNEL);
	if (!name) {
		ret = -ENOMEM;
		goto fail_record_led;
	}
	snprintf (name, name_sz, "%s" K90_RECORD_LED_SUFFIX, dev_name (&dev->dev));
	led = &drvdata->record_led;
	led->cdev.name = name;
	led->cdev.max_brightness = 1;
	led->cdev.brightness_set = k90_brightness_set;
	led->cdev.brightness_get = k90_brightness_get;
	INIT_WORK (&led->work, k90_record_led_work);
	if (0 != (ret = led_classdev_register (&dev->dev, &led->cdev))) {
		goto fail_record_led;
	}

	/* Create profile devices */
	for (i = 0; i < 3; ++i) {
		drvdata->profile[i].profile = i+1;
		drvdata->profile[i].dev = device_create_with_groups (
				k90_profile_class, &dev->dev, 0,
				&drvdata->profile[i], k90_profile_attr_groups,
				"%s:profile%d", dev_name(&dev->dev), i+1);
		if (IS_ERR(drvdata->profile[i].dev)) {
			ret = PTR_ERR(drvdata->profile[i].dev);
			for (i = i-1; i >= 0; --i) {
				device_unregister (drvdata->profile[i].dev);
			}
			goto fail_profile;
		}
	}

	/* Init attributes */
	if (0 != (ret = sysfs_create_group(&dev->dev.kobj, &k90_attr_group))) {
		goto fail_sysfs;
	}

	return 0;

fail_sysfs:
	for (i = 0; i < 3; ++i) {
		device_unregister (drvdata->profile[i].dev);
	}
fail_profile:
	led_classdev_unregister (&drvdata->record_led.cdev);
	flush_work (&drvdata->record_led.work);
fail_record_led:
	led_classdev_unregister (&drvdata->backlight.cdev);
	flush_work (&drvdata->backlight.work);
fail_backlight:
	kfree (drvdata);
fail_drvdata:
	hid_set_drvdata (dev, NULL);
	return ret;
}

static void k90_cleanup_special_functions(struct hid_device *dev)
{
	int i;
	struct k90_drvdata *drvdata = hid_get_drvdata (dev);

	if (drvdata) {
		sysfs_remove_group(&dev->dev.kobj, &k90_attr_group);
		for (i = 0; i < 3; ++i) {
			device_unregister (drvdata->profile[i].dev);
		}
		led_classdev_unregister (&drvdata->record_led.cdev);
		led_classdev_unregister (&drvdata->backlight.cdev);
		flush_work (&drvdata->record_led.work);
		flush_work (&drvdata->backlight.work);
		kfree (drvdata);
	}
}

static int k90_probe(struct hid_device *dev, const struct hid_device_id *id)
{
	int ret;
	struct usb_interface *usbif = to_usb_interface(dev->dev.parent);

	if (0 != (ret = hid_parse(dev))) {
		hid_err(dev, "parse failed\n");
		return ret;
	}
	if (0 != (ret = hid_hw_start(dev, HID_CONNECT_DEFAULT))) {
		hid_err(dev, "hw start failed\n");
		return ret;
	}

	if (usbif->cur_altsetting->desc.bInterfaceNumber == 0) {
		if (0 != (ret = k90_init_special_functions(dev))) {
			printk (KERN_WARNING "Failed to initialize K90 special functions.\n");
		}
	}
	else {
		hid_set_drvdata (dev, NULL);
	}

	return 0;
}

static void k90_remove(struct hid_device *dev)
{
	struct usb_interface *usbif = to_usb_interface(dev->dev.parent);

	if (usbif->cur_altsetting->desc.bInterfaceNumber == 0) {
		k90_cleanup_special_functions(dev);
	}

	hid_hw_stop(dev);
}

static int k90_event(struct hid_device *dev, struct hid_field *field,
		struct hid_usage *usage, __s32 value)
{
	struct k90_drvdata* drvdata = hid_get_drvdata (dev);

	if (!drvdata)
		return 0;
	
	switch (usage->hid & HID_USAGE) {
	case K90_USAGE_MACRO_RECORD_START:
		drvdata->record_led.brightness = 1;
		break;
	case K90_USAGE_MACRO_RECORD_STOP:
		drvdata->record_led.brightness = 0;
		break;
	case K90_USAGE_M1:
	case K90_USAGE_M2:
	case K90_USAGE_M3:
		drvdata->current_profile = (usage->hid & HID_USAGE) - K90_USAGE_PROFILE + 1;
		break;
	case K90_USAGE_META_OFF:
		drvdata->meta_locked = 0;
		break;
	case K90_USAGE_META_ON:
		drvdata->meta_locked = 0;
		break;
	case K90_USAGE_LIGHT_OFF:
	case K90_USAGE_LIGHT_DIM:
	case K90_USAGE_LIGHT_MEDIUM:
	case K90_USAGE_LIGHT_BRIGHT:
		drvdata->backlight.brightness = (usage->hid & HID_USAGE) -
				K90_USAGE_LIGHT;
		break;
	default:
		break;
	}

	return 0;
}

static int k90_input_mapping(struct hid_device *dev, struct hid_input *input,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	int gkey;
	if ((gkey = k90_usage_to_gkey(usage->hid & HID_USAGE))) {
		hid_map_usage_clear(input, usage, bit, max, EV_KEY, k90_gkey_map[gkey-1]);
		return 1;
	}
	if ((usage->hid & HID_USAGE) >= K90_USAGE_SPECIAL_MIN &&
			(usage->hid & HID_USAGE) <= K90_USAGE_SPECIAL_MAX) {
		return -1;
	}
	return 0;
}

static const struct hid_device_id k90_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_CORSAIR, USB_DEVICE_ID_CORSAIR_K90) },
	{ }
};
MODULE_DEVICE_TABLE(hid, k90_devices);

static struct hid_driver k90_driver = {
	.name = "k90",
	.id_table = k90_devices,
	.probe = k90_probe,
	.event = k90_event,
	.remove = k90_remove,
	.input_mapping = k90_input_mapping,
};

static int __init k90_init(void)
{
	int ret;

	k90_profile_class = class_create(THIS_MODULE, "k90_profile");
	if (IS_ERR(k90_profile_class)) {
		return PTR_ERR(k90_profile_class);
	}

	if (0 != (ret = hid_register_driver(&k90_driver))) {
		class_destroy(k90_profile_class);
		return ret;
	}

	return 0;
}

static void k90_exit(void)
{
	hid_unregister_driver(&k90_driver);
	class_destroy(k90_profile_class);
}

module_init(k90_init);
module_exit(k90_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clément Vuchener");
MODULE_DESCRIPTION("HID driver for Corsair Vengeance K90 Keyboard");
