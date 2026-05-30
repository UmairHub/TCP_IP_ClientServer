#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <string>

#define I2C_DEVICE "/dev/i2c-1"
#define BME280_ADDR 0x76

#define REG_CHIP_ID   0xD0
#define REG_CTRL_MEAS 0xF4
#define REG_TEMP_MSB  0xFA
#define REG_CALIB_T   0x88

// Simple TCP sender with reconnect-on-failure
class TCPSender {
    std::string host_;
    int port_;
    int fd_;

    bool connectSocket() {
        if (fd_ >= 0) return true;
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) return false;

        sockaddr_in srv{};
        srv.sin_family = AF_INET;
        srv.sin_port = htons(port_);
        if (inet_pton(AF_INET, host_.c_str(), &srv.sin_addr) <= 0) {
            // try DNS lookup
            hostent* he = gethostbyname(host_.c_str());
            if (!he) { close(fd_); fd_ = -1; return false; }
            srv.sin_addr = *(in_addr*)he->h_addr;
        }

        if (connect(fd_, (sockaddr*)&srv, sizeof(srv)) < 0) {
            close(fd_); fd_ = -1; return false;
        }
        return true;
    }

public:
    TCPSender(const std::string &host, int port) : host_(host), port_(port), fd_(-1) {}
    ~TCPSender() { if (fd_ >= 0) close(fd_); }

    bool sendLine(const std::string &line) {
        if (!connectSocket()) return false;
        std::string payload = line;
        if (payload.empty() || payload.back() != '\n') payload.push_back('\n');

        const char *buf = payload.data();
        size_t remaining = payload.size();
        while (remaining) {
            ssize_t n = send(fd_, buf, remaining, 0);
            if (n > 0) { buf += n; remaining -= n; }
            else { close(fd_); fd_ = -1; return false; }
        }
        return true;
    }
};

class Sensor
{
private:
    int fd_;
    uint16_t dig_T1_;
    int16_t  dig_T2_;
    int16_t  dig_T3_;
    bool initialized_;
    // pressure calibration
    uint16_t dig_P1_;
    int16_t  dig_P2_;
    int16_t  dig_P3_;
    int16_t  dig_P4_;
    int16_t  dig_P5_;
    int16_t  dig_P6_;
    int16_t  dig_P7_;
    int16_t  dig_P8_;
    int16_t  dig_P9_;
    // humidity calibration
    uint8_t  dig_H1_;
    int16_t  dig_H2_;
    uint8_t  dig_H3_;
    int16_t  dig_H4_;
    int16_t  dig_H5_;
    int8_t   dig_H6_;
    // cached t_fine from temperature compensation
    int32_t  t_fine_;

public:
    Sensor();
    ~Sensor();
    bool isOk() const { return initialized_; }
    bool readTemperature(float &temperature);
    bool readPressure(float &pressure); // hPa
    bool readHumidity(float &humidity); // %RH
};

Sensor::Sensor()
    : fd_{-1}, dig_T1_{0}, dig_T2_{0}, dig_T3_{0}, initialized_{false}
{
    fd_ = open(I2C_DEVICE, O_RDWR);
    if (fd_ < 0) {
        perror("Failed to open I2C bus");
        return;
    }

    if (ioctl(fd_, I2C_SLAVE, BME280_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(fd_);
        fd_ = -1;
        return;
    }

    // Read Chip ID
    uint8_t reg = REG_CHIP_ID;
    uint8_t chip_id = 0;
    if (write(fd_, &reg, 1) != 1 || read(fd_, &chip_id, 1) != 1) {
        perror("Failed to read chip id");
        close(fd_);
        fd_ = -1;
        return;
    }

    if (chip_id != 0x60) {
        std::cerr << "Not a BME280! (chip id=0x" << std::hex << int(chip_id) << std::dec << ")\n";
        close(fd_);
        fd_ = -1;
        return;
    }

    // Read calibration data (temperature + pressure + H1 at 0xA1)
    reg = REG_CALIB_T;
    if (write(fd_, &reg, 1) != 1) {
        perror("Failed to set calib register");
        close(fd_);
        fd_ = -1;
        return;
    }
    uint8_t calib[26];
    if (read(fd_, calib, 26) != 26) {
        perror("Failed to read calibration data");
        close(fd_);
        fd_ = -1;
        return;
    }

    // temperature
    dig_T1_ = (uint16_t)(calib[1] << 8 | calib[0]);
    dig_T2_ = (int16_t)(calib[3] << 8 | calib[2]);
    dig_T3_ = (int16_t)(calib[5] << 8 | calib[4]);
    // pressure
    dig_P1_ = (uint16_t)(calib[7] << 8 | calib[6]);
    dig_P2_ = (int16_t)(calib[9] << 8 | calib[8]);
    dig_P3_ = (int16_t)(calib[11] << 8 | calib[10]);
    dig_P4_ = (int16_t)(calib[13] << 8 | calib[12]);
    dig_P5_ = (int16_t)(calib[15] << 8 | calib[14]);
    dig_P6_ = (int16_t)(calib[17] << 8 | calib[16]);
    dig_P7_ = (int16_t)(calib[19] << 8 | calib[18]);
    dig_P8_ = (int16_t)(calib[21] << 8 | calib[20]);
    dig_P9_ = (int16_t)(calib[23] << 8 | calib[22]);
    // humidity H1 at 0xA1
    dig_H1_ = calib[25];

    // Read humidity calibration from 0xE1
    uint8_t reg_h = 0xE1;
    if (write(fd_, &reg_h, 1) != 1) {
        perror("Failed to set humidity calib register");
        close(fd_);
        fd_ = -1;
        return;
    }
    uint8_t calib_h[7];
    if (read(fd_, calib_h, 7) != 7) {
        perror("Failed to read humidity calibration data");
        close(fd_);
        fd_ = -1;
        return;
    }

    dig_H2_ = (int16_t)(calib_h[1] << 8 | calib_h[0]);
    dig_H3_ = calib_h[2];
    dig_H4_ = (int16_t)((int16_t)(calib_h[3] << 4) | (calib_h[4] & 0x0F));
    dig_H5_ = (int16_t)((int16_t)(calib_h[5] << 4) | ((calib_h[4] & 0xF0) >> 4));
    dig_H6_ = (int8_t)calib_h[6];

    // Enable temperature measurement (forced/normal settings from original)
    uint8_t ctrl_meas[2] = {REG_CTRL_MEAS, 0x27};
    if (write(fd_, ctrl_meas, 2) != 2) {
        perror("Failed to write ctrl_meas");
        close(fd_);
        fd_ = -1;
        return;
    }

    initialized_ = true;
}

Sensor::~Sensor()
{
    if (fd_ >= 0) close(fd_);
}

bool Sensor::readTemperature(float &temperature)
{
    if (!initialized_) return false;

    // wait a short time for measurement
    usleep(100000); // 100 ms

    uint8_t reg = REG_TEMP_MSB;
    if (write(fd_, &reg, 1) != 1) {
        perror("Failed to set temp msb reg");
        return false;
    }

    uint8_t temp_data[3] = {0};
    if (read(fd_, temp_data, 3) != 3) {
        perror("Failed to read temp data");
        return false;
    }

    int32_t adc_T = ((int32_t)temp_data[0] << 12) | ((int32_t)temp_data[1] << 4) | ((int32_t)temp_data[2] >> 4);

    int32_t var1, var2, t_fine;

    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1_ << 1))) * ((int32_t)dig_T2_)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1_)) * ((adc_T >> 4) - ((int32_t)dig_T1_))) >> 12) * ((int32_t)dig_T3_)) >> 14;
    t_fine = var1 + var2;

    int32_t T = (t_fine * 5 + 128) >> 8; // temperature in centi-degrees
    temperature = T / 100.0f;

    // cache t_fine for pressure/humidity calculations
    t_fine_ = t_fine;

    return true;
}

bool Sensor::readPressure(float &pressure)
{
    if (!initialized_) return false;

    // ensure temperature was read recently to populate t_fine_
    float tmp;
    if (!readTemperature(tmp)) return false;

    uint8_t reg = 0xF7; // pressure MSB
    if (write(fd_, &reg, 1) != 1) {
        perror("Failed to set pressure msb reg");
        return false;
    }

    uint8_t data[3];
    if (read(fd_, data, 3) != 3) {
        perror("Failed to read pressure data");
        return false;
    }

    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)(data[2] >> 4));

    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine_) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6_;
    var2 = var2 + ((var1*(int64_t)dig_P5_)<<17);
    var2 = var2 + (((int64_t)dig_P4_)<<35);
    var1 = ((var1 * var1 * (int64_t)dig_P3_)>>8) + ((var1 * (int64_t)dig_P2_)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)dig_P1_)>>33;
    if (var1 == 0) return false; // avoid exception / division by zero
    p = 1048576 - adc_P;
    p = (((p<<31) - var2)*3125)/var1;
    var1 = (((int64_t)dig_P9_) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)dig_P8_) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7_)<<4);

    // p is in Pa * 256? convert to Pa
    int64_t p_pa = p >> 8;
    pressure = p_pa / 100.0f; // hPa
    return true;
}

bool Sensor::readHumidity(float &humidity)
{
    if (!initialized_) return false;

    // ensure temperature was read recently to populate t_fine_
    float tmp;
    if (!readTemperature(tmp)) return false;

    uint8_t reg = 0xFD; // humidity MSB
    if (write(fd_, &reg, 1) != 1) {
        perror("Failed to set humidity msb reg");
        return false;
    }

    uint8_t data[2];
    if (read(fd_, data, 2) != 2) {
        perror("Failed to read humidity data");
        return false;
    }

    int32_t adc_H = (int32_t)data[0] << 8 | data[1];

    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine_ - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - ((int32_t)dig_H4_ << 20) - ((int32_t)dig_H5_ * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * (int32_t)dig_H6_) >> 10) * (((v_x1_u32r * (int32_t)dig_H3_) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * (int32_t)dig_H2_ + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * (int32_t)dig_H1_) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    humidity = (v_x1_u32r>>12) / 1024.0f;
    return true;
}

int main(int argc, char **argv) {
    std::string server_host = "192.168.178.36";
    int server_port = 4000;
    if (argc >= 2) server_host = argv[1];
    if (argc >= 3) server_port = std::stoi(argv[2]);

    Sensor sensor;
    if (!sensor.isOk()) return 1;

    TCPSender sender(server_host, server_port);
    std::cout << "BME280 detected. Sending readings to " << server_host << ":" << server_port << "\n";

    while (true) {
        float temperature = 0.0f;
        float pressure = 0.0f;
        float humidity = 0.0f;

        sensor.readTemperature(temperature);
        sensor.readPressure(pressure);
        sensor.readHumidity(humidity);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "{\"temp\":" << temperature << ",\"pres\":" << pressure << ",\"hum\":" << humidity << "}";

        if (sender.sendLine(oss.str())) {
            std::cout << "Sent: " << oss.str() << "\n";
        } else {
            std::cerr << "Failed to send to server " << server_host << ":" << server_port << " (will retry)\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}