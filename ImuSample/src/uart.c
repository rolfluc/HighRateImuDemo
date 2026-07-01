#include "uart.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <string.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32 // TODO
#define NUM_MSG 10
#define ALIGN 4

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, NUM_MSG, ALIGN);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        uart_poll_out(uart_dev, c);
	}
}

void InitUart() {
    if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

    struct uart_config config;
    int ret = uart_config_get(uart_dev, &config);
    if (ret != 0) {
        // Handle error reading config
        return;
    }

    // 3. Update only the baudrate to your desired speed
    config.baudrate = 1228800; 

    // 4. Apply the new configuration
    ret = uart_configure(uart_dev, &config);
    if (ret != 0) {
        // Handle error applying config (e.g., hardware doesn't support that exact speed)
        return;
    }

	#if 0
	/* configure interrupt and callback to receive data */
	ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return;
	}
	uart_irq_rx_enable(uart_dev);
    // TODO migrate rx to TX for IRQ management. Won't need RX.
	#endif
}

void putChar(char c) {
    uart_poll_out(uart_dev, c);
}

void putStrn(char* str, uint8_t len) {
    for(uint8_t i = 0; i < len; i++) {
        uart_poll_out(uart_dev, str[i]);
    }
    // TODO move to IRQ based, for IMU integration.
    // TODO move to k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
}