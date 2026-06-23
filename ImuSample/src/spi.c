#include "spi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include "imu.h"

#define SPI_RATE 1 * 1000 * 1000 // 1 Mhz
#define SPI_DEVICE_NODE DT_NODELABEL(lpspi1)
static const struct device *spi_dev;

static const struct spi_config spi_cfg = {
    .frequency = SPI_RATE, // 1 MHz
    //.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
     .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, // mode 1
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL, // mode 2
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA, // mode 3
    .slave = 1, // <-- Tells the driver to use PCS1 instead of PCS0
    .cs = NULL,
};


void InitSpi() {
    spi_dev = DEVICE_DT_GET(SPI_DEVICE_NODE);

    if (!device_is_ready(spi_dev)) {
        printk("SPI device not ready\n");
        return;
    }

    // TODO Incredibly tightly coupled. Just run it for now.
    // Write bit is bit 0. Setting to 1 indicates read.
    uint8_t tx_data[2] = {LSM6_REG_SPI2_WHO_AM_I | 1 << 7, 0x00};
    uint8_t rx_data[2] = {0};
    struct spi_buf tx_buf = {.buf = tx_data, .len = sizeof(tx_data)};
    struct spi_buf rx_buf = {.buf = rx_data, .len = sizeof(rx_data)};
    struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
    struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

    int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
    if (ret == 0) {
        printk("SPI transfer successful\n");
    } else {
        printk("SPI transfer failed: %d\n", ret);
    }
}

