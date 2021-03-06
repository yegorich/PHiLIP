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
 * @addtogroup Application
 * @{
 * @file			app_shell_if.h
 * @author			Kevin Weiss
 * @date			13.02.2019
 * @brief			Protocol for application communication.
 * @details			This initializes and runs the serial communication
 * 					protocol for interfacing to registers.  It used the STM HAL
 * 					and a UART for the IO.  It also uses DMA.
 ******************************************************************************
 */

#ifndef APP_SHELL_IF_H_
#define APP_SHELL_IF_H_

/* Defines -------------------------------------------------------------------*/
/** @brief   Parse on newline */
#define RX_END_CHAR		'\n'

/** @brief   Send newline at end of transmission */
#define TX_END_STR		"\n"

/* Function prototypes -------------------------------------------------------*/
/**
 * @brief Parses a string and executes commands.
 *
 * @param[in]	str			String with the command
 * @param[in]	buf_size	The max size of the string buffer
 * @param[in]	access		The callers access level
 *
 * @return 		EOK on success
 * @return 		EPROTONOSUPPORT command not supported
 * @return 		EACCES caller doesn't have access
 * @return 		EMSGSIZE message size too big
 * @return 		EINVAL Invalid value
 * @return 		EOVERFLOW invalid address
 * @return 		ERANGE invalid number range
 * @return 		ENODATA not enough data
 * @return 		EUNKNOWN
 *
 * @warning		May protect interrupts and cause jitter.
 */
error_t parse_command(char *str, uint16_t buf_size, uint8_t access);

#endif /* APP_SHELL_IF_H_ */
/** @} */
