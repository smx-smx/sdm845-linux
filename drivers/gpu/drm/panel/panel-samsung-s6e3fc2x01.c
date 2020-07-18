// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020, The Linux Foundation. All rights reserved.

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>

struct samsung_s6e3fc2x01 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	struct backlight_device *backlight;
	bool prepared;
};

static inline
struct samsung_s6e3fc2x01 *to_samsung_s6e3fc2x01(struct drm_panel *panel)
{
	return container_of(panel, struct samsung_s6e3fc2x01, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void samsung_s6e3fc2x01_reset(struct samsung_s6e3fc2x01 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
}

static int samsung_s6e3fc2x01_on(struct samsung_s6e3fc2x01 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0x9f, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	dsi_dcs_write_seq(dsi, 0x9f, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x01);
	dsi_dcs_write_seq(dsi, 0xcd, 0x01);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	usleep_range(15000, 16000);
	dsi_dcs_write_seq(dsi, 0x9f, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0x9f, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xeb, 0x17, 0x41, 0x92, 0x0e, 0x10, 0x82, 0x5a);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_column_address(dsi, 0x0000, 0x0437);
	if (ret < 0) {
		dev_err(dev, "Failed to set column address: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_page_address(dsi, 0x0000, 0x0923);
	if (ret < 0) {
		dev_err(dev, "Failed to set page address: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x09);
	dsi_dcs_write_seq(dsi, 0xe8, 0x10, 0x30);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x07);
	dsi_dcs_write_seq(dsi, 0xb7, 0x01);
	dsi_dcs_write_seq(dsi, 0xb0, 0x08);
	dsi_dcs_write_seq(dsi, 0xb7, 0x12);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x01);
	dsi_dcs_write_seq(dsi, 0xe3, 0x88);
	dsi_dcs_write_seq(dsi, 0xb0, 0x07);
	dsi_dcs_write_seq(dsi, 0xed, 0x67);
	dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	usleep_range(1000, 2000);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb3, 0x00, 0xc1);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	return 0;
}

static int samsung_s6e3fc2x01_off(struct samsung_s6e3fc2x01 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi_dcs_write_seq(dsi, 0x9f, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	usleep_range(16000, 17000);
	dsi_dcs_write_seq(dsi, 0xb0, 0x50);
	dsi_dcs_write_seq(dsi, 0xb9, 0x82);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	usleep_range(16000, 17000);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}

	dsi_dcs_write_seq(dsi, 0x9f, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_dcs_write_seq(dsi, 0xb0, 0x05);
	dsi_dcs_write_seq(dsi, 0xf4, 0x01);
	dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	msleep(150);

	return 0;
}

static int samsung_s6e3fc2x01_prepare(struct drm_panel *panel)
{
	struct samsung_s6e3fc2x01 *ctx = to_samsung_s6e3fc2x01(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	samsung_s6e3fc2x01_reset(ctx);

	ret = samsung_s6e3fc2x01_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int samsung_s6e3fc2x01_unprepare(struct drm_panel *panel)
{
	struct samsung_s6e3fc2x01 *ctx = to_samsung_s6e3fc2x01(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = samsung_s6e3fc2x01_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode samsung_s6e3fc2x01_mode = {
	.clock = (1080 + 72 + 16 + 36) * (2340 + 32 + 4 + 18) * 70 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 72,
	.hsync_end = 1080 + 72 + 16,
	.htotal = 1080 + 72 + 16 + 36,
	.vdisplay = 2340,
	.vsync_start = 2340 + 32,
	.vsync_end = 2340 + 32 + 4,
	.vtotal = 2340 + 32 + 4 + 18,
	.vrefresh = 60,
	.width_mm = 68,
	.height_mm = 145,
};

static int samsung_s6e3fc2x01_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &samsung_s6e3fc2x01_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs samsung_s6e3fc2x01_panel_funcs = {
	.prepare = samsung_s6e3fc2x01_prepare,
	.unprepare = samsung_s6e3fc2x01_unprepare,
	.get_modes = samsung_s6e3fc2x01_get_modes,
};

static int samsung_s6e3fc2x01_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;
	u16 brightness = bl->props.brightness;

	err = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (err < 0)
		 return err;

	return brightness & 0xff;
}

static int samsung_s6e3fc2x01_bl_update_status(struct backlight_device *bl)
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

static const struct backlight_ops samsung_s6e3fc2x01_bl_ops = {
	.update_status = samsung_s6e3fc2x01_bl_update_status,
	.get_brightness = samsung_s6e3fc2x01_bl_get_brightness,
};

static struct backlight_device *
samsung_s6e3fc2x01_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props = {
		.type = BACKLIGHT_PLATFORM,
		.scale = BACKLIGHT_SCALE_LINEAR,
		.brightness = 255,
		.max_brightness = 512,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
							&samsung_s6e3fc2x01_bl_ops, &props);
}

static int samsung_s6e3fc2x01_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct samsung_s6e3fc2x01 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_err(dev, "Failed to get reset-gpios: %d\n", ret);
		return ret;
	}

	 ctx->backlight = samsung_s6e3fc2x01_create_backlight(dsi);
	if (IS_ERR(ctx->backlight)) {
		ret = PTR_ERR(ctx->backlight);
		dev_err(dev, "Failed to create backlight: %d\n", ret);
		return ret;
	}

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &samsung_s6e3fc2x01_panel_funcs,
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

	return 0;
}

static int samsung_s6e3fc2x01_remove(struct mipi_dsi_device *dsi)
{
	struct samsung_s6e3fc2x01 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id samsung_s6e3fc2x01_of_match[] = {
	{ .compatible = "samsung,s6e3fc2x01" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, samsung_s6e3fc2x01_of_match);

static struct mipi_dsi_driver samsung_s6e3fc2x01_driver = {
	.probe = samsung_s6e3fc2x01_probe,
	.remove = samsung_s6e3fc2x01_remove,
	.driver = {
		.name = "panel-samsung-s6e3fc2x01",
		.of_match_table = samsung_s6e3fc2x01_of_match,
	},
};
module_mipi_dsi_driver(samsung_s6e3fc2x01_driver);

MODULE_DESCRIPTION("DRM driver for samsung s6e3fc2x01 cmd mode dsi panel");
MODULE_LICENSE("GPL v2");
