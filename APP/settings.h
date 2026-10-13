#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "stm32f1xx_hal.h"

#define SETTINGS_MAGIC 0x53455431UL
#define SETTINGS_VERSION 1
#define SETTINGS_DEFAULT_LED_BRIGHTNESS 50
#define SETTINGS_DEFAULT_SERVO_PULSE 1500
#define SETTINGS_DEFAULT_SERVO_MIN 1000
#define SETTINGS_DEFAULT_SERVO_CENTER 1500
#define SETTINGS_DEFAULT_SERVO_MAX 2000

typedef struct
{
	uint32_t magic;
	uint16_t version;
	uint8_t led_brightness;
	uint16_t servo_pulse;
	uint16_t servo_min;
	uint16_t servo_center;
	uint16_t servo_max;
	uint16_t crc;
} SystemSettings;

void Settings_SetDefaults(SystemSettings *settings);
uint8_t Settings_Init(void);
uint8_t Settings_Load(void);
uint8_t Settings_Save(void);
uint8_t Settings_ResetDefaults(void);
uint8_t Settings_IsValid(void);
uint8_t Settings_IsDirty(void);
void Settings_Get(SystemSettings *settings);
uint8_t Settings_SetLedBrightness(uint8_t brightness);
uint8_t Settings_SetServoPulse(uint16_t pulse_us);

#endif
