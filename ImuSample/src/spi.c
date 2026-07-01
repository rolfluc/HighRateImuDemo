#include "spi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include "imu.h"
#include "uart.h"

#define SPI_RATE 8 * 1000 * 1000 // 8 Mhz
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
    tx_data[1] = 0x80; // High per mode, 7.68 khz
    ret =  spi_write(spi_dev, &spi_cfg, &tx_set);
    if (ret == 0) {
        printk("SPI transfer successful\n");
    } else {
        printk("SPI transfer failed: %d\n", ret);
    }


    #if 0
    uint8_t tx_dataPoll[2] = {0x9E,0x00};
    uint8_t rx_dataPoll[2] = {0,0};
    struct spi_buf tx_bufPoll = {.buf = tx_dataPoll, .len = sizeof(tx_data)};
    struct spi_buf rx_bufPoll = {.buf = rx_dataPoll, .len = sizeof(rx_data)};
    struct spi_buf_set tx_setPoll = {.buffers = &tx_bufPoll, .count = 1};
    struct spi_buf_set rx_setPoll = {.buffers = &rx_bufPoll, .count = 1};
    #endif

    uint8_t tx_data1[7] = {0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_data1[7] = {0};
    uint16_t accel_x,accel_y,accel_z = 0;
    struct spi_buf tx_buf1 = {.buf = tx_data1, .len = sizeof(tx_data1)};
    struct spi_buf rx_buf1 = {.buf = rx_data1, .len = sizeof(rx_data1)};
    struct spi_buf_set tx_set1 = {.buffers = &tx_buf1, .count = 1};
    struct spi_buf_set rx_set1 = {.buffers = &rx_buf1, .count = 1};
    uint32_t cycleVal = 0;
    char buffer[18];
    while(1) {
        cycleVal = k_cycle_get_32();

        // Now that we're ready, read it and report it out.
        ret = spi_transceive(spi_dev, &spi_cfg, &tx_set1, &rx_set1);
        accel_x = (uint16_t)((rx_data1[1] << 8) | rx_data1[0]);
        accel_y = (uint16_t)((rx_data1[3] << 8) | rx_data1[2]);
        accel_z = (uint16_t)((rx_data1[5] << 8) | rx_data1[4]);

        snprintf(buffer, sizeof(buffer), "%03X%04X%04X%04X\r\n", cycleVal & 0x00000fff, accel_x, accel_y, accel_z);
        //snprintf(buffer, sizeof(buffer), "%03X\r\n", cycleVal & 0x00000fff);
        putStrn(buffer, sizeof(buffer));
    }
}

