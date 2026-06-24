#include "spi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include "imu.h"

#define SPI_RATE 2 * 1000 * 1000 // 4 Mhz
#define SPI_DEVICE_NODE DT_NODELABEL(lpspi1)
static const struct device *spi_dev;

static const struct spi_config spi_cfg = {
    .frequency = SPI_RATE, 
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB, // mode 0 // 0x60
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, // mode 1 // 0x60
    // .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL, // mode 2 // 0x00
    //.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA, // mode 3 // 0x60
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
    uint8_t tx_data[2] = {LSM6_REG_SPI2_WHO_AM_I | LSM6_READ, 0x00};
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
    // write to CTRL1 (0x10) to activate accelerometer
    tx_data[0] = LSM6_REG_CTRL1; // write mode
    tx_data[1] = 0b00001100; // High per mode, 7.68 khz
    ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
    if (ret == 0) {
        printk("SPI transfer successful\n");
    } else {
        printk("SPI transfer failed: %d\n", ret);
    }
    uint8_t tx_data1[3] = {LSM6_REG_SPI2_OUTX_L_G_OIS | LSM6_READ, 0x00};
    uint8_t rx_data1[3] = {0};
    struct spi_buf tx_buf1 = {.buf = tx_data, .len = sizeof(tx_data1)};
    struct spi_buf rx_buf1 = {.buf = rx_data, .len = sizeof(rx_data1)};
    struct spi_buf_set tx_set1 = {.buffers = &tx_buf1, .count = 1};
    struct spi_buf_set rx_set1 = {.buffers = &rx_buf1, .count = 1};
    ret = spi_transceive(spi_dev, &spi_cfg, &tx_set1, &rx_set1);
}

