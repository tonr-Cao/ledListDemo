#include "menu.h"
#include "encoder.h"
#include "delay.h"
#include "menu_font.h"
#include "led_app.h"

#define MENU_ITEM_COUNT 3
#define MENU_ITEM_HEIGHT 21
#define MENU_WELCOME_DURATION_MS 3000
#define LED_BRIGHTNESS_SEGMENTS 13

static OLED_TypeDef *menu_display;
static Encoder_TypeDef menu_encoder;
static uint8_t selected_item;
static uint8_t in_submenu;
static uint8_t screen_needs_refresh;
static uint8_t menu_ready;
static uint32_t welcome_started_at;
static int16_t selector_y;

static const char *menu_items[MENU_ITEM_COUNT] = {
	"\351\243\216\346\211\207\346\216\247\345\210\266",
	"LED Control",
	"MPU6000"
};

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

	center_y = y + 10;
	OLED_SetPen(menu_display, PEN_COLOR_TRANSPARENT, 1);
	OLED_SetBrush(menu_display, BRUSH_WHITE);
	OLED_SetCursor(menu_display, 18, y);
	OLED_DrawRect(menu_display, 92, 20);
	OLED_SetCursor(menu_display, 18, center_y);
	OLED_DrawCircle(menu_display, 10);
	OLED_SetCursor(menu_display, 110, center_y);
	OLED_DrawCircle(menu_display, 10);
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
	OLED_SetCursor(menu_display, 22, 63);
	OLED_DrawString(menu_display, "Press = Back");
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
		if(selected_item == 1)
		{
			OLED_ShowLEDControl();
			OLED_SendBuffer(menu_display);
			return;
		}

		item = menu_items[selected_item];
		OLED_SetFont(menu_display, &default_font);
		OLED_SetPen(menu_display, PEN_COLOR_WHITE, 1);
		OLED_SetCursor(menu_display, 8, 8);
		OLED_DrawRect(menu_display, 112, 48);
		if(selected_item == 2)
		{
			text_width = OLED_GetStrWidth(menu_display, item);
			OLED_SetCursor(menu_display,
				(OLED_GetScreenWidth(menu_display) - text_width) / 2, 24);
			OLED_DrawString(menu_display, item);
		}
		else
		{
			OLED_SetFont(menu_display, &menu_font);
			text_width = OLED_GetStrWidth(menu_display, item);
			OLED_SetCursor(menu_display,
				(OLED_GetScreenWidth(menu_display) - text_width) / 2, 24);
			OLED_DrawString(menu_display, item);
			OLED_SetFont(menu_display, &default_font);
		}

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
			OLED_SetFont(menu_display, (i == 0) ? &menu_font : &default_font);
			text_width = OLED_GetStrWidth(menu_display, menu_items[i]);
			OLED_SetCursor(menu_display,
				(OLED_GetScreenWidth(menu_display) - text_width) / 2, row_y + 18);
			OLED_DrawString(menu_display, menu_items[i]);
		}
	}

	OLED_SendBuffer(menu_display);
}

void Menu_Init(OLED_TypeDef *display)
{
	menu_display = display;
	LED_App_Init();
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

	if(events.rotation != 0 && in_submenu && selected_item == 1)
	{
		if(LED_App_AdjustBrightness(events.rotation))
		{
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
		in_submenu = !in_submenu;
		screen_needs_refresh = 1;
	}

	selector_animating = OLED_UpdateSelector();
	if(screen_needs_refresh || selector_animating)
	{
		OLED_ShowMenu();
		screen_needs_refresh = 0;
	}
}
