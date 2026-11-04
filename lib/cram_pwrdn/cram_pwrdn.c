/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_memconf.h>
#include <cram_pwrdn.h>

LOG_MODULE_REGISTER(cram_pwrdn, CONFIG_CRAM_POWERDOWN_LOG_LEVEL);

#define CRAM_ALL_BANKS BIT_MASK(CRAM_PWRDN_BANK_COUNT)

#define APP_CRAM_NODE  DT_CHOSEN(zephyr_flash)
#define APP_CRAM_START DT_REG_ADDR(APP_CRAM_NODE)
#define APP_CRAM_END   (APP_CRAM_START + DT_REG_SIZE(APP_CRAM_NODE))

BUILD_ASSERT(CRAM_PWRDN_FIRST_BIT + CRAM_PWRDN_BANK_COUNT <= 32,
	     "CRAM banks do not fit in one MEMCONF POWER[].CONTROL register");
BUILD_ASSERT(CRAM_PWRDN_BASE == NRF_MEMORY_FLASH_BASE &&
	     CRAM_PWRDN_END == NRF_MEMORY_FLASH_BASE + NRF_MEMORY_FLASH_SIZE,
	     "CRAM bank layout does not match the MDK memory map");
BUILD_ASSERT(APP_CRAM_END <= CRAM_PWRDN_END, "zephyr,flash extends beyond CRAM");

/*
 * Banks covering [start_address, end_address). With whole_only set, only banks that
 * lie entirely inside the range are returned; otherwise every overlapping bank is.
 */
static uint32_t banks_in_range(uintptr_t start_address, uintptr_t end_address, bool whole_only)
{
	uintptr_t start = MAX(start_address, CRAM_PWRDN_BASE) - CRAM_PWRDN_BASE;
	uintptr_t end = MIN(end_address, CRAM_PWRDN_END) - CRAM_PWRDN_BASE;
	uint32_t first;
	uint32_t last;

	if (start_address >= CRAM_PWRDN_END || end_address <= CRAM_PWRDN_BASE || start >= end) {
		return 0;
	}

	if (whole_only) {
		first = DIV_ROUND_UP(start, CRAM_PWRDN_BANK_SIZE);
		last = end / CRAM_PWRDN_BANK_SIZE;
	} else {
		first = start / CRAM_PWRDN_BANK_SIZE;
		last = DIV_ROUND_UP(end, CRAM_PWRDN_BANK_SIZE);
	}

	if (first >= last) {
		return 0;
	}

	return GENMASK(last - 1, first);
}

static void banks_power_set(uint32_t banks, bool enable)
{
	unsigned int key;

	banks &= CRAM_ALL_BANKS;
	if (banks == 0) {
		return;
	}

	LOG_DBG("%s CRAM banks 0x%03x", enable ? "Powering up" : "Powering down", banks);

	key = irq_lock();
	nrf_memconf_ramblock_control_mask_enable_set(NRF_MEMCONF, CRAM_PWRDN_POWER_ID,
						     banks << CRAM_PWRDN_FIRST_BIT, enable);
	irq_unlock(key);
}

uint32_t cram_banks_in_use(void)
{
	uintptr_t image_start = (uintptr_t)__rom_region_start;
	uintptr_t image_end = image_start + (uintptr_t)_flash_used;
	uint32_t owned = banks_in_range(APP_CRAM_START, APP_CRAM_END, true);

	return (CRAM_ALL_BANKS & ~owned) | banks_in_range(image_start, image_end, false);
}

int cram_bank_power_set(uint32_t bank, bool enable)
{
	if (bank >= CRAM_PWRDN_BANK_COUNT) {
		return -EINVAL;
	}

	if (!enable && (cram_banks_in_use() & BIT(bank))) {
		return -EBUSY;
	}

	banks_power_set(BIT(bank), enable);

	return 0;
}

bool cram_bank_is_powered(uint32_t bank)
{
	if (bank >= CRAM_PWRDN_BANK_COUNT) {
		return false;
	}

	return nrf_memconf_ramblock_control_enable_check(NRF_MEMCONF, CRAM_PWRDN_POWER_ID,
							 CRAM_PWRDN_FIRST_BIT + bank);
}

void power_down_cram(uintptr_t start_address, uintptr_t end_address)
{
	banks_power_set(banks_in_range(start_address, end_address, true) & ~cram_banks_in_use(),
			false);
}

void power_up_cram(uintptr_t start_address, uintptr_t end_address)
{
	banks_power_set(banks_in_range(start_address, end_address, false), true);
}

void power_down_unused_cram(void)
{
	power_down_cram(CRAM_PWRDN_BASE, CRAM_PWRDN_END);
}

void power_up_unused_cram(void)
{
	banks_power_set(CRAM_ALL_BANKS & ~cram_banks_in_use(), true);
}

#if defined(CONFIG_CRAM_POWER_DOWN_UNUSED_AT_BOOT)
static int cram_pwrdn_init(void)
{
	power_down_unused_cram();

	return 0;
}

SYS_INIT(cram_pwrdn_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
