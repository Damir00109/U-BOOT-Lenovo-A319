// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Frank Wunderlich <frank-w@public-files.de>
 */

#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <power/pmic.h>
#include <dm.h>

#define MT6323_PWRC_BASE	0x8000
#define RTC_BBPU		0x0000
#define RTC_BBPU_KEY		(0x43 << 8)
#define RTC_WRTGR		0x003c
#define RTC_BBPU_PWREN		BIT(0)
#define RTC_BBPU_AUTO		BIT(3)

int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *pmic;
	u16 val;
	int ret;

	ret = uclass_get_device(UCLASS_PMIC, 0, &pmic);
	if (ret) {
		printf("Failed to find mt6323 dev (%d)\n", ret);
		return ret;
	}

	val = RTC_BBPU_KEY | RTC_BBPU_PWREN | RTC_BBPU_AUTO;
	pmic_reg_write(pmic, MT6323_PWRC_BASE + RTC_BBPU, val);
	pmic_reg_write(pmic, MT6323_PWRC_BASE + RTC_WRTGR, 1);
	mdelay(10);

	mdelay(400);
	/* If device isn't off by this point then usb needs to be unplugged */
	printf("Unplug USB to shutdown!\n");
	return CMD_RET_SUCCESS;
}
