/*
 * linux/arch/arm/mach-omap2/board-panther.c
 *
 * Copyright (C) 2008 Texas Instruments
 *
 * Modified from mach-omap2/board-3430sdp.c
 *               mach-omap2/board-omap3beagle.c
 *
 * Initial code: Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mmc/host.h>

#include <linux/usb/android_composite.h>

#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <plat/nand.h>
#include <plat/usb.h>

#include "mux.h"
#include "hsmmc.h"
#include "timer-gp.h"
#include "board-flash.h"

#define NAND_BLOCK_SIZE		SZ_128K

#ifdef CONFIG_USB_ANDROID

#define GOOGLE_VENDOR_ID		0x18d1
#define GOOGLE_PRODUCT_ID		0x9018
#define GOOGLE_ADB_PRODUCT_ID		0x9015

static char *usb_functions_adb[] = {
	"adb",
};

static char *usb_functions_mass_storage[] = {
	"usb_mass_storage",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
	"adb", "usb_mass_storage",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= GOOGLE_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_adb),
		.functions	= usb_functions_adb,
	},
	{
		.product_id	= GOOGLE_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mass_storage),
		.functions	= usb_functions_mass_storage,
	},
	{
		.product_id	= GOOGLE_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "rowboat",
	.product	= "rowboat gadget",
	.release	= 0x100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= GOOGLE_VENDOR_ID,
	.product_id	= GOOGLE_PRODUCT_ID,
	.functions	= usb_functions_all,
	.products	= usb_products,
	.num_products	= ARRAY_SIZE(usb_products),
	.version	= 0x0100,
	.product_name	= "rowboat gadget",
	.manufacturer_name	= "rowboat",
	.serial_number	= "20100720",
	.num_functions	= ARRAY_SIZE(usb_functions_all),
};

static struct platform_device androidusb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static void panther_android_gadget_init(void)
{
	platform_device_register(&androidusb_device);
}
#endif

static struct mtd_partition panther_nand_partitions[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name		= "X-Loader",
		.offset		= 0,
		.size		= 4 * NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x80000 */
		.size		= 10 * NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot Env",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x1c0000 */
		.size		= 6 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "Kernel",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x280000 */
		.size		= 40 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "File System",
		.offset		= MTDPART_OFS_APPEND,	/* Offset = 0x780000 */
		.size		= MTDPART_SIZ_FULL,
	},
};

/* DSS */

static int panther_enable_dvi(struct omap_dss_device *dssdev)
{
	if (gpio_is_valid(dssdev->reset_gpio))
		gpio_set_value(dssdev->reset_gpio, 1);

	return 0;
}

static void panther_disable_dvi(struct omap_dss_device *dssdev)
{
	if (gpio_is_valid(dssdev->reset_gpio))
		gpio_set_value(dssdev->reset_gpio, 0);
}

static struct omap_dss_device panther_dvi_device = {
	.type = OMAP_DISPLAY_TYPE_DPI,
	.name = "dvi",
	.driver_name = "generic_panel",
	.phy.dpi.data_lines = 24,
	.reset_gpio = -EINVAL,
	.platform_enable = panther_enable_dvi,
	.platform_disable = panther_disable_dvi,
};

static struct omap_dss_device panther_tv_device = {
	.name = "tv",
	.driver_name = "venc",
	.type = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type = OMAP_DSS_VENC_TYPE_SVIDEO,
};

static struct omap_dss_device *panther_dss_devices[] = {
	&panther_dvi_device,
	&panther_tv_device,
};

static struct omap_dss_board_info panther_dss_data = {
	.num_devices = ARRAY_SIZE(panther_dss_devices),
	.devices = panther_dss_devices,
	.default_device = &panther_dvi_device,
};

static struct platform_device panther_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &panther_dss_data,
	},
};

static struct regulator_consumer_supply panther_vdac_supply =
	REGULATOR_SUPPLY("vdda_dac", "omapdss");

static struct regulator_consumer_supply panther_vdvi_supply =
	REGULATOR_SUPPLY("vdds_dsi", "omapdss");

static void __init panther_display_init(void)
{
	int r;

	/* DVI */
	panther_dvi_device.reset_gpio = 129;
	omap_mux_init_gpio(panther_dvi_device.reset_gpio, OMAP_PIN_INPUT);

	r = gpio_request(panther_dvi_device.reset_gpio, "DVI reset");
	if (r < 0) {
		printk(KERN_ERR "Unable to get DVI reset GPIO\n");
		return;
	}

	gpio_direction_output(panther_dvi_device.reset_gpio, 0);
}

#include "sdram-micron-mt46h32m32lf-6.h"

static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
		.gpio_wp	= 29,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply panther_vmmc1_supply = {
	.supply			= "vmmc",
};

// This regulator has been enaled in u-boot. The following code is removed since we don't need to control it in kernel.
#if 0
static struct regulator_consumer_supply panther_vsim_supply = {
// Rename regulator's name from "vmmc_aux" to "vsim" to avoid unnecessary operations in "<ROWBOAT_ANDROID>/kernel/drivers/host/omap_hsmmc.c".
#if 0
	.supply			= "vmmc_aux",
#else
	.supply			= "vsim",
#endif
};
#endif

static struct regulator_consumer_supply panther_vaux3_supply = {
	.supply         = "cam_1v8",
};

static struct regulator_consumer_supply panther_vaux4_supply = {
	.supply         = "cam_2v8",
};

static struct gpio_led gpio_leds[];

static int panther_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	mmc[0].gpio_wp = -EINVAL;

	/* gpio + 0 is "mmc0_cd" (input/IRQ) */
	mmc[0].gpio_cd = gpio + 0;
	omap2_hsmmc_init(mmc);

	/* link regulators to MMC adapters */
	panther_vmmc1_supply.dev = mmc[0].dev;
	// VSIM isn't a part of MMC1's system.
	//panther_vsim_supply.dev = mmc[0].dev;

	// Move to panther_display_init()
	/* DVI reset GPIO */
	//panther_dvi_device.reset_gpio = 129;

	/* TWL4030_GPIO_MAX + 0 == ledA, EHCI nEN_USB_PWR (out, active low) */
	gpio_request(gpio + TWL4030_GPIO_MAX, "nEN_USB_PWR");
	gpio_direction_output(gpio + TWL4030_GPIO_MAX, 1);

	/* gpio + 1 on Panther controls the TFP410's enable line (active low) */
	gpio_request(gpio + 1, "nDVI_PWR_EN");
	gpio_direction_output(gpio + 1, 0);

	return 0;
}

static struct twl4030_gpio_platform_data panther_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.use_leds	= true,
	.pullups	= BIT(1),
	.pulldowns	= BIT(2) | BIT(6) | BIT(7) | BIT(8) | BIT(13)
				| BIT(15) | BIT(16) | BIT(17),
	.setup		= panther_twl_gpio_setup,
};

/* VMMC1 for MMC1 pins CMD, CLK, DAT0..DAT3 (20 mA, plus card == max 220 mA) */
static struct regulator_init_data panther_vmmc1 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &panther_vmmc1_supply,
};

// VSIM is not used by pantherboard right now.
#if 0
/* VSIM for GPIO_126,GPIO_127 & GPIO_129 */
static struct regulator_init_data panther_vsim = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &panther_vsim_supply,
};
#endif

/* VDAC for DSS driving S-Video (8 mA unloaded, max 65 mA) */
static struct regulator_init_data panther_vdac = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &panther_vdac_supply,
};

/* VPLL2 for digital video outputs */
static struct regulator_init_data panther_vpll2 = {
	.constraints = {
		.name			= "VDVI",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &panther_vdvi_supply,
};

/* VAUX3 for CAM_1V8 */
static struct regulator_init_data panther_vaux3 = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &panther_vaux3_supply,
};

 /* VAUX4 for CAM_2V8 */
static struct regulator_init_data panther_vaux4 = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
			| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
			| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &panther_vaux4_supply,
};

static struct twl4030_usb_data panther_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_codec_audio_data panther_audio_data = {
	.audio_mclk = 26000000,
	.digimic_delay = 1,
	.ramp_delay_value = 1,
	.offset_cncl_path = 1,
	.check_defaults = false,
	.reset_registers = false,
	.reset_registers = false,
};

static struct twl4030_codec_data panther_codec_data = {
	.audio_mclk = 26000000,
	.audio = &panther_audio_data,
};

static struct twl4030_platform_data panther_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.usb		= &panther_usb_data,
	.gpio		= &panther_gpio_data,
	.codec		= &panther_codec_data,
// The following table shows the relationship of pantherboard's regulators.
//
//	[LDO NAME]	[SCHEMATIC SYMBOLE]	[CONNECTED DEVICE (Outer/Inner)]
//	VPLL2		VDD_MIC		None / DSI, CSIPHY2
//	VMMC1		VDD_MMC1		Micro SD / None
//	VMMC2		VMMC2			None / Nnoe
//	VAUX1		CAM_ANA		None / None
//	VAUX2		EXP_VDD		USB PHY / None
//	VAUX3		CAM_DIGITAL		Camera Module / None
//	VAUX4		CAM_IO			Camera Module / None
// ========== Listed below are the regulators which used inside AP module ==========
//	VDAC		None			None / Video Buffer, DAC(S-Video & CVBS)
//	VPLL1		None			None / DPLL, DLL
//	VMIC1		None			None / Unknown
//	VMIC2		None			None / Unknown
//	VSIM		None			None / GPIO_126, GPIO127, GPIO_129
	.vmmc1		= &panther_vmmc1,
// VSIM is not used by pantherboard right now.
#if 0
	.vsim		= &panther_vsim,
#endif
	.vdac		= &panther_vdac,
	.vpll2		= &panther_vpll2,
	.vaux3		= &panther_vaux3,
	.vaux4		= &panther_vaux4,
};

static struct i2c_board_info __initdata panther_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl4030", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &panther_twldata,
	},
};

static struct i2c_board_info __initdata panther_i2c_eeprom[] = {
       {
               I2C_BOARD_INFO("eeprom", 0x50),
       },
};

static int __init panther_i2c_init(void)
{
	omap_register_i2c_bus(1, 2600, panther_i2c_boardinfo,
			ARRAY_SIZE(panther_i2c_boardinfo));

	/* Bus 2 is used for Camera/Sensor interface */
	omap_register_i2c_bus(2, 400, NULL, 0);

	/* Bus 3 is attached to the DVI port where devices like the pico DLP
	 * projector don't work reliably with 400kHz */
	omap_register_i2c_bus(3, 100, panther_i2c_eeprom, ARRAY_SIZE(panther_i2c_eeprom));

	return 0;
}

static struct gpio_led gpio_leds[] = {
// Pantherboard's leds aren't driven by GPIOs (except for D701(USB Active)).
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	},
};

static struct gpio_keys_button gpio_buttons[] = {
// Jack_20110907: defined the buttons on Chipsee's expansion board
//	To see the definitions of SCANCODE and KEYCODE, please refer to
//		-- <ROWBOAT_ANDROID>/kernel/include/linux/input.h
//		-- <ROWBOAT_ANDROID>/sdk/emulators/keymaps/qwerty.kl
	{
		.code			= KEY_PROG1,
		.gpio			= 4,
		.desc			= "user",
		.active_low		= false,
		.wakeup			= 1,
	},
};

static struct gpio_keys_platform_data gpio_key_info = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
};

static struct platform_device keys_gpio = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_key_info,
	},
};

static void __init panther_init_irq(void)
{
	omap2_init_common_infrastructure();
	omap2_init_common_devices(mt46h32m32lf6_sdrc_params,
				  mt46h32m32lf6_sdrc_params);
	omap_init_irq();
	gpmc_init();
#ifdef CONFIG_OMAP_32K_TIMER
	omap2_gp_clockevent_set_gptimer(1);
#endif
}

static struct platform_device *panther_devices[] __initdata = {
	&leds_gpio,
	&keys_gpio,
	&panther_dss_device,
	&usb_mass_storage_device,
};

static void __init panther_flash_init(void)
{
	u8 cs = 0;
	u8 nandcs = GPMC_CS_NUM + 1;

	/* find out the chip-select on which NAND exists */
	while (cs < GPMC_CS_NUM) {
		u32 ret = 0;
		ret = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG1);

		if ((ret & 0xC00) == 0x800) {
			printk(KERN_INFO "Found NAND on CS%d\n", cs);
			if (nandcs > GPMC_CS_NUM)
				nandcs = cs;
		}
		cs++;
	}

	if (nandcs > GPMC_CS_NUM) {
		printk(KERN_INFO "NAND: Unable to find configuration "
				 "in GPMC\n ");
		return;
	}

	if (nandcs < GPMC_CS_NUM) {
		printk(KERN_INFO "Registering NAND on CS%d\n", nandcs);
		board_nand_init(panther_nand_partitions,
			ARRAY_SIZE(panther_nand_partitions),
			nandcs, NAND_BUSWIDTH_16);
	}
}

static const struct ehci_hcd_omap_platform_data ehci_pdata __initconst = {

	.port_mode[0] = EHCI_HCD_OMAP_MODE_UNKNOWN,
	.port_mode[1] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[2] = EHCI_HCD_OMAP_MODE_UNKNOWN,

	.phy_reset  = true,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = 39,
	.reset_gpio_port[2]  = -EINVAL
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#endif

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
	.mode			= MUSB_OTG,
	.power			= 100,
};

static void __init panther_init(void)
{
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	panther_i2c_init();
	platform_add_devices(panther_devices,
			ARRAY_SIZE(panther_devices));
	omap_serial_init();

	usb_musb_init(&musb_board_data);
	usb_ehci_init(&ehci_pdata);
	panther_flash_init();

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	panther_display_init();
#ifdef CONFIG_USB_ANDROID
	panther_android_gadget_init();
#endif
}

MACHINE_START(PANTHER, "Panther Board")
	/* Maintainer: Syed Mohammed Khasim - http://beagleboard.org */
	.boot_params	= 0x80000100,
	.map_io		= omap3_map_io,
	.reserve	= omap_reserve,
	.init_irq	= panther_init_irq,
	.init_machine	= panther_init,
	.timer		= &omap_timer,
MACHINE_END
