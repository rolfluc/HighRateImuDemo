#include "spi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "imu.h"
#include "uart.h"

#define SPI_RATE 8 * 1000 * 1000 // 8 Mhz
#define SPI_DEVICE_NODE DT_NODELABEL(lpspi1)
static const struct device *spi_dev;

#define TEST_GPIO
#ifdef TEST_GPIO

/* Define your pin index (P2_6) */
#define PIN_P2_6 6

/* 2. Configure the Software/GPIO Chip-Select Control Block */
static const struct spi_cs_control my_custom_cs = {
    .gpio = {
        .port = DEVICE_DT_GET(DT_NODELABEL(gpio2)),
        .pin = PIN_P2_6,
        .dt_flags = GPIO_ACTIVE_LOW, /* Keep it active low for standard SPI */
    },
    /* Option A: Using Nanoseconds explicitly */
    .setup_ns = 1000,  /* 2 us leading setup delay before the first clock edge */
    .hold_ns = 500,    /* 500 ns trailing hold delay after the last clock edge */
    
    /* Option B: Legacy microsecond approach (uncomment if your Zephyr target prefers it) */
    // .delay = 1,     /* Wait 1 us before starting transceive and 1 us before releasing */
};
#endif

static const struct spi_config spi_cfg = {
    .frequency = SPI_RATE, 
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB, // mode 0 // 0x60
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, // mode 1 // 0x60
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL, // mode 2 // 0x00
    //.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA, // mode 3 // 0x60
    .slave = 1, // <-- Tells the driver to use PCS1 instead of PCS0
#ifdef TEST_GPIO
    .cs = my_custom_cs,
#else
    .cs = NULL,
#endif
    .word_delay = 1000,
};

static const struct gpio_dt_spec custom_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(my_output_pin), gpios);

void InitSpi() {
    spi_dev = DEVICE_DT_GET(SPI_DEVICE_NODE);

    k_msleep(50);
    if (!gpio_is_ready_dt(&custom_pin)) {
        printk("GPIO issue.\n");
        return;
    }
    int ret = gpio_pin_configure_dt(&custom_pin, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("GPIO issue.\n");
        return;
    }
    // Reset the status of SDx/SDc.
    gpio_pin_set_dt(&custom_pin, 1);
    k_msleep(500);
    gpio_pin_set_dt(&custom_pin, 0);

    if (!device_is_ready(spi_dev)) {
        printk("SPI device not ready\n");
        return;
    }

    // Write bit is bit 0. Setting to 1 indicates read.
    uint8_t tx_whoami[2] = {0x8f, 0x00};
    uint8_t rx_whoami[2] = {0};
    struct spi_buf buf_tx_whoami = {.buf = tx_whoami, .len = sizeof(tx_whoami)};
    struct spi_buf buf_rx_whoami = {.buf = rx_whoami, .len = sizeof(rx_whoami)};
    struct spi_buf_set set_tx_whoami = {.buffers = &buf_tx_whoami, .count = 1};
    struct spi_buf_set set_rx_whoami = {.buffers = &buf_rx_whoami, .count = 1};
    ret = spi_transceive(spi_dev, &spi_cfg, &set_tx_whoami, &set_rx_whoami);

    if (ret != 0) {
        printk("SPI Failed WHOAMI\n");
    }
    if (rx_whoami[1] != 0x6b) {
        printk("SPI Failed WHOAMI - invalid data\n");
    }
    uint8_t tx_ctrl[2] = {0x10, 0x28}; 
    struct spi_buf buf_tx_ctrl = {.buf = tx_ctrl, .len = sizeof(tx_ctrl)};
    struct spi_buf_set set_tx_ctrl = {.buffers = &buf_tx_ctrl, .count = 1};
    ret = spi_write(spi_dev, &spi_cfg, &set_tx_ctrl);
    if (ret != 0) {
        printk("SPI failed config\n");
    }

    // --- ADD THIS READ-BACK TEST HERE ---
    uint8_t tx_verify[2] = { 0x10 | 0x80, 0x00 }; // 0x90 (Read CTRL1)
    uint8_t rx_verify[2] = { 0 };
    struct spi_buf buf_verify_tx = {.buf = tx_verify, .len = 2};
    struct spi_buf buf_verify_rx = {.buf = rx_verify, .len = 2};
    struct spi_buf_set set_verify_tx = {.buffers = &buf_verify_tx, .count = 1};
    struct spi_buf_set set_verify_rx = {.buffers = &buf_verify_rx, .count = 1};

    spi_transceive(spi_dev, &spi_cfg, &set_verify_tx, &set_verify_rx);
    if (rx_verify[1] != 0x28) {
        printk("SPI Failed WHOAMI - invalid data\n");
    }

    uint8_t tx_data1[7] = {0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_data1[7] = {0};
    uint16_t accel_x,accel_y,accel_z = 0;
    struct spi_buf tx_buf1 = {.buf = tx_data1, .len = sizeof(tx_data1)};
    struct spi_buf rx_buf1 = {.buf = rx_data1, .len = sizeof(rx_data1)};
    struct spi_buf_set tx_set1 = {.buffers = &tx_buf1, .count = 1};
    struct spi_buf_set rx_set1 = {.buffers = &rx_buf1, .count = 1};

    char buffer[14];
    while(1) {
        // Now that we're ready, read it and report it out.
        ret = spi_transceive(spi_dev, &spi_cfg, &tx_set1, &rx_set1);
        accel_x = (uint16_t)((rx_data1[2] << 8) | rx_data1[1]);
        accel_y = (uint16_t)((rx_data1[4] << 8) | rx_data1[3]);
        accel_z = (uint16_t)((rx_data1[6] << 8) | rx_data1[5]);

        snprintf(buffer, sizeof(buffer), "%04X%04X%04X\n", accel_x, accel_y, accel_z);
        putStrn(buffer, sizeof(buffer));
    }
}

