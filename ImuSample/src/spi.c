#include "spi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

// TODO which SPI
#define SPI_DEVICE_NODE DT_NODELABEL(lpspi0)

static const struct spi_config spi_cfg = {
    .frequency = 1000000, // 1 MHz
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .slave = 0,
    .cs = NULL,
};


void InitSpi() {
    const struct device *const spi_dev = DEVICE_DT_GET(SPI_DEVICE_NODE);

    if (!device_is_ready(spi_dev)) {
        printk("SPI device not ready\n");
        return;
    }

    uint8_t tx_data[] = {0xAA, 0xBB, 0xCC};
    uint8_t rx_data[3] = {0};

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

