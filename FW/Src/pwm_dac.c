/*
 * MIT License
 *
 * Copyright 2018 Kevin Weiss for HAW Hamburg
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 ******************************************************************************
 * @addtogroup periph
 * @{
 * @file			pwm_dac.c
 * @author			Kevin Weiss
 * @date			15.03.2019
 * @brief			Controls the pwm and dac peripheral.
 * @note			The PWM and DAC share the same timer so changing the period
 * 					affects both.
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <errno.h>

#include "stm32f1xx_hal.h"

#include "PHiLIP_typedef.h"
#include "port.h"
#include "app_errno.h"
#include "app_common.h"
#include "app_defaults.h"

#include "pwm_dac.h"

/* Private enums/structs -----------------------------------------------------*/
/** @brief					The parameters for pwm control */
typedef struct {
	TIM_HandleTypeDef *htmr; /**< Handle of timer */
	TIM_OC_InitTypeDef hoc; /**< Handle of output */
	pwm_t *reg; /**< pwm live application registers */
	pwm_t *saved_reg; /**< pwm saved application registers */
} pwm_dev;

/** @brief					The parameters for dac control */
typedef struct {
	TIM_HandleTypeDef *htmr; /**< Handle of timer */
	TIM_OC_InitTypeDef hoc; /**< Handle of output */
	dac_t *reg; /**< dac live application registers */
	dac_t *saved_reg; /**< dac saved application registers */
} dac_dev;
/** @} */

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef htmr;
static pwm_dev dut_pwm;
static dac_dev dut_dac;

/* Functions -----------------------------------------------------------------*/
/**
 * @brief		Initializes pwm and dac registers.
 *
 * @param[in]	reg			Pointer to live register memory map
 * @param[in]	saved_reg	Pointer to saved register memory map
 * @note		Populates pwm defaults registers and assigns pwm and dac
 * 				register pointers.
 */
void init_dut_pwm_dac(map_t *reg, map_t *saved_reg) {
	dut_pwm.htmr = &htmr;
	dut_pwm.reg = &(reg->pwm);
	dut_pwm.saved_reg = &(saved_reg->pwm);

	dut_dac.htmr = &htmr;
	dut_dac.reg = &(reg->dac);
	dut_dac.saved_reg = &(saved_reg->dac);

	EN_RCC_DUT_PWM_DAC_CLK;

	dut_pwm.htmr->Instance = DUT_PWM_TMR;
	dut_pwm.htmr->Init.CounterMode = TIM_COUNTERMODE_UP;
	dut_pwm.htmr->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	dut_pwm.htmr->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;


	// Internal clock source
	dut_pwm.htmr->Instance->SMCR &= ~(TIM_SMCR_SMS | TIM_SMCR_TS | TIM_SMCR_ETF | TIM_SMCR_ETPS | TIM_SMCR_ECE | TIM_SMCR_ETP);
	dut_pwm.htmr->Instance->SMCR &= ~TIM_SMCR_SMS;

	dut_pwm.htmr->Instance->CR2 &= ~TIM_CR2_MMS;
	dut_pwm.htmr->Instance->CR2 |=  TIM_TRGO_RESET;
	dut_pwm.htmr->Instance->SMCR &= ~TIM_SMCR_MSM;
	dut_pwm.htmr->Instance->SMCR |= TIM_MASTERSLAVEMODE_DISABLE;

	dut_pwm.hoc.OCPolarity = TIM_OCPOLARITY_HIGH;
	dut_pwm.hoc.OCFastMode = TIM_OCFAST_DISABLE;
	commit_dut_pwm();
	commit_dut_dac();
}

/**
 * @brief		Commits the dut pwm registers and executes operations.
 *
 * @pre			pwm must first be initialized with init_dut_pwm_dac()
 * @return      EOK if init occurred
 * @return      ENOACTION if no init was triggered
 *
 * @note		Only executes actions if the pwm.mode.init is clear.
 */
error_t commit_dut_pwm() {

	if (dut_pwm.reg->mode.init) {
		return ENOACTION;
	}

	HAL_TIM_PWM_Stop(dut_pwm.htmr, TIM_CHANNEL_3);

	if (dut_pwm.reg->mode.disable) {
		dut_pwm.hoc.OCMode = TIM_OCMODE_INACTIVE;
	}
	else {
		dut_pwm.reg->period = dut_pwm.reg->h_ticks + dut_pwm.reg->l_ticks;
		uint16_t div;
		dut_pwm.hoc.OCMode = TIM_OCMODE_PWM1;
		if ((dut_pwm.reg->h_ticks > dut_pwm.reg->period) ||
			(dut_pwm.reg->l_ticks > dut_pwm.reg->period)) {
			dut_pwm.reg->mode.init = 1;
			return EOVERFLOW;
		}
		for (div = 0; (dut_pwm.reg->period >> div) >= (uint32_t)0xFFFF; div++);
		dut_pwm.htmr->Init.Period = (dut_pwm.reg->period >> div);
		dut_pwm.htmr->Init.Prescaler = (1 << div) - 1;
		dut_pwm.hoc.Pulse = dut_pwm.reg->h_ticks>>div;
		if (dut_pwm.reg->period != 0) {
			dut_pwm.reg->duty_cycle = ((uint64_t)dut_pwm.reg->h_ticks * 10000) / dut_pwm.reg->period;
		}
	}
	dut_pwm.reg->mode.init = 1;

	copy_until_same(dut_pwm.saved_reg, dut_pwm.reg, sizeof(*(dut_pwm.saved_reg)));
	TIM_Base_SetConfig(dut_pwm.htmr->Instance, &dut_pwm.htmr->Init);
	HAL_TIM_PWM_ConfigChannel(dut_pwm.htmr, &dut_pwm.hoc, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(dut_pwm.htmr, TIM_CHANNEL_3);

	return EOK;
}

/**
 * @brief		Commits the dut dac registers and executes operations.
 *
 * @pre			dac must first be initialized with init_dut_pwm_dac()
 * @return      EOK if init occurred
 * @return      ENOACTION if no init was triggered
 * @return      EINVAL value is too large
 *
 * @note		Only executes actions if the dac.mode.init is clear.
 */
error_t commit_dut_dac() {
	dac_t *reg = dut_dac.reg;
	TIM_HandleTypeDef *htmr = dut_dac.htmr;

	if (reg->mode.init) {
		return ENOACTION;
	}

	if (reg->level > 10000) {
		return EINVAL;
	}
	if (reg->mode.disable) {
		dut_dac.hoc.OCMode = TIM_OCMODE_INACTIVE;
	}
	else {
		dut_dac.hoc.OCMode = TIM_OCMODE_PWM1;
	}

	dut_dac.hoc.Pulse = ((uint32_t)reg->level * htmr->Init.Period) / 10000;
	HAL_TIM_PWM_ConfigChannel(htmr, &dut_dac.hoc, TIM_CHANNEL_4);
	HAL_TIM_PWM_Start(htmr, TIM_CHANNEL_4);
	return EOK;
}
