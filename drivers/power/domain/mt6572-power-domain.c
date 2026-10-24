// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 MediaTek Inc.
 * Author: Roman Vivchar <rva333@protonmail.com>
 */

#include <dm.h>
#include <dt-bindings/power/mediatek,mt6572-power-controller.h>
#include <linux/bitops.h>

#include "mtk-power-domain.h"

/* Bus protection bits */
#define MT6572_TOP_AXI_PROT_EN_MD1	GENMASK(10, 7)
#define MT6572_TOP_AXI_PROT_EN_CONN	(BIT(0) | BIT(4))
#define MT6572_TOP_AXI_PROT_EN_DIS	BIT(11)

static const struct mtk_scp_domain_data mt6572_scp_domain[] = {
	[MT6572_POWER_DOMAIN_MD1] = {
		.sta_mask = PWR_STATUS_MD1,
		.ctl_offs = SPM_MD1_PWR_CON,
		.pwr_sta_offs = SPM_PWR_STATUS,
		.pwr_sta2nd_offs = SPM_PWR_STATUS_2ND,
		.sram_pdn_bits = BIT(8),
		.sram_pdn_ack_bits = 0,
		.bp_cfg = {
			BUS_PROT_INFRA_UPDATE_TOPAXI(MT6572_TOP_AXI_PROT_EN_MD1),
		},
	},
	[MT6572_POWER_DOMAIN_CONN] = {
		.sta_mask = PWR_STATUS_CONN,
		.ctl_offs = SPM_CONN_PWR_CON,
		.pwr_sta_offs = SPM_PWR_STATUS,
		.pwr_sta2nd_offs = SPM_PWR_STATUS_2ND,
		.sram_pdn_bits = 0,
		.sram_pdn_ack_bits = 0,
		.bp_cfg = {
			BUS_PROT_INFRA_UPDATE_TOPAXI(MT6572_TOP_AXI_PROT_EN_CONN),
		},
	},
	[MT6572_POWER_DOMAIN_DIS] = {
		.sta_mask = PWR_STATUS_DISP,
		.ctl_offs = SPM_DIS_PWR_CON,
		.pwr_sta_offs = SPM_PWR_STATUS,
		.pwr_sta2nd_offs = SPM_PWR_STATUS_2ND,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bp_cfg = {
			BUS_PROT_INFRA_UPDATE_TOPAXI(MT6572_TOP_AXI_PROT_EN_DIS),
		},
	},
	[MT6572_POWER_DOMAIN_MFG] = {
		.sta_mask = PWR_STATUS_MFG,
		.ctl_offs = SPM_MFG_PWR_CON,
		.pwr_sta_offs = SPM_PWR_STATUS,
		.pwr_sta2nd_offs = SPM_PWR_STATUS_2ND,
		.sram_pdn_bits = BIT(8),
		.sram_pdn_ack_bits = BIT(12),
	},
};

MTK_SCP_SOC_DATA(mt6572, mt6572_scp_domain);

static const struct udevice_id mt6572_power_domain_ids[] = {
	{
		.compatible = "mediatek,mt6572-power-controller",
		.data = (ulong)&mt6572_scp_soc_data,
	},
	{ /* sentinel */ }
};

U_BOOT_DRIVER(mt6572_power_domain) = {
	.name = "mt6572_power_domain",
	.id = UCLASS_POWER_DOMAIN,
	.ops = &mtk_power_domain_ops,
	.probe = mtk_power_controller_probe,
	.of_match = mt6572_power_domain_ids,
	.priv_auto = sizeof(struct mtk_scpsys),
};
