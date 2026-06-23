/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart.h"
#include "spi.h"



int main(void)
{
	InitUart();
	InitSpi();
	while(1) {

	}
	return 0;
}
