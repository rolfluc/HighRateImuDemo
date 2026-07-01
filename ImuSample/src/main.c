/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart.h"
#include "spi.h"
#include <zephyr/kernel.h>


int main(void)
{
	InitUart();
	// printk("%x",sys_clock_hw_cycles_per_sec());
	InitSpi();
	while(1) {

	}
	return 0;
}
