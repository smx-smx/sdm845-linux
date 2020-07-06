// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2010, 2011, 2016 The Linux Foundation. All rights reserved.
 */
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/regmap.h>

#define PM8998_LED_TYPE_RED    0x0
#define PM8998_LED_TYPE_GREEN  0x1
#define PM8998_LED_TYPE_BLUE   0x2

#define PM8998_LED_MASK_RED   0x80
#define PM8998_LED_MASK_GREEN 0x40
#define PM8998_LED_MASK_BLUE  0x20

// Source select ? always written as 1 on downstream - could be which PWM device
#define PM8998_LED_REG_SRC_SELECT(reg)		(reg + 0x45)
// Enable register, 0x20 for pulse, 0x40 for constant brightness
#define PM8998_LED_REG_ENABLE(reg)			(reg + 0x46)
// Enables auto trickle charging, always written on boot
#define PM8998_LED_REG_ATC(reg)				(reg + 0x47)

struct pm8998_led {
	struct regmap *map;
	struct led_classdev cdev;
	u32 reg;
};

static void pm8998_led_set(struct led_classdev *cled,
	enum led_brightness value)
{
	struct pm8998_led *led;
	int ret = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	pr_info("Setting brightness to: %d, reg: 0x%x", value, PM8998_LED_REG_ENABLE(led->reg));

	led = container_of(cled, struct pm8998_led, cdev);
	mask = 0x80;
	val = 0x40;

	ret = regmap_update_bits(led->map, PM8998_LED_REG_ENABLE(led->reg), mask, val);
	if (ret)
		pr_err("Failed to set LED brightness\n");
}

static enum led_brightness pm8998_led_get(struct led_classdev *cled)
{
	struct pm8998_led *led;
	int ret;
	unsigned int val;

	led = container_of(cled, struct pm8998_led, cdev);

	// ret = regmap_read(led->map, led->reg, &val);
	// if (ret) {
	// 	pr_err("Failed to get LED brightness\n");
	// 	return LED_OFF;
	// }

	return led->cdev.brightness;

	//return val;
}

static int pm8998_led_init(struct pm8998_led *led)
{
	unsigned int mask = 0;
	unsigned int val = 0;
	int ret = 0;

	// Src select
	mask = 0x03;
	val = 0x01;
	ret = regmap_update_bits(led->map, PM8998_LED_REG_SRC_SELECT(led->reg), mask, val);
	if (ret)
	{
		pr_err("Failed to src_select");
		return ret;
	}
	msleep(50);

	// Auto trickle charge enable
	mask = 0x80; //TODO: Make this dependant on LED data!
	val = 0x80;
	ret = regmap_update_bits(led->map, PM8998_LED_REG_ATC(led->reg), mask, val);
	if (ret)
	{
		pr_err("Failed to enable ATC");
		return ret;
	}
	msleep(50);

	// Set LED off
	mask = 0x80; //TODO: Make this dependant on LED data!
	val = 0x00;
	regmap_update_bits(led->map, PM8998_LED_REG_ENABLE(led->reg), mask, val);
	if (ret)
	{
		pr_err("Failed to reg_enable");
		return ret;
	}
	msleep(50);

	return ret;
}

static int pm8998_led_probe(struct platform_device *pdev)
{
	struct pm8998_led *led;
	struct device_node *np = pdev->dev.of_node;
	int ret;
	struct regmap *map;
	const char *state;
	enum led_brightness maxbright;

	led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	map = dev_get_regmap(pdev->dev.parent, NULL);
	if (!map) {
		dev_err(&pdev->dev, "Parent regmap unavailable.\n");
		return -ENXIO;
	}
	led->map = map;

	ret = of_property_read_u32(np, "reg", &led->reg);
	if (ret) {
		dev_err(&pdev->dev, "no register offset specified\n");
		return -EINVAL;
	}

	/* Use label else node name */
	led->cdev.name = of_get_property(np, "label", NULL) ? : np->name;
	led->cdev.default_trigger =
		of_get_property(np, "linux,default-trigger", NULL);
	led->cdev.brightness_set = pm8998_led_set;
	led->cdev.brightness_get = pm8998_led_get;
	led->cdev.max_brightness = 255;

	state = of_get_property(np, "default-state", NULL);
	if (state) {
		if (!strcmp(state, "keep")) {
			led->cdev.brightness = pm8998_led_get(&led->cdev);
		} else if (!strcmp(state, "on")) {
			led->cdev.brightness = maxbright;
			pm8998_led_set(&led->cdev, maxbright);
		} else {
			led->cdev.brightness = LED_OFF;
			pm8998_led_set(&led->cdev, LED_OFF);
		}
	}

	ret = devm_led_classdev_register(&pdev->dev, &led->cdev);
	if (ret) {
		dev_err(&pdev->dev, "unable to register led \"%s\"\n",
			led->cdev.name);
		return ret;
	}

	ret = pm8998_led_init(led);
	if (ret)
	{
		pr_err("Failed to init LED");
	}

	dev_info(&pdev->dev, "Successfully probed pm8998-led");

	return 0;
}

static const struct of_device_id pm8998_leds_id_table[] = {
	{
		.compatible = "qcom,pm8998-led",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, pm8998_leds_id_table);

static struct platform_driver pm8998_led_driver = {
	.probe		= pm8998_led_probe,
	.driver		= {
		.name	= "pm8998-leds",
		.of_match_table = pm8998_leds_id_table,
	},
};
module_platform_driver(pm8998_led_driver);

MODULE_DESCRIPTION("PM8998 LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pm8998-leds");
