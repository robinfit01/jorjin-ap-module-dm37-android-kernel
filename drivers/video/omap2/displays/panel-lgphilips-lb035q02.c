/*
 * LCD panel driver for LG.Philips LB035Q02
 *
 * Author: Steve Sakoman <steve@sakoman.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include <plat/display.h>

static struct spi_device	*spidev;

static struct omap_video_timings lb035q02_timings = {
	.x_res = 320,
	.y_res = 240,

	.pixel_clock	= 6500,

	.hsw		= 2,
	.hfp		= 20,
	.hbp		= 68,

	.vsw		= 2,
	.vfp		= 4,
	.vbp		= 18,
};

static int lb035q02_write_reg(u8 reg, u16 val)
{
	struct spi_message msg;
	struct spi_transfer index_xfer = {
		.len		= 3,
		.cs_change	= 1,
	};
	struct spi_transfer value_xfer = {
		.len		= 3,
	};
	u8	buffer[16];

	spi_message_init(&msg);

	/* register index */
	buffer[0] = 0x70;
	buffer[1] = 0x00;
	buffer[2] = reg & 0x7f;
	index_xfer.tx_buf = buffer;
	spi_message_add_tail(&index_xfer, &msg);

	/* register value */
	buffer[4] = 0x72;
	buffer[5] = val >> 8;
	buffer[6] = val;
	value_xfer.tx_buf = buffer + 4;
	spi_message_add_tail(&value_xfer, &msg);

	return spi_sync(spidev, &msg);
}

static int lb035q02_panel_power_on(struct omap_dss_device *dssdev)
{
	int r;

	if (dssdev->state == OMAP_DSS_DISPLAY_ACTIVE)
		return 0;

	r = omapdss_dpi_display_enable(dssdev);
	if (r)
		goto err0;

	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			goto err1;
	}

	/* Panel init sequence from page 28 of the spec */
	lb035q02_write_reg(0x01, 0x6300);
	lb035q02_write_reg(0x02, 0x0200);
	lb035q02_write_reg(0x03, 0x0177);
	lb035q02_write_reg(0x04, 0x04c7);
	lb035q02_write_reg(0x05, 0xffc0);
	lb035q02_write_reg(0x06, 0xe806);
	lb035q02_write_reg(0x0a, 0x4008);
	lb035q02_write_reg(0x0b, 0x0000);
	lb035q02_write_reg(0x0d, 0x0030);
	lb035q02_write_reg(0x0e, 0x2800);
	lb035q02_write_reg(0x0f, 0x0000);
	lb035q02_write_reg(0x16, 0x9f80);
	lb035q02_write_reg(0x17, 0x0a0f);
	lb035q02_write_reg(0x1e, 0x00c1);
	lb035q02_write_reg(0x30, 0x0300);
	lb035q02_write_reg(0x31, 0x0007);
	lb035q02_write_reg(0x32, 0x0000);
	lb035q02_write_reg(0x33, 0x0000);
	lb035q02_write_reg(0x34, 0x0707);
	lb035q02_write_reg(0x35, 0x0004);
	lb035q02_write_reg(0x36, 0x0302);
	lb035q02_write_reg(0x37, 0x0202);
	lb035q02_write_reg(0x3a, 0x0a0d);
	lb035q02_write_reg(0x3b, 0x0806);

	return 0;
err1:
	omapdss_dpi_display_disable(dssdev);
err0:
	return r;
}

static void lb035q02_panel_power_off(struct omap_dss_device *dssdev)
{
	if (dssdev->state != OMAP_DSS_DISPLAY_ACTIVE)
		return;

	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	omapdss_dpi_display_disable(dssdev);
}

static int lb035q02_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
		OMAP_DSS_LCD_IHS;
	dssdev->panel.timings = lb035q02_timings;

	return 0;
}

static void lb035q02_panel_remove(struct omap_dss_device *dssdev)
{
}

static int lb035q02_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = lb035q02_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void lb035q02_panel_disable(struct omap_dss_device *dssdev)
{
	lb035q02_panel_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int lb035q02_panel_suspend(struct omap_dss_device *dssdev)
{
	lb035q02_panel_disable(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	return 0;
}

static int lb035q02_panel_resume(struct omap_dss_device *dssdev)
{
	int r;

	r = lb035q02_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static struct omap_dss_driver lb035q02_driver = {
	.probe		= lb035q02_panel_probe,
	.remove		= lb035q02_panel_remove,

	.enable		= lb035q02_panel_enable,
	.disable	= lb035q02_panel_disable,
	.suspend	= lb035q02_panel_suspend,
	.resume		= lb035q02_panel_resume,

	.driver         = {
		.name   = "lgphilips_lb035q02_panel",
		.owner  = THIS_MODULE,
	},
};

static int __devinit lb035q02_panel_spi_probe(struct spi_device *spi)
{
	spidev = spi;
	return 0;
}

static int __devexit lb035q02_panel_spi_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver lb035q02_spi_driver = {
	.driver		= {
		.name	= "lgphilips_lb035q02_panel-spi",
		.owner	= THIS_MODULE,
	},
	.probe		= lb035q02_panel_spi_probe,
	.remove		= __devexit_p (lb035q02_panel_spi_remove),
};

static int __init lb035q02_panel_drv_init(void)
{
	int r;
	r = spi_register_driver(&lb035q02_spi_driver);
	if (r != 0)
		pr_err("lgphilips_lb035q02: Unable to register SPI driver: %d\n", r);

	r = omap_dss_register_driver(&lb035q02_driver);
	if (r != 0)
		pr_err("lgphilips_lb035q02: Unable to register panel driver: %d\n", r);

	return r;
}

static void __exit lb035q02_panel_drv_exit(void)
{
	spi_unregister_driver(&lb035q02_spi_driver);
	omap_dss_unregister_driver(&lb035q02_driver);
}

module_init(lb035q02_panel_drv_init);
module_exit(lb035q02_panel_drv_exit);
MODULE_LICENSE("GPL");
