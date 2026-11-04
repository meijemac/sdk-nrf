/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Code RAM (CRAM) bank power down header.
 */

#ifndef CRAM_PWRDN_H__
#define CRAM_PWRDN_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup cram_pwrdn CRAM Power Down
 * @{
 * @brief Module for powering on and off Code RAM (CRAM) banks.
 *
 * CRAM is split into @ref CRAM_PWRDN_BANK_COUNT banks of @ref CRAM_PWRDN_BANK_SIZE bytes.
 * Bank n starts at @ref CRAM_PWRDN_BANK_ADDR(n) and is powered by bit
 * (@ref CRAM_PWRDN_FIRST_BIT + n) of MEMCONF POWER[@ref CRAM_PWRDN_POWER_ID].CONTROL.
 *
 * CRAM is volatile. A bank that is powered down loses its content. After the bank is
 * powered up again its content is undefined, so it must be written before it is read
 * or executed from.
 */

/** @brief Start address of CRAM. */
#define CRAM_PWRDN_BASE 0x00000000UL

/** @brief Number of independently powered CRAM banks. */
#define CRAM_PWRDN_BANK_COUNT 10

/** @brief Size of one CRAM bank in bytes. */
#define CRAM_PWRDN_BANK_SIZE (128UL * 1024UL)

/** @brief End address (exclusive) of CRAM. */
#define CRAM_PWRDN_END (CRAM_PWRDN_BASE + CRAM_PWRDN_BANK_COUNT * CRAM_PWRDN_BANK_SIZE)

/** @brief Index of the MEMCONF POWER[] entry that controls CRAM. */
#define CRAM_PWRDN_POWER_ID 0

/** @brief MEMCONF POWER[].CONTROL bit that controls CRAM bank 0. */
#define CRAM_PWRDN_FIRST_BIT 10

/** @brief Start address of CRAM bank @p bank. */
#define CRAM_PWRDN_BANK_ADDR(bank) (CRAM_PWRDN_BASE + (uintptr_t)(bank) * CRAM_PWRDN_BANK_SIZE)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the CRAM banks that must stay powered.
 *
 * A bank is in use when it holds any part of the running application image
 * (code, read-only data and the initial values of .data), or when it is not
 * wholly inside the CRAM assigned to this core by the zephyr,flash chosen node,
 * for example the bank that holds the FLPR code when a FLPR snippet is used.
 *
 * @return Mask of banks in use, bit n is set for bank n.
 */
uint32_t cram_banks_in_use(void);

/**
 * @brief Power a single CRAM bank on or off.
 *
 * @param bank   Bank index, from 0 to @ref CRAM_PWRDN_BANK_COUNT - 1.
 * @param enable True to power the bank on, false to power it off.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p bank is out of range.
 * @retval -EBUSY  Powering off was requested for a bank that is in use,
 *                 see @ref cram_banks_in_use.
 */
int cram_bank_power_set(uint32_t bank, bool enable);

/**
 * @brief Check whether a CRAM bank is powered.
 *
 * @param bank Bank index, from 0 to @ref CRAM_PWRDN_BANK_COUNT - 1.
 *
 * @return True if the bank is powered, false if it is powered off or @p bank is out of range.
 */
bool cram_bank_is_powered(uint32_t bank);

/**
 * @brief Request powering down CRAM banks for the given address range.
 *
 * Power down CRAM banks which fully fall within the given address range and
 * are not in use, see @ref cram_banks_in_use.
 */
void power_down_cram(uintptr_t start_address, uintptr_t end_address);

/**
 * @brief Request powering up CRAM banks for the given address range.
 *
 * Power up CRAM banks which overlap with the given address range.
 */
void power_up_cram(uintptr_t start_address, uintptr_t end_address);

/**
 * @brief Request powering down unused CRAM banks.
 *
 * Power down every CRAM bank that is not in use, see @ref cram_banks_in_use.
 */
void power_down_unused_cram(void);

/**
 * @brief Request powering up unused CRAM banks.
 *
 * Power up CRAM banks which were disabled by @ref power_down_unused_cram function.
 */
void power_up_unused_cram(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* CRAM_PWRDN_H__ */
