// SPDX-License-Identifier: GPL-2.0
#include <asm/global_data.h>
#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <env.h>
#include <init.h>
#include <fdt_support.h>
#include <mmc.h>
#include <wdt.h>
#include <dt-bindings/clock/mediatek,mt6572-clk.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	#define SPM_PROJECT_CODE            0xb16
	writel((SPM_PROJECT_CODE << 16) | (1U << 0), 0x10006000);

	return 0;
}

static int mt6572_get_clk_provider(const char *compatible,
				  struct udevice **dev)
{
	ofnode node;

	node = ofnode_by_compatible(ofnode_null(), compatible);
	if (!ofnode_valid(node))
		return -ENODEV;

	return uclass_get_device_by_ofnode(UCLASS_CLK, node, dev);
}

int board_postclk_init(void) {
	struct udevice *topckgen_dev, *apmixed_dev, *infra_dev;
	int ret;

	ret = mt6572_get_clk_provider("mediatek,mt6572-topckgen", &topckgen_dev);
	if (ret)
		return log_ret(ret);

	ret = mt6572_get_clk_provider("mediatek,mt6572-apmixedsys", &apmixed_dev);
	if (ret)
		return log_ret(ret);

	ret = mt6572_get_clk_provider("mediatek,mt6572-infracfg_ao", &infra_dev);
	if (ret)
		return log_ret(ret);

	struct clk cpu_mux = { .dev = infra_dev, .id = CLK_INFRA_CPUSEL };
	struct clk armpll = { .dev = apmixed_dev, .id = CLK_APMIXED_ARMPLL };
	struct clk mainpll_d2 = { .dev = topckgen_dev, .id = CLK_TOP_MAINPLL_D2 };

	ret = clk_set_parent(&cpu_mux, &mainpll_d2);
	if (ret)
		return log_ret(ret);

	ret = clk_set_rate(&armpll, 1001000000);
	if (ret < 0)
		return log_ret(ret);

	ret = clk_enable(&armpll);
	if (ret < 0)
		return log_ret(ret);

	ret = clk_set_parent(&cpu_mux, &armpll);
	if (ret)
		return log_ret(ret);

	/* Setup SPM clocks */
	struct clk spm52m = { .dev = topckgen_dev, .id = CLK_TOP_SPM_52M };
	ret = clk_disable(&spm52m);
	if (ret)
		return log_ret(ret);

	struct clk sc26m = { .dev = topckgen_dev, .id = CLK_TOP_SC_26M };
	ret = clk_enable(&sc26m);
	if (ret)
		return log_ret(ret);

	struct clk scmem = { .dev = topckgen_dev, .id = CLK_TOP_SC_MEM };
	ret = clk_disable(&scmem);
	if (ret)
		return log_ret(ret);

	struct clk memslp_dlyer = { .dev = topckgen_dev, .id = CLK_TOP_MEMSLP_DLYER };
	ret = clk_enable(&memslp_dlyer);
	if (ret)
		return log_ret(ret);

	struct clk spm = { .dev = topckgen_dev, .id = CLK_TOP_SPM };
	ret = clk_enable(&spm);
	if (ret)
		return log_ret(ret);

	return 0;
}

int board_late_init(void)
{
	struct udevice *dev;
	int ret;

	/* Give fastboot and USB a stable, device-unique serial number. */
	struct mmc *mmc = find_mmc_device(0);

	if (mmc && !mmc_init(mmc)) {
		/* Product serial number lives in the lower half of CID[2..3]. */
		u32 psn = ((mmc->cid[2] & 0xffff) << 16) | (mmc->cid[3] >> 16);
		char serial[16];

		snprintf(serial, sizeof(serial), "A319%08x", psn);
		env_set("serial#", serial);
	}

	ret = uclass_get_device(UCLASS_USB_GADGET_GENERIC, 0, &dev);
	if (ret) {
		printf("%s: failed to get USB: %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

/*
 * The watchdog is started by U-Boot and serviced by its cyclic handler.
 * Nothing pings it between the handoff and the OS watchdog driver, so stop
 * it here to avoid a reset during early kernel boot.
 */
void arch_preboot_os(void)
{
	wdt_stop_all();
}

/* LK framebuffer */
phys_addr_t board_get_usable_ram_top(phys_addr_t total_size)
{
	if (of_machine_is_compatible("jty,d101"))
		return 0xbf400000;
	else if (of_machine_is_compatible("lenovo,a369i") ||
		 of_machine_is_compatible("lenovo,a319"))
		return 0x9fa00000;
	else
		panic("invalid compatible");
}
