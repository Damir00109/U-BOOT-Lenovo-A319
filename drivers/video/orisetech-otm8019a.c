// SPDX-License-Identifier: GPL-2.0+

#include <backlight.h>
#include <dm.h>
#include <panel.h>
#include <log.h>
#include <mipi_dsi.h>
#include <linux/delay.h>
#include <power/regulator.h>
#include <asm/gpio.h>

struct otm8019a_priv {
	struct udevice *vcc;

	struct udevice *backlight;

	struct gpio_desc enable_gpio;
	struct gpio_desc reset_gpio;
};

static struct display_timing default_timing = {
	.pixelclock.typ		= (480 + 37 + 6 + 37) * (800 + 8 + 8 + 8) * 60,
	.hactive.typ		= 480,
	.hfront_porch.typ	= 37,
	.hback_porch.typ	= 37,
	.hsync_len.typ		= 6,
	.vactive.typ		= 800,
	.vfront_porch.typ	= 8,
	.vback_porch.typ	= 8,
	.vsync_len.typ		= 8,
};

#define dsi_generic_write_seq(dsi, cmd, seq...) do {			\
		static const u8 b[] = { cmd, seq };			\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, b, ARRAY_SIZE(b));	\
		if (ret < 0)						\
			return log_ret(ret);				\
	} while (0)

/* Command stream decoded from the stock LK image's OTM8019A table. */
struct otm8019a_init_cmd {
	u8 cmd;
	u8 len;
	u8 data[20];
};

#define OTM8019A_CMD(_cmd, ...) \
	{ .cmd = (_cmd), .len = sizeof((u8[]){ __VA_ARGS__ }), \
	  .data = { __VA_ARGS__ } }

static const struct otm8019a_init_cmd otm8019a_init_seq[] = {
	OTM8019A_CMD(0xff, 0x80, 0x19, 0x01),
	OTM8019A_CMD(0x00, 0x80),
	OTM8019A_CMD(0xff, 0x80, 0x19),
	OTM8019A_CMD(0x00, 0x8a),
	OTM8019A_CMD(0xc4, 0x40),
	OTM8019A_CMD(0x00, 0xa6),
	OTM8019A_CMD(0xb3, 0x20, 0x01),
	OTM8019A_CMD(0x00, 0x90),
	OTM8019A_CMD(0xc0, 0x00, 0x15, 0x00, 0x00, 0x00, 0x03),
	OTM8019A_CMD(0x00, 0xb4),
	OTM8019A_CMD(0xc0, 0x00),
	OTM8019A_CMD(0x00, 0x81),
	OTM8019A_CMD(0xc1, 0x33),
	OTM8019A_CMD(0x00, 0x81),
	OTM8019A_CMD(0xc4, 0x81),
	OTM8019A_CMD(0x00, 0x87),
	OTM8019A_CMD(0xc4, 0x00),
	OTM8019A_CMD(0x00, 0x89),
	OTM8019A_CMD(0xc4, 0x00),
	OTM8019A_CMD(0x00, 0x82),
	OTM8019A_CMD(0xc5, 0xb0),
	OTM8019A_CMD(0x00, 0x90),
	OTM8019A_CMD(0xc5, 0x6e, 0xdb, 0x06, 0x91, 0x44, 0x44, 0x23),
	OTM8019A_CMD(0x00, 0xb1),
	OTM8019A_CMD(0xc5, 0xa8),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0xd8, 0x6f, 0x6f),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0xd9, 0x64),
	OTM8019A_CMD(0x00, 0x80),
	OTM8019A_CMD(0xce, 0x86, 0x01, 0x00, 0x85, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xa0),
	OTM8019A_CMD(0xce, 0x18, 0x05, 0x83, 0x39, 0x00, 0x00, 0x00, 0x18, 0x04, 0x83, 0x3a, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xb0),
	OTM8019A_CMD(0xce, 0x18, 0x03, 0x83, 0x3b, 0x86, 0x00, 0x00, 0x18, 0x02, 0x83, 0x3c, 0x88, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xc0),
	OTM8019A_CMD(0xcf, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xd0),
	OTM8019A_CMD(0xcf, 0x00),
	OTM8019A_CMD(0x00, 0xc0),
	OTM8019A_CMD(0xcb, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xd0),
	OTM8019A_CMD(0xcb, 0x00),
	OTM8019A_CMD(0x00, 0xd5),
	OTM8019A_CMD(0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xe0),
	OTM8019A_CMD(0xcb, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00),
	OTM8019A_CMD(0x00, 0x80),
	OTM8019A_CMD(0xcc, 0x00, 0x26, 0x09, 0x0b, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0x90),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0x9a),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xa0),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x02, 0x0c, 0x0a, 0x26, 0x00),
	OTM8019A_CMD(0x00, 0xb0),
	OTM8019A_CMD(0xcc, 0x00, 0x25, 0x0c, 0x0a, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xc0),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xca),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xd0),
	OTM8019A_CMD(0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x01, 0x09, 0x0b, 0x25, 0x00),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0xe1, 0x00, 0x0e, 0x1e, 0x36, 0x4e, 0x65, 0x6e, 0x9c, 0x89, 0x9d, 0x6c, 0x5d, 0x76, 0x64, 0x6a, 0x64, 0x5b, 0x53, 0x47, 0x00),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0xe2, 0x00, 0x0e, 0x1e, 0x36, 0x4e, 0x65, 0x6e, 0x9c, 0x89, 0x9d, 0x6b, 0x5c, 0x76, 0x64, 0x6a, 0x64, 0x5b, 0x53, 0x46, 0x00),
	OTM8019A_CMD(0x00, 0x98),
	OTM8019A_CMD(0xc0, 0x00),
	OTM8019A_CMD(0x00, 0xa9),
	OTM8019A_CMD(0xc0, 0x0a),
	OTM8019A_CMD(0x00, 0xb0),
	OTM8019A_CMD(0xc1, 0x20, 0x00, 0x00),
	OTM8019A_CMD(0x00, 0xe1),
	OTM8019A_CMD(0xc0, 0x40, 0x30),
	OTM8019A_CMD(0x00, 0x80),
	OTM8019A_CMD(0xc4, 0x30),
	OTM8019A_CMD(0x00, 0x80),
	OTM8019A_CMD(0xc1, 0x03, 0x33),
	OTM8019A_CMD(0x00, 0xa0),
	OTM8019A_CMD(0xc1, 0xe8),
	OTM8019A_CMD(0x00, 0x90),
	OTM8019A_CMD(0xb6, 0xb4),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0xff, 0xff, 0xff, 0xff),
	OTM8019A_CMD(0x00, 0x00),
	OTM8019A_CMD(0x3a, 0x77),
};

static int otm8019a_write_init_sequence(struct mipi_dsi_device *dsi)
{
	u8 packet[21];
	size_t pos;
	int ret;

	for (pos = 0; pos < ARRAY_SIZE(otm8019a_init_seq); pos++) {
		const struct otm8019a_init_cmd *entry = &otm8019a_init_seq[pos];

		packet[0] = entry->cmd;
		memcpy(packet + 1, entry->data, entry->len);
		ret = mipi_dsi_dcs_write_buffer(dsi, packet, entry->len + 1);
		if (ret < 0)
			return log_ret(ret);
	}

	return 0;
}

static int otm8019a_enable_backlight(struct udevice *dev)
{
	struct otm8019a_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	int ret;

	ret = otm8019a_write_init_sequence(dsi);
	if (ret)
		return ret;

	dsi_generic_write_seq(dsi, 0x11, 0x00);
	mdelay(120);
	dsi_generic_write_seq(dsi, 0x29, 0x00);
	mdelay(50);

	ret = backlight_enable(priv->backlight);
	if (ret)
		return log_ret(ret);

	return backlight_set_brightness(priv->backlight, BACKLIGHT_DEFAULT);
}

static int otm8019a_set_backlight(struct udevice *dev, int percent)
{
	struct otm8019a_priv *priv = dev_get_priv(dev);
	int ret;

	ret = backlight_enable(priv->backlight);
	if (ret)
		return log_ret(ret);

	return backlight_set_brightness(priv->backlight, percent);
}

static int otm8019a_timings(struct udevice *dev, struct display_timing *timing)
{
	memcpy(timing, &default_timing, sizeof(*timing));
	return 0;
}

static int otm8019a_of_to_plat(struct udevice *dev)
{
	struct otm8019a_priv *priv = dev_get_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret) {
		log_debug("%s: cannot get backlight: ret = %d\n",
			  __func__, ret);
		return log_ret(ret);
	}

	ret = device_get_supply_regulator(dev, "vcc-supply", &priv->vcc);
	if (ret) {
		log_debug("%s: cannot get vcc-supply: ret = %d\n",
			  __func__, ret);
		return log_ret(ret);
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->enable_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_debug("%s: cannot decode enable-gpios (%d)\n",
			  __func__, ret);
		return log_ret(ret);
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_debug("%s: cannot decode reset-gpios (%d)\n",
			  __func__, ret);
		return log_ret(ret);
	}

	return 0;
}

static int otm8019a_hw_init(struct udevice *dev)
{
	struct otm8019a_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dm_gpio_set_value(&priv->reset_gpio, 0);
	if (ret)
		return log_ret(ret);

	ret = regulator_set_enable_if_allowed(priv->vcc, true);
	if (ret)
		return log_ret(ret);
	mdelay(20);

	ret = dm_gpio_set_value(&priv->enable_gpio, 1);
	if (ret)
		return log_ret(ret);
	mdelay(20);

	ret = dm_gpio_set_value(&priv->reset_gpio, 1);
	if (ret)
		return log_ret(ret);

	ret = dm_gpio_set_value(&priv->reset_gpio, 0);
	if (ret)
		return log_ret(ret);

	mdelay(10);

	ret = dm_gpio_set_value(&priv->reset_gpio, 1);
	if (ret)
		return log_ret(ret);

	mdelay(50);

	return 0;
}

static int otm8019a_probe(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	int ret;

	/* fill characteristics of DSI data link */
	plat->lanes = 2;
	plat->format = MIPI_DSI_FMT_RGB888;
	plat->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			   MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	if (plat->device) {
		plat->device->lanes = plat->lanes;
		plat->device->format = plat->format;
		plat->device->mode_flags = plat->mode_flags;

		ret = mipi_dsi_attach(plat->device);
		if (ret)
			return log_ret(ret);
	}

	return otm8019a_hw_init(dev);
}

static const struct panel_ops otm8019a_ops = {
	.enable_backlight	= otm8019a_enable_backlight,
	.set_backlight		= otm8019a_set_backlight,
	.get_display_timing	= otm8019a_timings,
};

static const struct udevice_id otm8019a_ids[] = {
	{ .compatible = "djn,otm8019a" },
	{ }
};

U_BOOT_DRIVER(otm8019a) = {
	.name		= "otm8019a",
	.id		= UCLASS_PANEL,
	.of_match	= otm8019a_ids,
	.ops		= &otm8019a_ops,
	.of_to_plat	= otm8019a_of_to_plat,
	.probe		= otm8019a_probe,
	.plat_auto	= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct otm8019a_priv),
};
