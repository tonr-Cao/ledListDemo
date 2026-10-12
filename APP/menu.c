#include "menu.h"
#include "encoder.h"
#include "delay.h"
#include "menu_font.h"
#include "led_app.h"
#include "adc.h"
#include "servo_pwm.h"
#include "w25qxx.h"
#include "settings.h"

#define MENU_ITEM_COUNT 4
#define MENU_ITEM_HEIGHT 12
#define MENU_WELCOME_DURATION_MS 3000
#define SETTINGS_SAVE_DEBOUNCE_MS 2000
#define SETTINGS_SAVE_RETRY_MS 10000
#define LED_SAVE_FEEDBACK_MS 1500
#define LED_SAVE_FEEDBACK_NONE 0
#define LED_SAVE_FEEDBACK_SAVED 1
#define LED_SAVE_FEEDBACK_ERROR 2
#define SETTINGS_VIEW_MENU 0
#define SETTINGS_VIEW_FLASH_INFO 1
#define SETTINGS_VIEW_SYSTEM_INFO 2
#define SETTINGS_VIEW_RESET_CONFIRM 3
#define SETTINGS_VIEW_MESSAGE 4
#define SETTINGS_ACTION_COUNT 6
#define SETTINGS_MESSAGE_LOADED 3
#define SETTINGS_MESSAGE_DEFAULTS_LOADED 5
#define LED_BRIGHTNESS_SEGMENTS 13
#define ADC_GRAPH_SAMPLE_COUNT 20
#define ADC_GRAPH_SAMPLE_INTERVAL_MS 100
#define ADC_GRAPH_LEFT 27
#define ADC_GRAPH_RIGHT 126
#define ADC_GRAPH_TOP 8
#define ADC_GRAPH_BOTTOM 43
#define SERVO_PULSE_STEP_US 10
#define SERVO_GRAPH_LEFT 14
#define SERVO_GRAPH_RIGHT 114

static OLED_TypeDef *menu_display;
static Encoder_TypeDef menu_encoder;
static uint16_t adc_samples[ADC_GRAPH_SAMPLE_COUNT];
static uint16_t servo_pulse_us;
static uint8_t flash_jedec_id[3];
static uint8_t flash_connected;
static uint8_t settings_save_pending;
static uint8_t led_save_feedback;
static uint8_t settings_view;
static uint8_t settings_selected_item;
static uint8_t reset_selected_yes;
static uint8_t settings_message;
static uint32_t adc_last_sample_at;
static uint32_t settings_last_change_at;
static uint32_t settings_next_save_at;
static uint32_t led_save_feedback_until;
static uint32_t settings_message_until;
static uint8_t adc_samples_initialized;
static uint8_t selected_item;
static uint8_t in_submenu;
static uint8_t screen_needs_refresh;
static uint8_t menu_ready;
static uint32_t welcome_started_at;
static int16_t selector_y;

static const char *menu_items[MENU_ITEM_COUNT] = {
	"ADC Monitor",
	"LED Control",
	"Servo Tester",
	"Settings"
};

static const char *settings_items[SETTINGS_ACTION_COUNT] = {
	"Save Settings",
	"Load Settings",
	"Reset Defaults",
	"Flash Info",
	"System Info",
	"Back"
};

static void Menu_ApplySettings(void)
{
	SystemSettings settings;

	Settings_Get(&settings);
	LED_App_SetBrightness(settings.led_brightness);
	if(BSP_ServoPWM_SetLimits(settings.servo_min, settings.servo_center,
		settings.servo_max))
	{
		servo_pulse_us = settings.servo_pulse;
		BSP_ServoPWM_SetPulse(servo_pulse_us);
	}
}

static void Menu_MarkSettingsChanged(void)
{
	uint32_t now;

	now = GetTick();
	settings_save_pending = 1;
	settings_last_change_at = now;
	settings_next_save_at = now + SETTINGS_SAVE_DEBOUNCE_MS;
	led_save_feedback = LED_SAVE_FEEDBACK_NONE;
}

static void OLED_ShowWelcome(void)
{
	OLED_Clear(menu_display);
	OLED_SetFont(menu_display, &menu_font);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
	OLED_SetCursor(menu_display, 32, 40);
	OLED_DrawString(menu_display, "\346\254\242\350\277\216\344\275\277\347\224\250");
	OLED_SendBuffer(menu_display);
}

static void OLED_DrawSelectionPill(int16_t y)
{
	int16_t center_y;

	center_y = y + 7;
	OLED_SetPen(menu_display, PEN_COLOR_TRANSPARENT, 1);
	OLED_SetBrush(menu_display, BRUSH_WHITE);
	OLED_SetCursor(menu_display, 18, y);
	OLED_DrawRect(menu_display, 92, 15);
	OLED_SetCursor(menu_display, 18, center_y);
	OLED_DrawCircle(menu_display, 7);
	OLED_SetCursor(menu_display, 110, center_y);
	OLED_DrawCircle(menu_display, 7);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
}

static void OLED_ShowLEDControl(void)
{
	uint8_t i;
	uint8_t brightness;
	uint8_t filled_segments;
	char brightness_text[5];
	uint16_t text_width;
	int16_t segment_x;

	brightness = LED_App_GetBrightness();
	filled_segments = (brightness * LED_BRIGHTNESS_SEGMENTS + 50) / 100;
	if(brightness == 100)
	{
		brightness_text[0] = '1';
		brightness_text[1] = '0';
		brightness_text[2] = '0';
		brightness_text[3] = '%';
		brightness_text[4] = '\0';
	}
	else if(brightness >= 10)
	{
		brightness_text[0] = '0' + brightness / 10;
		brightness_text[1] = '0' + brightness % 10;
		brightness_text[2] = '%';
		brightness_text[3] = '\0';
	}
	else
	{
		brightness_text[0] = '0' + brightness;
		brightness_text[1] = '%';
		brightness_text[2] = '\0';
	}

	OLED_SetFont(menu_display, &default_font);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	text_width = OLED_GetStrWidth(menu_display, "LED CONTROL");
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - text_width) / 2, 12);
	OLED_DrawString(menu_display, "LED CONTROL");

	OLED_SetPen(menu_display, PEN_COLOR_TRANSPARENT, 1);
	OLED_SetBrush(menu_display, BRUSH_WHITE);
	OLED_SetCursor(menu_display, 8, 16);
	OLED_DrawRect(menu_display, 112, 1);

	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetCursor(menu_display, 8, 30);
	OLED_DrawString(menu_display, "Brightness");

	text_width = OLED_GetStrWidth(menu_display, brightness_text);
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - text_width) / 2, 44);
	OLED_DrawString(menu_display, brightness_text);

	OLED_SetCursor(menu_display, 14, 56);
	OLED_DrawString(menu_display, "[");
	for(i = 0; i < LED_BRIGHTNESS_SEGMENTS; i++)
	{
		segment_x = 25 + i * 6;
		if(i < filled_segments)
		{
			OLED_SetPen(menu_display, PEN_COLOR_TRANSPARENT, 1);
			OLED_SetBrush(menu_display, BRUSH_WHITE);
			OLED_SetCursor(menu_display, segment_x, 49);
			OLED_DrawRect(menu_display, 4, 7);
		}
		else
		{
			OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
			OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
			OLED_SetCursor(menu_display, segment_x, 52);
			OLED_DrawRect(menu_display, 4, 1);
		}
	}

	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
	OLED_SetCursor(menu_display, 105, 56);
	OLED_DrawString(menu_display, "]");
	if(led_save_feedback != LED_SAVE_FEEDBACK_NONE &&
		(int32_t)(GetTick() - led_save_feedback_until) < 0)
	{
		text_width = OLED_GetStrWidth(menu_display,
			led_save_feedback == LED_SAVE_FEEDBACK_SAVED ?
				"SAVED" : "SAVE ERROR");
		OLED_SetCursor(menu_display,
			(OLED_GetScreenWidth(menu_display) - text_width) / 2, 63);
		OLED_DrawString(menu_display,
			led_save_feedback == LED_SAVE_FEEDBACK_SAVED ?
				"SAVED" : "SAVE ERROR");
	}
	else
	{
		OLED_SetCursor(menu_display, 22, 63);
		OLED_DrawString(menu_display, "Press = Back");
	}
}

static void OLED_ShowADCMonitor(void)
{
	uint8_t i;
	uint16_t text_width;
	int16_t sample_x;
	int16_t sample_y;
	int16_t previous_x;
	int16_t previous_y;

	OLED_SetFont(menu_display, &default_font);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	OLED_SetCursor(menu_display, 3, 12);
	OLED_DrawString(menu_display, "3.3V");
	OLED_SetCursor(menu_display, 0, 51);
	OLED_DrawString(menu_display, "0.0V");

	OLED_SetCursor(menu_display, ADC_GRAPH_LEFT, ADC_GRAPH_TOP);
	OLED_DrawLine(menu_display, ADC_GRAPH_LEFT, ADC_GRAPH_BOTTOM);
	OLED_SetCursor(menu_display, ADC_GRAPH_LEFT, ADC_GRAPH_BOTTOM);
	OLED_DrawLine(menu_display, ADC_GRAPH_RIGHT, ADC_GRAPH_BOTTOM);

	for(i = 0; i < ADC_GRAPH_SAMPLE_COUNT; i++)
	{
		sample_x = ADC_GRAPH_LEFT + (ADC_GRAPH_RIGHT - ADC_GRAPH_LEFT) * i /
			(ADC_GRAPH_SAMPLE_COUNT - 1);
		sample_y = ADC_GRAPH_BOTTOM -
			((uint32_t)adc_samples[i] *
				(ADC_GRAPH_BOTTOM - ADC_GRAPH_TOP) + 2047) / 4095;

		if(i > 0)
		{
			OLED_SetCursor(menu_display, previous_x, previous_y);
			OLED_DrawLine(menu_display, sample_x, sample_y);
		}
		previous_x = sample_x;
		previous_y = sample_y;
	}

	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
	text_width = OLED_GetStrWidth(menu_display, "RECENT SAMPLES");
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - text_width) / 2, 63);
	OLED_DrawString(menu_display, "RECENT SAMPLES");

	OLED_SetCursor(menu_display, 6, 58);
	OLED_DrawLine(menu_display, 14, 58);
	OLED_SetCursor(menu_display, 6, 58);
	OLED_DrawLine(menu_display, 9, 55);
	OLED_SetCursor(menu_display, 6, 58);
	OLED_DrawLine(menu_display, 9, 61);
	OLED_SetCursor(menu_display, 122, 58);
	OLED_DrawLine(menu_display, 114, 58);
	OLED_SetCursor(menu_display, 122, 58);
	OLED_DrawLine(menu_display, 119, 55);
	OLED_SetCursor(menu_display, 122, 58);
	OLED_DrawLine(menu_display, 119, 61);
}

static void OLED_ShowServoTester(void)
{
	SystemSettings settings;
	uint16_t title_width;
	uint16_t angle_degrees;
	uint16_t indicator_x;

	Settings_Get(&settings);
	if(servo_pulse_us <= settings.servo_center)
	{
		angle_degrees = (uint16_t)
			((servo_pulse_us - settings.servo_min) * 90UL /
				(settings.servo_center - settings.servo_min));
	}
	else
	{
		angle_degrees = 90 + (uint16_t)
			((servo_pulse_us - settings.servo_center) * 90UL /
				(settings.servo_max - settings.servo_center));
	}
	indicator_x = SERVO_GRAPH_LEFT +
		(servo_pulse_us - settings.servo_min) *
		(SERVO_GRAPH_RIGHT - SERVO_GRAPH_LEFT) /
		(settings.servo_max - settings.servo_min);

	OLED_SetFont(menu_display, &default_font);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	title_width = OLED_GetStrWidth(menu_display, "SERVO TESTER");
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - title_width) / 2, 1);
	OLED_DrawString(menu_display, "SERVO TESTER");

	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
	OLED_SetCursor(menu_display, 12, 19);
	OLED_Printf(menu_display, "Pulse: %4u us", (unsigned int)servo_pulse_us);
	OLED_SetCursor(menu_display, 12, 30);
	OLED_Printf(menu_display, "Angle: %3u deg", (unsigned int)angle_degrees);

	OLED_SetCursor(menu_display, SERVO_GRAPH_LEFT, 45);
	OLED_DrawLine(menu_display, SERVO_GRAPH_RIGHT, 45);
	OLED_SetCursor(menu_display, SERVO_GRAPH_LEFT, 43);
	OLED_DrawLine(menu_display, SERVO_GRAPH_LEFT, 47);
	OLED_SetCursor(menu_display, SERVO_GRAPH_LEFT +
		(SERVO_GRAPH_RIGHT - SERVO_GRAPH_LEFT) / 2, 43);
	OLED_DrawLine(menu_display,
		SERVO_GRAPH_LEFT + (SERVO_GRAPH_RIGHT - SERVO_GRAPH_LEFT) / 2, 47);
	OLED_SetCursor(menu_display, SERVO_GRAPH_RIGHT, 43);
	OLED_DrawLine(menu_display, SERVO_GRAPH_RIGHT, 47);
	OLED_SetCursor(menu_display, indicator_x, 41);
	OLED_DrawLine(menu_display, indicator_x, 49);

	OLED_SetCursor(menu_display, SERVO_GRAPH_LEFT, 54);
	OLED_DrawString(menu_display, "0");
	OLED_SetCursor(menu_display,
		SERVO_GRAPH_LEFT + (SERVO_GRAPH_RIGHT - SERVO_GRAPH_LEFT) / 2 - 6, 54);
	OLED_DrawString(menu_display, "90");
	OLED_SetCursor(menu_display, SERVO_GRAPH_RIGHT - 18, 54);
	OLED_DrawString(menu_display, "180");

	title_width = OLED_GetStrWidth(menu_display, "PRESS: BACK");
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - title_width) / 2, 63);
	OLED_DrawString(menu_display, "PRESS: BACK");
}

static void OLED_ShowSettings(void)
{
	uint8_t i;
	uint16_t text_width;
	const char *title;
	const char *message;

	OLED_SetFont(menu_display, &default_font);
	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	if(settings_view == SETTINGS_VIEW_FLASH_INFO)
	{
		title = "FLASH INFO";
	}
	else if(settings_view == SETTINGS_VIEW_SYSTEM_INFO)
	{
		title = "SYSTEM INFO";
	}
	else if(settings_view == SETTINGS_VIEW_RESET_CONFIRM)
	{
		title = "RESET DEFAULTS";
	}
	else
	{
		title = "SETTINGS";
	}
	text_width = OLED_GetStrWidth(menu_display, title);
	OLED_SetCursor(menu_display,
		(OLED_GetScreenWidth(menu_display) - text_width) / 2, 1);
	OLED_DrawString(menu_display, title);

	OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	if(settings_view == SETTINGS_VIEW_MENU)
	{
		for(i = 0; i < SETTINGS_ACTION_COUNT; i++)
		{
			if(i == settings_selected_item)
			{
				OLED_SetPen(menu_display, PEN_COLOR_TRANSPARENT, 1);
				OLED_SetBrush(menu_display, BRUSH_WHITE);
				OLED_SetCursor(menu_display, 3, 11 + i * 8);
				OLED_DrawRect(menu_display, 122, 8);
				OLED_SetCursor(menu_display, 7, 15 + i * 8);
				OLED_DrawCircle(menu_display, 4);
				OLED_SetCursor(menu_display, 121, 15 + i * 8);
				OLED_DrawCircle(menu_display, 4);
			}
			OLED_SetPen(menu_display,
				(i == settings_selected_item) ?
					PEN_COLOR_BLACK : PEN_COLOR_WHITE, 1);
			OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);
			text_width = OLED_GetStrWidth(menu_display, settings_items[i]);
			OLED_SetCursor(menu_display,
				(OLED_GetScreenWidth(menu_display) - text_width) / 2,
				19 + i * 8);
			OLED_DrawString(menu_display, settings_items[i]);
		}
		if(led_save_feedback != LED_SAVE_FEEDBACK_NONE &&
			(int32_t)(GetTick() - led_save_feedback_until) < 0)
		{
			OLED_SetCursor(menu_display, 92, 59);
			OLED_DrawString(menu_display,
				led_save_feedback == LED_SAVE_FEEDBACK_SAVED ?
					"SAVED" : "ERROR");
		}
	}
	else if(settings_view == SETTINGS_VIEW_FLASH_INFO)
	{
		if(flash_connected)
		{
			OLED_SetCursor(menu_display, 6, 17);
			OLED_DrawString(menu_display, "Chip: W25Q64");
			OLED_SetCursor(menu_display, 6, 26);
			OLED_Printf(menu_display, "JEDEC: %02X%02X%02X",
				(unsigned int)flash_jedec_id[0],
				(unsigned int)flash_jedec_id[1],
				(unsigned int)flash_jedec_id[2]);
			OLED_SetCursor(menu_display, 6, 35);
			OLED_DrawString(menu_display, "Size: 8 MB");
		}
		else
		{
			OLED_SetCursor(menu_display, 6, 17);
			OLED_DrawString(menu_display, "Chip: Unknown");
			OLED_SetCursor(menu_display, 6, 26);
			OLED_DrawString(menu_display, "JEDEC: --------");
			OLED_SetCursor(menu_display, 6, 35);
			OLED_DrawString(menu_display, "Size: Unknown");
		}
		OLED_SetCursor(menu_display, 6, 47);
		OLED_DrawString(menu_display,
			Settings_IsValid() ? "Settings: VALID" : "Settings: INVALID");
		OLED_SetCursor(menu_display, 6, 56);
		OLED_Printf(menu_display, "Version: %u", SETTINGS_VERSION);
	}
	else if(settings_view == SETTINGS_VIEW_SYSTEM_INFO)
	{
		OLED_SetCursor(menu_display, 6, 17);
		OLED_DrawString(menu_display, "MCU: STM32F103");
		OLED_SetCursor(menu_display, 6, 26);
		OLED_DrawString(menu_display, "FW: v1.0");
		OLED_SetCursor(menu_display, 6, 37);
		OLED_Printf(menu_display, "LED: %3u%%",
			(unsigned int)LED_App_GetBrightness());
		OLED_SetCursor(menu_display, 6, 46);
		OLED_Printf(menu_display, "Servo: %4u us",
			(unsigned int)servo_pulse_us);
		OLED_SetCursor(menu_display, 6, 55);
		OLED_DrawString(menu_display,
			flash_connected ? "Flash: OK" : "Flash: ERROR");
	}
	else if(settings_view == SETTINGS_VIEW_RESET_CONFIRM)
	{
		OLED_SetCursor(menu_display, 7, 20);
		OLED_DrawString(menu_display, "Reset all settings?");
		OLED_SetCursor(menu_display, 24, 38);
		OLED_DrawString(menu_display, reset_selected_yes ? "  NO" : "> NO");
		OLED_SetCursor(menu_display, 24, 49);
		OLED_DrawString(menu_display, reset_selected_yes ? "> YES" : "  YES");
	}
	else
	{
		if(settings_message == LED_SAVE_FEEDBACK_SAVED)
		{
			message = "SETTINGS SAVED";
		}
		else if(settings_message == SETTINGS_MESSAGE_LOADED)
		{
			message = "SETTINGS LOADED";
		}
		else if(settings_message == SETTINGS_MESSAGE_DEFAULTS_LOADED)
		{
			message = "DEFAULTS LOADED";
		}
		else if(settings_message == 4)
		{
			message = "RESET COMPLETE";
		}
		else
		{
			message = "SAVE ERROR";
		}
		text_width = OLED_GetStrWidth(menu_display, message);
		OLED_SetCursor(menu_display,
			(OLED_GetScreenWidth(menu_display) - text_width) / 2, 34);
		OLED_DrawString(menu_display, message);
	}
}

static uint8_t OLED_UpdateSelector(void)
{
	int16_t target_y;
	int16_t delta;
	int16_t step;

	if(in_submenu)
	{
		return 0;
	}

	target_y = selected_item * MENU_ITEM_HEIGHT;
	delta = target_y - selector_y;
	if(delta == 0)
	{
		return 0;
	}

	step = delta / 3;
	if(step == 0)
	{
		step = (delta > 0) ? 1 : -1;
	}
	selector_y += step;
	return 1;
}

static void OLED_ShowMenu(void)
{
	uint8_t i;
	int16_t row_y;
	uint16_t text_width;
	const char *item;

	OLED_Clear(menu_display);
	OLED_SetFont(menu_display, &menu_font);
	OLED_SetBrush(menu_display, BRUSH_TRANSPARENT);

	if(in_submenu)
	{
		if(selected_item == 0)
		{
			OLED_ShowADCMonitor();
			OLED_SendBuffer(menu_display);
			return;
		}
		else if(selected_item == 1)
		{
			OLED_ShowLEDControl();
			OLED_SendBuffer(menu_display);
			return;
		}
		else if(selected_item == 2)
		{
			OLED_ShowServoTester();
			OLED_SendBuffer(menu_display);
			return;
		}
		else if(selected_item == 3)
		{
			OLED_ShowSettings();
			OLED_SendBuffer(menu_display);
			return;
		}

		item = menu_items[selected_item];
		OLED_SetFont(menu_display, &default_font);
		OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
		OLED_SetCursor(menu_display, 8, 8);
		OLED_DrawRect(menu_display, 112, 48);
		text_width = OLED_GetStrWidth(menu_display, item);
		OLED_SetCursor(menu_display,
			(OLED_GetScreenWidth(menu_display) - text_width) / 2, 24);
		OLED_DrawString(menu_display, item);

		text_width = OLED_GetStrWidth(menu_display, "PRESS: BACK");
		OLED_SetCursor(menu_display,
			(OLED_GetScreenWidth(menu_display) - text_width) / 2, 48);
		OLED_DrawString(menu_display, "PRESS: BACK");
	}
	else
	{
		OLED_DrawSelectionPill(selector_y);
		for(i = 0; i < MENU_ITEM_COUNT; i++)
		{
			row_y = i * MENU_ITEM_HEIGHT;
			OLED_SetPen(menu_display,
				(i == selected_item) ? PEN_COLOR_BLACK : PEN_COLOR_WHITE, 1);
			OLED_SetFont(menu_display, &default_font);
			text_width = OLED_GetStrWidth(menu_display, menu_items[i]);
			OLED_SetCursor(menu_display,
				(OLED_GetScreenWidth(menu_display) - text_width) / 2, row_y + 11);
			OLED_DrawString(menu_display, menu_items[i]);
		}
	}

	OLED_SendBuffer(menu_display);
}

void Menu_Init(OLED_TypeDef *display)
{
	SystemSettings settings;

	menu_display = display;
	W25QXX_Init();
	Settings_Init();
	Settings_Get(&settings);
	LED_App_Init(settings.led_brightness);
	BSP_ADC_Init();
	BSP_ServoPWM_Init();
	BSP_ServoPWM_SetLimits(settings.servo_min, settings.servo_center,
		settings.servo_max);
	servo_pulse_us = settings.servo_pulse;
	BSP_ServoPWM_SetPulse(servo_pulse_us);
	flash_connected = W25QXX_ReadJEDECID(flash_jedec_id) &&
		W25QXX_IsConnected(flash_jedec_id);
	settings_save_pending = 0;
	led_save_feedback = LED_SAVE_FEEDBACK_NONE;
	settings_view = SETTINGS_VIEW_MENU;
	settings_selected_item = 0;
	reset_selected_yes = 0;
	adc_samples_initialized = 0;
	adc_last_sample_at = 0;
	selected_item = 0;
	in_submenu = 0;
	screen_needs_refresh = 0;
	menu_ready = 0;
	selector_y = 0;
	welcome_started_at = GetTick();
	BSP_Encoder_Init(&menu_encoder);
	OLED_ShowWelcome();
}

void Menu_Process(void)
{
	Encoder_EventTypeDef events;
	uint8_t selector_animating;
	uint8_t servo_changed;
	uint32_t now;

	servo_changed = 0;
	BSP_Encoder_Process(&menu_encoder, &events);
	if(!menu_ready)
	{
		if((uint32_t)(GetTick() - welcome_started_at) <
			MENU_WELCOME_DURATION_MS)
		{
			return;
		}
		menu_ready = 1;
		screen_needs_refresh = 1;
	}

	if(in_submenu && selected_item == 3 &&
		settings_view == SETTINGS_VIEW_MENU && events.rotation != 0)
	{
		if(events.rotation == ENCODER_ROTATE_LEFT)
		{
			settings_selected_item = (settings_selected_item +
				SETTINGS_ACTION_COUNT - 1) % SETTINGS_ACTION_COUNT;
		}
		else
		{
			settings_selected_item = (settings_selected_item + 1) %
				SETTINGS_ACTION_COUNT;
		}
		screen_needs_refresh = 1;
	}
	else if(in_submenu && selected_item == 3 &&
		settings_view == SETTINGS_VIEW_RESET_CONFIRM && events.rotation != 0)
	{
		reset_selected_yes = !reset_selected_yes;
		screen_needs_refresh = 1;
	}
	else if(events.rotation != 0 && in_submenu && selected_item == 1)
	{
		if(LED_App_AdjustBrightness(events.rotation))
		{
			now = GetTick();
			Settings_SetLedBrightness(LED_App_GetBrightness());
			Menu_MarkSettingsChanged();
			screen_needs_refresh = 1;
		}
	}
	else if(events.rotation != 0 && in_submenu && selected_item == 2)
	{
		if(events.rotation == ENCODER_ROTATE_RIGHT)
		{
			SystemSettings settings;

			Settings_Get(&settings);
			if(servo_pulse_us <= settings.servo_max - SERVO_PULSE_STEP_US)
			{
				servo_pulse_us += SERVO_PULSE_STEP_US;
				servo_changed = 1;
			}
		}
		else
		{
			SystemSettings settings;

			Settings_Get(&settings);
			if(servo_pulse_us >= settings.servo_min + SERVO_PULSE_STEP_US)
			{
				servo_pulse_us -= SERVO_PULSE_STEP_US;
				servo_changed = 1;
			}
		}
		if(servo_changed)
		{
			BSP_ServoPWM_SetPulse(servo_pulse_us);
			Settings_SetServoPulse(servo_pulse_us);
			Menu_MarkSettingsChanged();
			screen_needs_refresh = 1;
		}
	}
	else if(events.rotation != 0 && !in_submenu)
	{
		if(events.rotation == ENCODER_ROTATE_LEFT)
		{
			selected_item = (selected_item + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
		}
		else
		{
			selected_item = (selected_item + 1) % MENU_ITEM_COUNT;
		}
		screen_needs_refresh = 1;
	}

	if(events.pressed)
	{
		if(in_submenu && selected_item == 3)
		{
			if(settings_view == SETTINGS_VIEW_MENU)
			{
				if(settings_selected_item == 0)
				{
					settings_message = Settings_Save() ?
						LED_SAVE_FEEDBACK_SAVED : LED_SAVE_FEEDBACK_ERROR;
					settings_view = SETTINGS_VIEW_MESSAGE;
					settings_message_until = GetTick() + LED_SAVE_FEEDBACK_MS;
					settings_save_pending = Settings_IsDirty();
					if(settings_save_pending)
					{
						settings_next_save_at = GetTick() +
							SETTINGS_SAVE_RETRY_MS;
					}
				}
				else if(settings_selected_item == 1)
				{
					settings_message = Settings_Load() ?
						SETTINGS_MESSAGE_LOADED :
						SETTINGS_MESSAGE_DEFAULTS_LOADED;
					Menu_ApplySettings();
					settings_save_pending = 0;
					settings_view = SETTINGS_VIEW_MESSAGE;
					settings_message_until = GetTick() + LED_SAVE_FEEDBACK_MS;
				}
				else if(settings_selected_item == 2)
				{
					settings_view = SETTINGS_VIEW_RESET_CONFIRM;
					reset_selected_yes = 0;
				}
				else if(settings_selected_item == 3)
				{
					flash_connected = W25QXX_ReadJEDECID(flash_jedec_id) &&
						W25QXX_IsConnected(flash_jedec_id);
					settings_view = SETTINGS_VIEW_FLASH_INFO;
				}
				else if(settings_selected_item == 4)
				{
					settings_view = SETTINGS_VIEW_SYSTEM_INFO;
				}
				else
				{
					in_submenu = 0;
				}
			}
			else if(settings_view == SETTINGS_VIEW_RESET_CONFIRM)
			{
				if(reset_selected_yes)
				{
					if(Settings_ResetDefaults())
					{
						Menu_ApplySettings();
						if(Settings_Save())
						{
							settings_save_pending = 0;
							settings_message = 4;
						}
						else
						{
							settings_save_pending = 1;
							settings_last_change_at = GetTick();
							settings_next_save_at = GetTick() +
								SETTINGS_SAVE_RETRY_MS;
							settings_message = LED_SAVE_FEEDBACK_ERROR;
						}
					}
					else
					{
						settings_message = LED_SAVE_FEEDBACK_ERROR;
					}
					settings_view = SETTINGS_VIEW_MESSAGE;
					settings_message_until = GetTick() + LED_SAVE_FEEDBACK_MS;
				}
				else
				{
					settings_view = SETTINGS_VIEW_MENU;
				}
			}
			else if(settings_view == SETTINGS_VIEW_FLASH_INFO ||
				settings_view == SETTINGS_VIEW_SYSTEM_INFO)
			{
				settings_view = SETTINGS_VIEW_MENU;
			}
			else
			{
				settings_view = SETTINGS_VIEW_MENU;
			}
			screen_needs_refresh = 1;
		}
		else
		{
			in_submenu = !in_submenu;
			if(in_submenu && selected_item == 0)
			{
				adc_samples_initialized = 0;
			}
			else if(in_submenu && selected_item == 3)
			{
				settings_view = SETTINGS_VIEW_MENU;
			}
			screen_needs_refresh = 1;
		}
	}

	if(settings_save_pending &&
		(uint32_t)(GetTick() - settings_last_change_at) >=
			SETTINGS_SAVE_DEBOUNCE_MS &&
		(int32_t)(GetTick() - settings_next_save_at) >= 0)
	{
		now = GetTick();
		if(Settings_Save())
		{
			settings_save_pending = 0;
			led_save_feedback = LED_SAVE_FEEDBACK_SAVED;
			led_save_feedback_until = now + LED_SAVE_FEEDBACK_MS;
		}
		else
		{
			led_save_feedback = LED_SAVE_FEEDBACK_ERROR;
			led_save_feedback_until = now + LED_SAVE_FEEDBACK_MS;
			settings_next_save_at = now + SETTINGS_SAVE_RETRY_MS;
		}
		if((in_submenu && selected_item == 1) ||
			(in_submenu && selected_item == 3 &&
				settings_view == SETTINGS_VIEW_MENU))
		{
			screen_needs_refresh = 1;
		}
	}

	if(led_save_feedback != LED_SAVE_FEEDBACK_NONE &&
		(int32_t)(GetTick() - led_save_feedback_until) >= 0)
	{
		led_save_feedback = LED_SAVE_FEEDBACK_NONE;
		if((in_submenu && selected_item == 1) ||
			(in_submenu && selected_item == 3 &&
				settings_view == SETTINGS_VIEW_MENU))
		{
			screen_needs_refresh = 1;
		}
	}

	if(settings_view == SETTINGS_VIEW_MESSAGE &&
		(int32_t)(GetTick() - settings_message_until) >= 0)
	{
		settings_view = SETTINGS_VIEW_MENU;
		screen_needs_refresh = 1;
	}

	if(in_submenu && selected_item == 0)
	{
		uint16_t new_adc_value;
		uint8_t i;

		if(!adc_samples_initialized ||
			(uint32_t)(GetTick() - adc_last_sample_at) >=
				ADC_GRAPH_SAMPLE_INTERVAL_MS)
		{
			new_adc_value = BSP_ADC_ReadChannel3();
			if(!adc_samples_initialized)
			{
				for(i = 0; i < ADC_GRAPH_SAMPLE_COUNT; i++)
				{
					adc_samples[i] = new_adc_value;
				}
				adc_samples_initialized = 1;
			}
			else
			{
				for(i = 0; i < ADC_GRAPH_SAMPLE_COUNT - 1; i++)
				{
					adc_samples[i] = adc_samples[i + 1];
				}
				adc_samples[ADC_GRAPH_SAMPLE_COUNT - 1] = new_adc_value;
			}
			adc_last_sample_at = GetTick();
			screen_needs_refresh = 1;
		}
	}

	selector_animating = OLED_UpdateSelector();
	if(screen_needs_refresh || selector_animating)
	{
		OLED_ShowMenu();
		screen_needs_refresh = 0;
	}
}
