// SPDX-License-Identifier: GPL-2.0-only

#include <asm/io.h>
#include <dm.h>
#include <dt-bindings/clock/mediatek,mt6572-clk.h>
#include <linux/bitops.h>

#include "clk-mtk.h"

#define CLK_CFG_0		0x00

#define ARMPLL_OFFSET		0x100
#define MAINPLL_OFFSET		0x120
#define UNIVPLL_OFFSET		0x140
#define WHPLL_OFFSET		0x240

#define WHPLL_PATHSEL_CON 	0x254
#define RSV_RW0_CON1		0xf04

#define REG_CON0		0x0
#define REG_CON1		0x4
#define REG_PWR_CON0		0x10

#define CON0_RST_BAR		BIT(27)
#define PLL_AO			BIT(1)

enum {
	CLK_PAD_CLK26M,
	CLK_PAD_CLK32K,
};

static const ulong ext_clock_rates[] = {
	[CLK_PAD_CLK26M] = 26 * MHZ,
	[CLK_PAD_CLK32K] = 32000,
};

#define PLL(_id, _name, _base, _en_mask, _rst_bar_mask, _flags, _pcwbits, \
	    _fmin, _fmax)                                                 \
{                                                                         \
	.id = _id,                                                        \
	.reg = (_base) + REG_CON0,                                        \
	.pwr_reg = (_base) + REG_PWR_CON0,                                \
	.en_mask = _en_mask,                                              \
	.rst_bar_mask = _rst_bar_mask,                                    \
	.pd_reg = (_base) + REG_CON1,                                     \
	.pd_shift = 24,                                                   \
	.pcw_reg = (_base) + REG_CON1,                                    \
	.pcwbits = _pcwbits,                                              \
	.flags = _flags,                                                  \
	.fmin = _fmin,                                                    \
	.fmax = _fmax,                                                    \
}

static const struct mtk_pll_data apmixed_plls[] = {
	PLL(CLK_APMIXED_ARMPLL, "armpll", ARMPLL_OFFSET, 0x00000011, 0, PLL_AO,
	    21, 1001 * MHZ, 1989 * MHZ),

	PLL(CLK_APMIXED_MAINPLL, "mainpll", MAINPLL_OFFSET, 0x00000011,
	    CON0_RST_BAR, PLL_AO | CLK_PLL_HAVE_RST_BAR, 21, 1000 * MHZ,
	    1989 * MHZ),

	PLL(CLK_APMIXED_UNIVPLL, "univpll", UNIVPLL_OFFSET, 0x30000011,
	    CON0_RST_BAR, CLK_PLL_HAVE_RST_BAR, 7, 1248 * MHZ, 1248 * MHZ),

	PLL(CLK_APMIXED_WHPLL, "whpll", WHPLL_OFFSET, 0x00000011, 0, 0, 21,
	    1001 * MHZ, 1989 * MHZ),
};

static const struct mtk_gate_regs univpll_cg_regs = {
	.set_ofs = UNIVPLL_OFFSET,
	.clr_ofs = UNIVPLL_OFFSET,
	.sta_ofs = UNIVPLL_OFFSET,
};

#define GATE_UNIVPLL(_id, _name, _parent, _shift)          \
	GATE_FLAGS(_id, _parent, &univpll_cg_regs, _shift, \
		   CLK_GATE_NO_SETCLR | CLK_PARENT_TOPCKGEN)

static const struct mtk_gate apmixed_gates[] = {
	GATE_UNIVPLL(CLK_APMIXED_USB48M, "univpll_usb48m", CLK_TOP_UNIVPLL_D26, 26),
};

static const struct mtk_clk_tree mt6572_apmixedsys_clk_tree = {
	.pll_parent = EXT_PARENT(CLK_PAD_CLK26M),
	.ext_clk_rates = ext_clock_rates,
	.num_ext_clks = ARRAY_SIZE(ext_clock_rates),
	.gates_offs = CLK_APMIXED_USB48M,
	.plls = apmixed_plls,
	.gates = apmixed_gates,
	.num_plls = ARRAY_SIZE(apmixed_plls),
	.num_gates = ARRAY_SIZE(apmixed_gates),
	.flags = CLK_PARENT_APMIXED,
	.type = MTK_CLK_TREE_APMIXED,
};

#define FACTOR_PLL(_id, _name, _parent, _mult, _div)	\
	FACTOR(_id, _parent, _mult, _div, CLK_PARENT_APMIXED)

static const struct mtk_fixed_factor top_fixed_divs[] = {
	FACTOR_PLL(CLK_TOP_MAINPLL_D2, "mainpll_d2", CLK_APMIXED_MAINPLL, 1, 2),
	FACTOR_PLL(CLK_TOP_MAINPLL_D3, "mainpll_d3", CLK_APMIXED_MAINPLL, 1, 3),
	FACTOR_PLL(CLK_TOP_MAINPLL_D4, "mainpll_d4", CLK_APMIXED_MAINPLL, 1, 4),
	FACTOR_PLL(CLK_TOP_MAINPLL_D5, "mainpll_d5", CLK_APMIXED_MAINPLL, 1, 5),
	FACTOR_PLL(CLK_TOP_MAINPLL_D6, "mainpll_d6", CLK_APMIXED_MAINPLL, 1, 6),
	FACTOR_PLL(CLK_TOP_MAINPLL_D7, "mainpll_d7", CLK_APMIXED_MAINPLL, 1, 7),
	FACTOR_PLL(CLK_TOP_MAINPLL_D8, "mainpll_d8", CLK_APMIXED_MAINPLL, 1, 8),
	FACTOR_PLL(CLK_TOP_MAINPLL_D10, "mainpll_d10", CLK_APMIXED_MAINPLL, 1, 10),
	FACTOR_PLL(CLK_TOP_MAINPLL_D12, "mainpll_d12", CLK_APMIXED_MAINPLL, 1, 12),
	FACTOR_PLL(CLK_TOP_MAINPLL_D20, "mainpll_d20", CLK_APMIXED_MAINPLL, 1, 20),
	FACTOR_PLL(CLK_TOP_MAINPLL_D24, "mainpll_d24", CLK_APMIXED_MAINPLL, 1, 24),
	
	FACTOR_PLL(CLK_TOP_UNIVPLL_D2, "univpll_d2", CLK_APMIXED_UNIVPLL, 1, 2),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D3, "univpll_d3", CLK_APMIXED_UNIVPLL, 1, 3),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D4, "univpll_d4", CLK_APMIXED_UNIVPLL, 1, 4),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D5, "univpll_d5", CLK_APMIXED_UNIVPLL, 1, 5),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D6, "univpll_d6", CLK_APMIXED_UNIVPLL, 1, 6),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D7, "univpll_d7", CLK_APMIXED_UNIVPLL, 1, 7),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D8, "univpll_d8", CLK_APMIXED_UNIVPLL, 1, 8),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D10, "univpll_d10", CLK_APMIXED_UNIVPLL, 1, 10),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D12, "univpll_d12", CLK_APMIXED_UNIVPLL, 1, 12),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D16, "univpll_d16", CLK_APMIXED_UNIVPLL, 1, 16),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D20, "univpll_d20", CLK_APMIXED_UNIVPLL, 1, 20),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D24, "univpll_d24", CLK_APMIXED_UNIVPLL, 1, 24),
	FACTOR_PLL(CLK_TOP_UNIVPLL_D26, "univpll_d26", CLK_APMIXED_UNIVPLL, 1, 26),
};

static const struct mtk_parent uart_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_UNIVPLL_D24),
};

static const struct mtk_parent emi2x_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D3),
	TOP_PARENT(CLK_TOP_MAINPLL_D4),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D2),
};

static const struct mtk_parent axi_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D10),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D12)
};

static const struct mtk_parent mfg_mux_parents[] = {
	TOP_PARENT(CLK_TOP_MFG_PRE_491M),
	TOP_PARENT(CLK_TOP_MFG_PRE_500M),
	TOP_PARENT(CLK_TOP_MAINPLL_D3),
	TOP_PARENT(CLK_TOP_UNIVPLL_D2),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D2),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D2),
};

static const struct mtk_parent mfg_pre_parents[] = {
	TOP_PARENT(CLK_TOP_UNIVPLL_D3),
	TOP_PARENT(CLK_TOP_MFG_SEL),
};

static const struct mtk_parent msdc_sel_parents[] = {
	TOP_PARENT(CLK_TOP_MAINPLL_D12),
	TOP_PARENT(CLK_TOP_MAINPLL_D10),
	TOP_PARENT(CLK_TOP_MAINPLL_D8),
	TOP_PARENT(CLK_TOP_UNIVPLL_D7),
	TOP_PARENT(CLK_TOP_MAINPLL_D7),
	TOP_PARENT(CLK_TOP_MAINPLL_D8),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_UNIVPLL_D6),
};

static const struct mtk_parent spi_nand_sel_parents[] = {
	TOP_PARENT(CLK_TOP_MAINPLL_D24),
	TOP_PARENT(CLK_TOP_MAINPLL_D20),
	TOP_PARENT(CLK_TOP_UNIVPLL_D20),
	TOP_PARENT(CLK_TOP_UNIVPLL_D16),
	TOP_PARENT(CLK_TOP_UNIVPLL_D12),
	TOP_PARENT(CLK_TOP_UNIVPLL_D10),
	TOP_PARENT(CLK_TOP_MAINPLL_D12),
	TOP_PARENT(CLK_TOP_MAINPLL_D10),
};

static const struct mtk_parent cam_sel_parents[] = {
	TOP_PARENT(CLK_TOP_UNIVPLL_D26),
	TOP_PARENT(CLK_TOP_UNIVPLL_D6),
};

static const struct mtk_parent pwm_mm_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_UNIVPLL_D12),
};

static const struct mtk_parent spm_52m_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_UNIVPLL_D24),
};

static const struct mtk_parent pmic_spi_sel_ddr2_parents[] = {
	TOP_PARENT(CLK_TOP_MAINPLL_D24),
	TOP_PARENT(CLK_TOP_UNIVPLL_D26),
	TOP_PARENT(CLK_TOP_UNIVPLL_D16),
	EXT_PARENT(CLK_PAD_CLK26M)
};

static const struct mtk_parent pmic_spi_sel_ddr3_parents[] = {
	TOP_PARENT(CLK_TOP_MAINPLL_D20),
	TOP_PARENT(CLK_TOP_UNIVPLL_D26),
	TOP_PARENT(CLK_TOP_UNIVPLL_D16),
	EXT_PARENT(CLK_PAD_CLK26M)
};

static const struct mtk_parent aud_intbus_sel_ddr2_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D24),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D12)
};

static const struct mtk_parent aud_intbus_sel_ddr3_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D20),
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_MAINPLL_D10)
};

static const struct mtk_parent spinfi_pre_sel_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	TOP_PARENT(CLK_TOP_SPINFI_SEL)
};

static const struct mtk_composite top_muxes[] = {
	MUX(CLK_TOP_UART0_SEL, uart_sel_parents, CLK_CFG_0, 0, 1),
	MUX(CLK_TOP_EMI2X_SEL, emi2x_sel_parents, CLK_CFG_0, 1, 4),
	MUX(CLK_TOP_AXI_SEL, axi_sel_parents, CLK_CFG_0, 5, 3),
	MUX(CLK_TOP_MFG_SEL, mfg_mux_parents, CLK_CFG_0, 8, 3),
	MUX(CLK_TOP_MSDC0_SEL, msdc_sel_parents, CLK_CFG_0, 11, 3),
	MUX(CLK_TOP_SPINFI_SEL, spi_nand_sel_parents, CLK_CFG_0, 14, 3),
	MUX(CLK_TOP_CAM_SEL, cam_sel_parents, CLK_CFG_0, 17, 1),
	MUX(CLK_TOP_PWM_MM_SEL, pwm_mm_sel_parents, CLK_CFG_0, 18, 1),
	MUX(CLK_TOP_UART1_SEL, uart_sel_parents, CLK_CFG_0, 19, 1),
	MUX(CLK_TOP_MSDC1_SEL, msdc_sel_parents, CLK_CFG_0, 20, 3),
	MUX(CLK_TOP_SPM_52M_SEL, spm_52m_sel_parents, CLK_CFG_0, 23, 1),
	/* We need some way to switch between DDR2 and DDR3... */
	MUX(CLK_TOP_PMIC_SPI_SEL, pmic_spi_sel_ddr2_parents, CLK_CFG_0, 24, 2),
	MUX(CLK_TOP_AUD_INTBUS_SEL, aud_intbus_sel_ddr2_parents, CLK_CFG_0, 27, 3),
	MUX(CLK_TOP_SPINFI_PRE_SEL, spinfi_pre_sel_parents, CLK_CFG_0, 30, 1),
	MUX(CLK_TOP_MFG_PRE_SEL, mfg_pre_parents, CLK_CFG_0, 31, 1),
};

static const struct mtk_gate_regs top0_cg_regs = {
	.sta_ofs = 0x20,
	.set_ofs = 0x50,
	.clr_ofs = 0x80,
};

static const struct mtk_gate_regs top1_cg_regs = {
	.sta_ofs = 0x24,
	.set_ofs = 0x54,
	.clr_ofs = 0x84,
};

#define GATE_TOP0(_id, _name, _parent, _shift)          \
	GATE_FLAGS(_id, _parent, &top0_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

#define GATE_TOP0_INV_PLL(_id, _name, _parent, _shift)  \
	GATE_FLAGS(_id, _parent, &top0_cg_regs, _shift, \
		   CLK_GATE_SETCLR_INV | CLK_PARENT_APMIXED)

#define GATE_TOP0_INV_EXT(_id, _name, _parent, _shift)  \
	GATE_FLAGS(_id, _parent, &top0_cg_regs, _shift, \
		   CLK_GATE_SETCLR_INV | CLK_PARENT_EXT)

#define GATE_TOP1_TOP(_id, _name, _parent, _shift)      \
	GATE_FLAGS(_id, _parent, &top1_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

#define GATE_TOP1_EXT(_id, _name, _parent, _shift)      \
	GATE_FLAGS(_id, _parent, &top1_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

static const struct mtk_gate top_gates[] = {
	GATE_TOP0(CLK_TOP_PWM_MM, "pwm_mm", CLK_TOP_PWM_MM_SEL, 0),
	GATE_TOP0(CLK_TOP_CAM_MM, "cam_mm", CLK_TOP_CAM_SEL, 1),
	GATE_TOP0(CLK_TOP_MFG_MM, "mfg_mm", CLK_TOP_MFG_SEL, 2),
	GATE_TOP0(CLK_TOP_SPM_52M, "spm_52m", CLK_TOP_SPM_52M_SEL, 3),
	GATE_TOP0_INV_EXT(CLK_TOP_MIPI_26M_DBG, "mipi_26m_dbg", CLK_PAD_CLK26M, 4),
	/* ! DDR2 ! */
	GATE_TOP0(CLK_TOP_DBI_BCLK, "dbi_bclk", CLK_TOP_MAINPLL_D12, 5),
	GATE_TOP0_INV_EXT(CLK_TOP_SC_26M, "sc_26m", CLK_PAD_CLK26M, 6),
	GATE_TOP0_INV_EXT(CLK_TOP_SC_MEM, "sc_mem", CLK_PAD_CLK26M, 7),
	GATE_TOP0(CLK_TOP_DBI_PAD0, "dbi_pad0", CLK_TOP_DBI_BCLK, 16),
	GATE_TOP0(CLK_TOP_DBI_PAD1, "dbi_pad1", CLK_TOP_DBI_BCLK, 17),
	GATE_TOP0(CLK_TOP_DBI_PAD2, "dbi_pad2", CLK_TOP_DBI_BCLK, 18),
	GATE_TOP0(CLK_TOP_DBI_PAD3, "dbi_pad3", CLK_TOP_DBI_BCLK, 19),
	GATE_TOP0_INV_PLL(CLK_TOP_MFG_PRE_491M, "mfg_pre_whpll_491m", CLK_APMIXED_WHPLL, 20),
	GATE_TOP0_INV_PLL(CLK_TOP_MFG_PRE_500M, "mfg_pre_whpll_500m", CLK_APMIXED_WHPLL, 21),
	GATE_TOP0_INV_EXT(CLK_TOP_ARMDCM, "armdcm", CLK_PAD_CLK26M, 31),

	GATE_TOP1_EXT(CLK_TOP_EFUSE, "efuse", CLK_PAD_CLK26M, 0),
	GATE_TOP1_EXT(CLK_TOP_THERMAL, "thermal", CLK_PAD_CLK26M, 1),
	GATE_TOP1_TOP(CLK_TOP_APDMA, "apdma", CLK_TOP_AXI_SEL, 2),
	GATE_TOP1_TOP(CLK_TOP_I2C0, "i2c0", CLK_TOP_AXI_SEL, 3),
	GATE_TOP1_TOP(CLK_TOP_I2C1, "i2c1", CLK_TOP_AXI_SEL, 4),
	GATE_TOP1_TOP(CLK_TOP_NFI, "nfi", CLK_TOP_AXI_SEL, 6),
	GATE_TOP1_TOP(CLK_TOP_NFI_ECC, "nfi_ecc", CLK_TOP_AXI_SEL, 7),
	GATE_TOP1_TOP(CLK_TOP_DEBUGSYS, "debugsys", CLK_TOP_AXI_SEL, 8),
	GATE_TOP1_TOP(CLK_TOP_PWM, "pwm", CLK_TOP_AXI_SEL, 9),
	GATE_TOP1_TOP(CLK_TOP_UART0, "uart0", CLK_TOP_UART0_SEL, 10),
	GATE_TOP1_TOP(CLK_TOP_UART1, "uart1", CLK_TOP_UART1_SEL, 11),
	GATE_TOP1_TOP(CLK_TOP_BTIF, "btif", CLK_TOP_AXI_SEL, 12),
	GATE_TOP1_TOP(CLK_TOP_USB, "usb", CLK_TOP_AXI_SEL, 13),
	GATE_TOP1_EXT(CLK_TOP_FHCTL, "fhctl", CLK_PAD_CLK26M, 14),
	GATE_TOP1_TOP(CLK_TOP_SPINFI, "spinfi", CLK_TOP_SPINFI_SEL, 16),
	GATE_TOP1_TOP(CLK_TOP_MSDC0, "msdc0", CLK_TOP_MSDC0_SEL, 17),
	GATE_TOP1_TOP(CLK_TOP_MSDC1, "msdc1", CLK_TOP_MSDC1_SEL, 18),
	GATE_TOP1_TOP(CLK_TOP_PMIC_SPI, "pmic_spi", CLK_TOP_PMIC_SPI_SEL, 20),
	GATE_TOP1_EXT(CLK_TOP_SEJ, "sej", CLK_PAD_CLK26M, 21),
	GATE_TOP1_EXT(CLK_TOP_MEMSLP_DLYER, "memslp_dlyer", CLK_PAD_CLK26M, 22),
	GATE_TOP1_EXT(CLK_TOP_APXGPT, "apxgpt", CLK_PAD_CLK26M, 24),
	GATE_TOP1_TOP(CLK_TOP_AUD, "aud", CLK_TOP_AUD_INTBUS_SEL, 25),
	GATE_TOP1_EXT(CLK_TOP_SPM, "spm", CLK_PAD_CLK26M, 26),
	GATE_TOP1_EXT(CLK_TOP_PMIC_26M, "pmic_26m", CLK_PAD_CLK26M, 29),
	GATE_TOP1_EXT(CLK_TOP_AUXADC, "auxadc", CLK_PAD_CLK26M, 30),
};

static const struct mtk_clk_tree mt6572_topckgen_clk_tree = {
	.ext_clk_rates = ext_clock_rates,
	.num_ext_clks = ARRAY_SIZE(ext_clock_rates),
	.fdivs_offs = CLK_TOP_MAINPLL_D2,
	.muxes_offs = CLK_TOP_UART0_SEL,
	.gates_offs = CLK_TOP_PWM_MM,
	.fdivs = top_fixed_divs,
	.muxes = top_muxes,
	.gates = top_gates,
	.num_fdivs = ARRAY_SIZE(top_fixed_divs),
	.num_muxes = ARRAY_SIZE(top_muxes),
	.num_gates = ARRAY_SIZE(top_gates),
	.flags = CLK_PARENT_TOPCKGEN,
	.type = MTK_CLK_TREE_TOPCKGEN,
};

static const struct mtk_parent cpu_mux_parents[] = {
	EXT_PARENT(CLK_PAD_CLK26M),
	APMIXED_PARENT(CLK_APMIXED_ARMPLL),
	APMIXED_PARENT(CLK_APMIXED_UNIVPLL),
	TOP_PARENT(CLK_TOP_MAINPLL_D2),
};

static const struct mtk_composite infra_muxes[] = {
	MUX(CLK_INFRA_CPUSEL, cpu_mux_parents, 0x0, 2, 2),
};

static const struct mtk_clk_tree mt6572_infracfg_ao_clk_tree = {
	.ext_clk_rates = ext_clock_rates,
	.num_ext_clks = ARRAY_SIZE(ext_clock_rates),
	.muxes_offs = CLK_INFRA_CPUSEL,
	.muxes = infra_muxes,
	.num_muxes = ARRAY_SIZE(infra_muxes),
	.flags = CLK_PARENT_INFRASYS,
	.type = MTK_CLK_TREE_INFRASYS,
};

static const struct mtk_gate_regs mm0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs mm1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

#define GATE_MM0(_id, _name, _parent, _shift)          \
	GATE_FLAGS(_id, _parent, &mm0_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

#define GATE_MM0_EXT(_id, _name, _parent, _shift)      \
	GATE_FLAGS(_id, _parent, &mm0_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_EXT)

#define GATE_MM1(_id, _name, _parent, _shift)          \
	GATE_FLAGS(_id, _parent, &mm1_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

#define GATE_MM1_EXT(_id, _name, _parent, _shift)      \
	GATE_FLAGS(_id, _parent, &mm1_cg_regs, _shift, \
		   CLK_GATE_SETCLR | CLK_PARENT_TOPCKGEN)

/* CLK_TOP_MAINPLL_D6 should be smi_mm, but for simplicity we keep PLL divider here */
static const struct mtk_gate mm_clks[] = {
	GATE_MM0(CLK_MM_SMI_COMMON, "mm_smi_common", CLK_TOP_MAINPLL_D6, 0),
	GATE_MM0(CLK_MM_SMI_LARB0, "mm_smi_larb0", CLK_TOP_MAINPLL_D6, 1),
	GATE_MM0(CLK_MM_CMDQ, "mm_cmdq", CLK_TOP_MAINPLL_D6, 2),
	GATE_MM0(CLK_MM_SMI_CMDQ, "mm_smi_cmdq", CLK_TOP_MAINPLL_D6, 3),
	GATE_MM0(CLK_MM_DISP_COLOR, "mm_disp_color", CLK_TOP_MAINPLL_D6, 4),
	GATE_MM0(CLK_MM_DISP_BLS, "mm_disp_bls", CLK_TOP_MAINPLL_D6, 5),
	GATE_MM0(CLK_MM_DISP_WDMA, "mm_disp_wdma", CLK_TOP_MAINPLL_D6, 6),
	GATE_MM0(CLK_MM_DISP_RDMA, "mm_disp_rdma", CLK_TOP_MAINPLL_D6, 7),
	GATE_MM0(CLK_MM_DISP_OVL, "mm_disp_ovl", CLK_TOP_MAINPLL_D6, 8),
	GATE_MM0(CLK_MM_DISP_MDP_TDSHP, "mm_mdp_tdshp", CLK_TOP_MAINPLL_D6, 9),
	GATE_MM0(CLK_MM_DISP_MDP_WROT, "mm_mdp_wrot", CLK_TOP_MAINPLL_D6, 10),
	GATE_MM0(CLK_MM_DISP_MDP_WDMA, "mm_mdp_wdma", CLK_TOP_MAINPLL_D6, 11),
	GATE_MM0(CLK_MM_DISP_MDP_RSZ1, "mm_mdp_rsz1", CLK_TOP_MAINPLL_D6, 12),
	GATE_MM0(CLK_MM_DISP_MDP_RSZ0, "mm_mdp_rsz0", CLK_TOP_MAINPLL_D6, 13),
	GATE_MM0(CLK_MM_DISP_MDP_RDMA, "mm_mdp_rdma", CLK_TOP_MAINPLL_D6, 14),
	GATE_MM0_EXT(CLK_MM_DISP_MDP_BLS_26M, "mm_mdp_bls_26m", CLK_PAD_CLK26M, 15),
	GATE_MM0(CLK_MM_CAM, "mm_cam", CLK_TOP_MAINPLL_D6, 16),
	GATE_MM0(CLK_MM_SENINF, "mm_seninf", CLK_TOP_MAINPLL_D6, 17),
	GATE_MM0(CLK_MM_CAMTG, "mm_camtg", CLK_TOP_CAM_MM, 18),
	GATE_MM0(CLK_MM_CODEC, "mm_codec", CLK_TOP_MAINPLL_D6, 19),
	GATE_MM0(CLK_MM_DISP_FAKE_ENG, "mm_disp_fake_eng", CLK_TOP_MAINPLL_D6, 20),
	GATE_MM0_EXT(CLK_MM_MUTEX_SLOW_CLOCK, "mm_mutex_slow_clock", CLK_PAD_CLK32K, 21),

	GATE_MM1(CLK_MM_DSI_ENGINE, "mm_dsi_engine", CLK_TOP_MAINPLL_D6, 0),
	/* This clock fed by the PHY PLL, but we put clk26m for simplicity */
	GATE_MM1_EXT(CLK_MM_DSI_DIGITAL, "mm_dsi_digital", CLK_PAD_CLK26M, 1),
	GATE_MM1(CLK_MM_DPI_ENGINE, "mm_dpi_engine", CLK_TOP_MAINPLL_D6, 2),
	/* This clock fed by the PHY PLL, but we put clk26m for simplicity */
	GATE_MM1_EXT(CLK_MM_DPI_IF, "mm_dpi_if", CLK_PAD_CLK26M, 3),
	GATE_MM1(CLK_MM_DBI_ENGINE, "mm_dbi_engine", CLK_TOP_MAINPLL_D6, 4),
	GATE_MM1(CLK_MM_DBI_SMI, "mm_dbi_smi", CLK_TOP_MAINPLL_D6, 5),
	GATE_MM1(CLK_MM_DBI_IF, "mm_dbi_if", CLK_TOP_DBI_BCLK, 6),
};

static const struct mtk_clk_tree mt6572_mm_clk_tree = {
	.ext_clk_rates = ext_clock_rates,
	.num_ext_clks = ARRAY_SIZE(ext_clock_rates),
	.gates_offs = CLK_MM_SMI_COMMON,
	.gates = mm_clks,
	.num_gates = ARRAY_SIZE(mm_clks),
};

MTK_GATE_CLK_TREE(ext_clock_rates, mm_clks_tree, mm_clks);

static const struct udevice_id mt6572_clk_compat[] = {
	{ .compatible = "mediatek,mt6572-apmixedsys", .data = (ulong)&mt6572_apmixedsys_clk_tree},
	{ .compatible = "mediatek,mt6572-topckgen", .data = (ulong)&mt6572_topckgen_clk_tree },
	{ .compatible = "mediatek,mt6572-infracfg_ao", .data = (ulong)&mt6572_infracfg_ao_clk_tree },
	{ }
};

U_BOOT_DRIVER(mt6572_clk) = {
	.name = "mt6572-clk",
	.id = UCLASS_CLK,
	.of_match = mt6572_clk_compat,
	.probe = mtk_clk_probe,
	.priv_auto = sizeof(struct mtk_clk_priv),
	.ops = &mtk_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};


static int mt6572_mm_bind(struct udevice *dev)
{
	dev->driver_data = (ulong)&mt6572_mm_clk_tree;

	return 0;
}

U_BOOT_DRIVER(mtk_clk_mm) = {
	.name = "mt6572-mm",
	.id = UCLASS_CLK,
	.bind = mt6572_mm_bind,
	.probe = mtk_clk_probe,
	.priv_auto = sizeof(struct mtk_clk_priv),
	.ops = &mtk_clk_ops,
	.flags = DM_FLAG_PRE_RELOC | DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
