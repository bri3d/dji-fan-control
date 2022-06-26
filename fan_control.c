#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define CPU_TEMP_PATH "/sys/devices/platform/soc/f0a00000.apb/f0a71000.omc/temp1"
#define RAM_TEMP_PATH "/sys/devices/platform/soc/f0a00000.apb/f0a71000.omc/temp0"
#define MODEM_TEMP_PATH "/sys/devices/platform/soc/f0a00000.apb/f0a71000.omc/temp2"
#define MEDIA_TEMP_PATH "/sys/devices/platform/soc/f0a00000.apb/f0a71000.omc/temp3"

#define FAN_PWM_PATH "/sys/class/pwm/pwmchip0/pwm2"
#define PERIOD "/period"
#define DUTY_CYCLE "/duty_cycle"
#define POLARITY "/polarity"
#define ENABLE "/enable"

// This system is kind of silly
// But we need to match the DJI one or we will fight with the glasses process while it's running.

// Temperature steps, overlapping to provide some hysteresis protection
static uint8_t fan_temp[6][2] = {
    {0, 50},
    {40, 70},
    {65, 80},
    {77, 90},
    {85, 98},
    {98, 120}
};

// Fan speeds in %
static uint8_t fan_speeds[6] = {
    0,
    60,
    70,
    80,
    100,
    100
};

static volatile sig_atomic_t quit = 0;

static void sig_handler(int _)
{
    quit = 1;
}

static int32_t get_int_from_fs(char* path) {
    int32_t val;
    char read_buffer[32];
    memset(read_buffer, 0, 32);
    int fd = open(path, O_RDONLY, 0);
    if(fd < 0) {
        return -1;
    }
    int read_count = read(fd, read_buffer, 32);
    if(read_count > 0) {
        val = atoi(read_buffer);
    }
    close(fd);
    return val;
}

static uint8_t get_fan_percentage() {
    char path[255];
    memset(path, 0, 255);
    snprintf(path, 255, "%s%s", FAN_PWM_PATH, PERIOD);
    int32_t fan_period = get_int_from_fs(path);
    memset(path, 0, 255);
    snprintf(path, 255, "%s%s", FAN_PWM_PATH, DUTY_CYCLE);
    int32_t fan_duty_cycle = get_int_from_fs(path);
    return (uint8_t)((fan_duty_cycle * 100) / fan_period);
}

static void set_fan_percentage(uint8_t percent) {
    char buf[64];
    char path[255];
    memset(path, 0, 255);
    snprintf(path, 255, "%s%s", FAN_PWM_PATH, ENABLE);
    int fd = open(path, O_RDWR, 0);
    // set enable/disable
    write(fd, "0", 2);
    close(fd);
    memset(path, 0, 255);
    snprintf(path, 255, "%s%s", FAN_PWM_PATH, DUTY_CYCLE);
    fd = open(path, O_RDWR, 0);
    memset(buf, 0, 64);
    snprintf(buf, 64, "%d", percent * 10000);
    write(fd, buf, strnlen(buf, 64));
    close(fd);
    memset(path, 0, 255);
    snprintf(path, 255, "%s%s", FAN_PWM_PATH, ENABLE);
    fd = open(path, O_RDWR, 0);
    write(fd, "1", 2);
    close(fd);
}

int main(void) {
    signal(SIGINT, sig_handler);
    uint8_t fan_speed = 0;
    while(!quit) {
        int32_t temp = get_int_from_fs(CPU_TEMP_PATH);
        if(temp > fan_temp[fan_speed][1] && fan_speed < 5) {
            fan_speed++;
        }
        if(temp < fan_temp[fan_speed][0] && fan_speed > 1) {
            fan_speed--;
        }
        uint8_t fan_percent = fan_speeds[fan_speed];
        uint8_t cur_fan_percent = get_fan_percentage();
        if(cur_fan_percent != fan_percent) {
            printf("fan_control: set speed %d -> %d for temp %d\n", cur_fan_percent, fan_percent, temp);
            set_fan_percentage(fan_percent);
        }
        usleep(1000000);
    }
    return 0;
}