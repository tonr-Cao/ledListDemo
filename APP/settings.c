#include "settings.h"
#include "w25qxx.h"

#define SETTINGS_FLASH_ADDRESS \
	(W25QXX_CAPACITY_BYTES - W25QXX_SECTOR_SIZE)
#define SETTINGS_RECORD_SIZE 17
#define SETTINGS_CRC_DATA_SIZE 15
#define SETTINGS_MIN_SAFE_SERVO_PULSE 1000
#define SETTINGS_MAX_SAFE_SERVO_PULSE 2000

static SystemSettings current_settings;
static uint8_t settings_initialized;
static uint8_t settings_dirty;
static uint8_t settings_flash_supported;
static uint8_t settings_valid;

static uint16_t Settings_CalculateCRC(const uint8_t *data, uint8_t size)
{
	uint8_t i;
	uint8_t bit;
	uint16_t crc;

	crc = 0xFFFF;
	for(i = 0; i < size; i++)
	{
		crc ^= (uint16_t)data[i] << 8;
		for(bit = 0; bit < 8; bit++)
		{
			if(crc & 0x8000)
			{
				crc = (crc << 1) ^ 0x1021;
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

static uint8_t Settings_ParametersValid(const SystemSettings *settings)
{
	return (uint8_t)(settings->led_brightness <= 100 &&
		settings->servo_min >= SETTINGS_MIN_SAFE_SERVO_PULSE &&
		settings->servo_min < settings->servo_center &&
		settings->servo_center < settings->servo_max &&
		settings->servo_max <= SETTINGS_MAX_SAFE_SERVO_PULSE &&
		settings->servo_pulse >= settings->servo_min &&
		settings->servo_pulse <= settings->servo_max);
}

static void Settings_Encode(const SystemSettings *settings,
	uint8_t record[SETTINGS_RECORD_SIZE])
{
	uint16_t crc;

	record[0] = (uint8_t)settings->magic;
	record[1] = (uint8_t)(settings->magic >> 8);
	record[2] = (uint8_t)(settings->magic >> 16);
	record[3] = (uint8_t)(settings->magic >> 24);
	record[4] = (uint8_t)settings->version;
	record[5] = (uint8_t)(settings->version >> 8);
	record[6] = settings->led_brightness;
	record[7] = (uint8_t)settings->servo_pulse;
	record[8] = (uint8_t)(settings->servo_pulse >> 8);
	record[9] = (uint8_t)settings->servo_min;
	record[10] = (uint8_t)(settings->servo_min >> 8);
	record[11] = (uint8_t)settings->servo_center;
	record[12] = (uint8_t)(settings->servo_center >> 8);
	record[13] = (uint8_t)settings->servo_max;
	record[14] = (uint8_t)(settings->servo_max >> 8);
	crc = Settings_CalculateCRC(record, SETTINGS_CRC_DATA_SIZE);
	record[15] = (uint8_t)crc;
	record[16] = (uint8_t)(crc >> 8);
}

static void Settings_UpdateCRC(SystemSettings *settings)
{
	uint8_t record[SETTINGS_RECORD_SIZE];

	Settings_Encode(settings, record);
	settings->crc = (uint16_t)record[15] | ((uint16_t)record[16] << 8);
}

static uint8_t Settings_Decode(const uint8_t record[SETTINGS_RECORD_SIZE],
	SystemSettings *settings)
{
	uint16_t version;
	uint16_t stored_crc;

	settings->magic = (uint32_t)record[0] |
		((uint32_t)record[1] << 8) |
		((uint32_t)record[2] << 16) |
		((uint32_t)record[3] << 24);
	version = (uint16_t)record[4] | ((uint16_t)record[5] << 8);
	settings->led_brightness = record[6];
	settings->servo_pulse = (uint16_t)record[7] |
		((uint16_t)record[8] << 8);
	settings->servo_min = (uint16_t)record[9] |
		((uint16_t)record[10] << 8);
	settings->servo_center = (uint16_t)record[11] |
		((uint16_t)record[12] << 8);
	settings->servo_max = (uint16_t)record[13] |
		((uint16_t)record[14] << 8);
	stored_crc = (uint16_t)record[15] | ((uint16_t)record[16] << 8);

	if(settings->magic != SETTINGS_MAGIC || version != SETTINGS_VERSION ||
		stored_crc != Settings_CalculateCRC(record, SETTINGS_CRC_DATA_SIZE))
	{
		return 0;
	}
	settings->version = version;
	settings->crc = stored_crc;
	return Settings_ParametersValid(settings);
}

static uint8_t Settings_RecordsEqual(const uint8_t *left,
	const uint8_t *right)
{
	uint8_t i;

	for(i = 0; i < SETTINGS_RECORD_SIZE; i++)
	{
		if(left[i] != right[i])
		{
			return 0;
		}
	}
	return 1;
}

void Settings_SetDefaults(SystemSettings *settings)
{
	if(settings == 0)
	{
		return;
	}

	settings->magic = SETTINGS_MAGIC;
	settings->version = SETTINGS_VERSION;
	settings->led_brightness = SETTINGS_DEFAULT_LED_BRIGHTNESS;
	settings->servo_pulse = SETTINGS_DEFAULT_SERVO_PULSE;
	settings->servo_min = SETTINGS_DEFAULT_SERVO_MIN;
	settings->servo_center = SETTINGS_DEFAULT_SERVO_CENTER;
	settings->servo_max = SETTINGS_DEFAULT_SERVO_MAX;
	Settings_UpdateCRC(settings);
}

uint8_t Settings_Load(void)
{
	uint8_t record[SETTINGS_RECORD_SIZE];
	SystemSettings loaded_settings;

	if(!settings_initialized || !settings_flash_supported ||
		!W25QXX_ReadData(SETTINGS_FLASH_ADDRESS, record, SETTINGS_RECORD_SIZE) ||
		!Settings_Decode(record, &loaded_settings))
	{
		Settings_SetDefaults(&current_settings);
		settings_dirty = 0;
		settings_valid = 0;
		return 0;
	}

	current_settings = loaded_settings;
	settings_dirty = 0;
	settings_valid = 1;
	return 1;
}

uint8_t Settings_Init(void)
{
	uint8_t jedec_id[3];

	settings_initialized = 1;
	settings_dirty = 0;
	settings_valid = 0;
	settings_flash_supported =
		W25QXX_ReadJEDECID(jedec_id) &&
		jedec_id[0] == 0xEF && jedec_id[1] == 0x40 && jedec_id[2] == 0x17;
	Settings_SetDefaults(&current_settings);

	return Settings_Load();
}

uint8_t Settings_Save(void)
{
	uint8_t record[SETTINGS_RECORD_SIZE];
	uint8_t verify_record[SETTINGS_RECORD_SIZE];
	uint8_t existing_record[SETTINGS_RECORD_SIZE];
	SystemSettings verified_settings;

	if(!settings_initialized || !settings_flash_supported ||
		!Settings_ParametersValid(&current_settings))
	{
		return 0;
	}

	current_settings.magic = SETTINGS_MAGIC;
	current_settings.version = SETTINGS_VERSION;
	Settings_Encode(&current_settings, record);
	current_settings.crc = (uint16_t)record[15] |
		((uint16_t)record[16] << 8);
	if(Settings_IsValid() &&
		W25QXX_ReadData(SETTINGS_FLASH_ADDRESS, existing_record,
			SETTINGS_RECORD_SIZE) &&
		Settings_RecordsEqual(record, existing_record))
	{
		settings_dirty = 0;
		return 1;
	}

	if(!W25QXX_EraseSector(SETTINGS_FLASH_ADDRESS) ||
		!W25QXX_WritePage(SETTINGS_FLASH_ADDRESS, record, SETTINGS_RECORD_SIZE) ||
		!W25QXX_ReadData(SETTINGS_FLASH_ADDRESS, verify_record,
			SETTINGS_RECORD_SIZE) ||
		!Settings_Decode(verify_record, &verified_settings) ||
		!Settings_RecordsEqual(record, verify_record))
	{
		settings_valid = 0;
		return 0;
	}

	current_settings = verified_settings;
	settings_dirty = 0;
	settings_valid = 1;
	return 1;
}

uint8_t Settings_ResetDefaults(void)
{
	if(!settings_initialized)
	{
		return 0;
	}
	Settings_SetDefaults(&current_settings);
	settings_dirty = 1;
	return 1;
}

uint8_t Settings_IsValid(void)
{
	return settings_valid;
}

uint8_t Settings_IsDirty(void)
{
	return settings_dirty;
}

void Settings_Get(SystemSettings *settings)
{
	if(settings != 0)
	{
		*settings = current_settings;
	}
}

uint8_t Settings_SetLedBrightness(uint8_t brightness)
{
	if(!settings_initialized || brightness > 100 ||
		current_settings.led_brightness == brightness)
	{
		return 0;
	}

	current_settings.led_brightness = brightness;
	Settings_UpdateCRC(&current_settings);
	settings_dirty = 1;
	return 1;
}

uint8_t Settings_SetServoPulse(uint16_t pulse_us)
{
	if(!settings_initialized ||
		pulse_us < current_settings.servo_min ||
		pulse_us > current_settings.servo_max ||
		current_settings.servo_pulse == pulse_us)
	{
		return 0;
	}

	current_settings.servo_pulse = pulse_us;
	Settings_UpdateCRC(&current_settings);
	settings_dirty = 1;
	return 1;
}
