#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstdint>

#define I2C_DEVICE "/dev/i2c-1"
#define BME280_ADDR 0x76

#define REG_CHIP_ID   0xD0
#define REG_CTRL_MEAS 0xF4
#define REG_TEMP_MSB  0xFA
#define REG_CALIB_T   0x88

int main() {
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, BME280_ADDR) < 0) {
        perror("Failed to set I2C address");
        return 1;
    }

    // ---- Read Chip ID ----
    uint8_t reg = REG_CHIP_ID;
    uint8_t chip_id;

    write(fd, &reg, 1);
    read(fd, &chip_id, 1);

    if (chip_id != 0x60) {
        std::cerr << "Not a BME280!" << std::endl;
        return 1;
    }

    std::cout << "BME280 detected.\n";

    // ---- Read Calibration Data (6 bytes) ----
    reg = REG_CALIB_T;
    write(fd, &reg, 1);

    uint8_t calib[6];
    read(fd, calib, 6);

    uint16_t dig_T1 = calib[1] << 8 | calib[0];
    int16_t  dig_T2 = calib[3] << 8 | calib[2];
    int16_t  dig_T3 = calib[5] << 8 | calib[4];

    // ---- Enable Temperature Measurement ----
    uint8_t ctrl_meas[2] = {REG_CTRL_MEAS, 0x27};
    write(fd, ctrl_meas, 2);

    usleep(100000); // wait 100 ms

    // ---- Read Raw Temperature ----
    reg = REG_TEMP_MSB;
    write(fd, &reg, 1);

    uint8_t temp_data[3];
    read(fd, temp_data, 3);

    int32_t adc_T =
        (temp_data[0] << 12) |
        (temp_data[1] << 4)  |
        (temp_data[2] >> 4);

    // ---- Compensation Formula (Bosch) ----
    int32_t var1, var2, t_fine;
    float temperature;

    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) *
            ((int32_t)dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
              ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
            ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;

    temperature = (t_fine * 5 + 128) >> 8;
    temperature = temperature / 100.0;

    std::cout << "Temperature: " << temperature << " °C\n";

    close(fd);
    return 0;
}