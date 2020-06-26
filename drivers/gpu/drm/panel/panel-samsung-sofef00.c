/* SPDX-License-Identifier: GPL-2.0-only
* Copyright (c) 2020 Caleb Connolly <caleb@connolly.tech>
* Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
*   Copyright (c) 2020, The Linux Foundation. All rights reserved.
*
* Caleb Connolly <caleb@connolly.tech>
*/

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>

struct samsung_sofef00 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct backlight_device *backlight;
	struct regulator *supply;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *enable_gpio;
	bool prepared;
	bool enabled;
};

static inline
struct samsung_sofef00 *to_samsung_sofef00(struct drm_panel *panel)
{
	return container_of(panel, struct samsung_sofef00, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void samsung_sofef00_reset(struct samsung_sofef00 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
}

static int samsung_sofef00_on(struct samsung_sofef00 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x07);
	dsi_dcs_write_seq(dsi, 0xb6, 0x12);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int samsung_sofef00_off(struct samsung_sofef00 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(40);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(160);

	return 0;
}

static int samsung_sofef00_prepare(struct drm_panel *panel)
{
	struct samsung_sofef00 *ctx = to_samsung_sofef00(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	samsung_sofef00_reset(ctx);

	ret = samsung_sofef00_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int samsung_sofef00_unprepare(struct drm_panel *panel)
{
	struct samsung_sofef00 *ctx = to_samsung_sofef00(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;
	
	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulator: %d\n", ret);
		return ret;
	}

	ret = samsung_sofef00_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	regulator_disable(ctx->supply);

	ctx->prepared = false;
	return 0;
}


static int samsung_sofef00_enable(struct drm_panel *panel)
{
	struct samsung_sofef00 *ctx = to_samsung_sofef00(panel);
	int ret;

	if (ctx->enabled)
		return 0;

	ret = backlight_enable(ctx->backlight);
	if (ret < 0) {
		dev_err(&ctx->dsi->dev, "Failed to enable backlight: %d\n", ret);
		return ret;
	}

	ctx->enabled = true;
	return 0;
}

static int samsung_sofef00_disable(struct drm_panel *panel)
{
	struct samsung_sofef00 *ctx = to_samsung_sofef00(panel);
	int ret;

	if (!ctx->enabled)
		return 0;

	ret = backlight_disable(ctx->backlight);
	if (ret < 0) {
		dev_err(&ctx->dsi->dev, "Failed to disable backlight: %d\n", ret);
		return ret;
	}

	ctx->enabled = false;
	return 0;
}


static const struct drm_display_mode samsung_sofef00_mode = {
	.clock = (1080 + 112 + 16 + 36) * (2280 + 36 + 8 + 12) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 112,
	.hsync_end = 1080 + 112 + 16,
	.htotal = 1080 + 112 + 16 + 36,
	.vdisplay = 2280,
	.vsync_start = 2280 + 36,
	.vsync_end = 2280 + 36 + 8,
	.vtotal = 2280 + 36 + 8 + 12,
	.vrefresh = 60,
	.width_mm = 68,
	.height_mm = 145,
};

static int samsung_sofef00_get_modes(struct drm_panel *panel,
				       struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &samsung_sofef00_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs samsung_sofef00_panel_funcs = {
	.disable = samsung_sofef00_disable,
	.enable = samsung_sofef00_enable,
	.prepare = samsung_sofef00_prepare,
	.unprepare = samsung_sofef00_unprepare,
	.get_modes = samsung_sofef00_get_modes,
};

static int samsung_sofef00_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;
	u16 brightness = bl->props.brightness;

	err = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (err < 0)
		 return err;

	return brightness & 0xff;
}

static int samsung_sofef00_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;

	// This panel needs the high and low bytes swapped for the brightness value
	u16 brightness = ((bl->props.brightness<<8)&0xff00)|((bl->props.brightness>>8)&0x00ff);

	err = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (err < 0)
		 return err;

	return 0;
}

static const struct backlight_ops samsung_sofef00_bl_ops = {
	.update_status = samsung_sofef00_bl_update_status,
	.get_brightness = samsung_sofef00_bl_get_brightness,
};

static struct backlight_device *
samsung_sofef00_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props = {
		.type = BACKLIGHT_PLATFORM,
		.scale = BACKLIGHT_SCALE_LINEAR,
		.brightness = 255,
		.max_brightness = 512,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &samsung_sofef00_bl_ops, &props);
}


static int samsung_sofef00_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct samsung_sofef00 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	
	ctx->supply = devm_regulator_get(dev, "vddio");
	if (IS_ERR(ctx->supply)) {
		ret = PTR_ERR(ctx->supply);
		dev_err(dev, "Failed to get vddio regulator: %d\n", ret);
		return ret;
	}

	ctx->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->enable_gpio)) {
		ret = PTR_ERR(ctx->enable_gpio);
		dev_warn(dev, "Failed to get enable-gpios: %d\n", ret);
		//return ret;
	}

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_warn(dev, "Failed to get reset-gpios: %d\n", ret);
		return ret;
	}

	ctx->backlight = samsung_sofef00_create_backlight(dsi);
	if (IS_ERR(ctx->backlight)) {
		ret = PTR_ERR(ctx->backlight);
		dev_err(dev, "Failed to create backlight: %d\n", ret);
		return ret;
	}

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
    
	drm_panel_init(&ctx->panel, dev, &samsung_sofef00_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0) {
		dev_err(dev, "Failed to add panel: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		return ret;
	}

	dev_info(dev, "Successfully added sofef00 panel");

	return 0;
}

static int samsung_sofef00_remove(struct mipi_dsi_device *dsi)
{
	struct samsung_sofef00 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id samsung_sofef00_of_match[] = {
	{ .compatible = "samsung,sofef00" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, samsung_sofef00_of_match);

static struct mipi_dsi_driver samsung_sofef00_driver = {
	.probe = samsung_sofef00_probe,
	.remove = samsung_sofef00_remove,
	.driver = {
		.name = "panel-samsung-sofef00",
		.of_match_table = samsung_sofef00_of_match,
	},
};
module_mipi_dsi_driver(samsung_sofef00_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <caleb@connolly.tech>"); 
MODULE_DESCRIPTION("DRM driver for samsung sofef00_m cmd mode dsi panel");
MODULE_LICENSE("GPL v2");
