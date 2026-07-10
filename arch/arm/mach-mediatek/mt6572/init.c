// SPDX-License-Identifier: GPL-2.0

#include <cpu_func.h>
#include <config.h>
#include <init.h>
#include <asm/global_data.h>
#include <linux/bitfield.h>
#include <linux/io.h>
#include <linux/sizes.h>
#include <asm/arch/misc.h>
#include <asm/secure.h>

DECLARE_GLOBAL_DATA_PTR;

#define MCUSYS_BASE	0x10200000
#define L2_MASK		GENMASK(7, 5)
#define L2_256KB	1

static void switch_sram_to_l2(void) {
	clrsetbits_le32(MCUSYS_BASE, L2_MASK, FIELD_PREP(L2_MASK, L2_256KB));
}

static inline u32 read_actlr(void)
{
	u32 val;

	asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r"(val));

	return val;
}

static inline void write_actlr(u32 val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 1" ::"r"(val));
	isb();
}

void l2_cache_disable(void)
{
	write_actlr(read_actlr() & ~(1 << 1));
}

void l2_cache_enable(void)
{
	write_actlr(read_actlr() | (1 << 1));
}

void enable_cortex_a7_features(void)
{
	u32 val = read_actlr();

	/* SMP */
	val |= BIT(6);
	/* L1 Data Prefetch */
	val |= BIT(2);
	/* L1 Instruction Prefetch */
	val |= BIT(1);

	write_actlr(val);
}

int mach_cpu_init(void)
{
	enable_cortex_a7_features();

	l2_cache_disable();
	flush_dcache_all();
	dcache_disable();

	switch_sram_to_l2();

	l2_cache_enable();

	invalidate_dcache_all();
	invalidate_icache_all();
	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)CFG_SYS_SDRAM_BASE, SZ_1G);
	return 0;
}

void reset_cpu(void)
{
	// wdt reset
	writel(0x1209, 0x10007000 + 0x14);
}

int print_cpuinfo(void)
{
	printf("CPU:   MediaTek MT6572\n");

	return 0;
}


