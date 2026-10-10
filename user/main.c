#include "stm32f10x.h"
#include "delay.h"
#include "si2c.h"
#include "oled.h"
#include "button.h"
#include "menu_font.h"

static SI2C_TypeDef SI2C1 =
{
	GPIOB,
	GPIO_Pin_6,
	GPIOB,
	GPIO_Pin_7
};

static OLED_TypeDef OLED;
static Button_TypeDef ButtonUp;
static Button_TypeDef ButtonDown;
static Button_TypeDef ButtonEnter;
static uint8_t selected_item;
static uint8_t in_submenu;
static uint8_t screen_needs_refresh = 1;

static const char *menu_items[] = {
	"\351\243\216\346\211\207\346\216\247\345\210\266",
	"\346\221\207\346\235\206\346\225\260\346\215\256",
	"MPU6000"
};

// OLED 驱动使用的 I2C 写数据回调函数
static int OLED_I2C_Write(uint8_t addr, const uint8_t *pdata, uint16_t size)
{
	return My_SI2C_SendBytes(&SI2C1, addr, pdata, size);
}

static void OnButtonUp(uint8_t clicks)
{
	if(!in_submenu)
	{
		selected_item = (selected_item + 3 - clicks % 3) % 3;
		screen_needs_refresh = 1;
	}
}

static void OnButtonDown(uint8_t clicks)
{
	if(!in_submenu)
	{
		selected_item = (selected_item + clicks) % 3;
		screen_needs_refresh = 1;
	}
}

static void OnButtonEnter(uint8_t clicks)
{
	if(clicks > 0)
	{
		in_submenu = !in_submenu;
		screen_needs_refresh = 1;
	}
}

static void OLED_ShowWelcome(OLED_TypeDef *OLED)
{
	OLED_Clear(OLED);
	OLED_SetFont(OLED, &menu_font);
	OLED_SetPen(OLED, PEN_COLOR_WHITE, 1);
	OLED_SetBrush(OLED, BRUSH_TRANSPARENT);
	OLED_SetCursor(OLED, 32, 40);
	OLED_DrawString(OLED, "\346\254\242\350\277\216\344\275\277\347\224\250");
	OLED_SendBuffer(OLED);
}

static void OLED_ShowMenu(OLED_TypeDef *OLED)
{
	uint8_t i;
	int16_t row_y;
	uint16_t text_width;
	const char *item;

	OLED_Clear(OLED);
	OLED_SetFont(OLED, &menu_font);
	OLED_SetBrush(OLED, BRUSH_TRANSPARENT);

	if(in_submenu)
	{
		item = menu_items[selected_item];
		if(selected_item == 2)
		{
			OLED_SetFont(OLED, &default_font);
			text_width = OLED_GetStrWidth(OLED, item);
			OLED_SetCursor(OLED, (OLED_GetScreenWidth(OLED) - text_width) / 2, 24);
			OLED_DrawString(OLED, item);
			text_width = OLED_GetStrWidth(OLED, "PA6: BACK");
			OLED_SetCursor(OLED, (OLED_GetScreenWidth(OLED) - text_width) / 2, 48);
			OLED_DrawString(OLED, "PA6: BACK");
		}
		else
		{
			OLED_SetPen(OLED, PEN_COLOR_WHITE, 1);
			OLED_SetCursor(OLED, 32, 24);
			OLED_DrawString(OLED, item);
			OLED_SetFont(OLED, &default_font);
			text_width = OLED_GetStrWidth(OLED, "PA6: BACK");
			OLED_SetCursor(OLED, (OLED_GetScreenWidth(OLED) - text_width) / 2, 48);
			OLED_DrawString(OLED, "PA6: BACK");
		}
	}
	else
	{
		for(i = 0; i < 3; i++)
		{
			row_y = i * 21;
			if(i == selected_item)
			{
				OLED_SetPen(OLED, PEN_COLOR_TRANSPARENT, 1);
				OLED_SetBrush(OLED, BRUSH_WHITE);
				OLED_SetCursor(OLED, 0, row_y);
				OLED_DrawRect(OLED, 128, (i == 2) ? 22 : 21);
				OLED_SetBrush(OLED, BRUSH_TRANSPARENT);
				OLED_SetPen(OLED, PEN_COLOR_BLACK, 1);
			}
			else
			{
				OLED_SetPen(OLED, PEN_COLOR_WHITE, 1);
			}

			if(i == 2)
			{
				OLED_SetFont(OLED, &default_font);
				text_width = OLED_GetStrWidth(OLED, menu_items[i]);
				OLED_SetCursor(OLED, (OLED_GetScreenWidth(OLED) - text_width) / 2, row_y + 18);
			}
			else
			{
				OLED_SetFont(OLED, &menu_font);
				OLED_SetCursor(OLED, 32, row_y + 18);
			}
			OLED_DrawString(OLED, menu_items[i]);
		}
	}
	OLED_SendBuffer(OLED);
}

int main(void)
{
	OLED_InitTypeDef OLED_InitStruct =
	{
		OLED_I2C_Write
	};
	Button_InitTypeDef button_init = {0};

	Delay_Init();
	My_SI2C_Init(&SI2C1);
	OLED_Init(&OLED, &OLED_InitStruct);
	OLED_ShowWelcome(&OLED);

	button_init.GPIOx = GPIOB;
	button_init.GPIO_Pin = GPIO_Pin_11;
	button_init.button_clicked_cb = OnButtonUp;
	My_Button_Init(&ButtonUp, &button_init);

	button_init.GPIOx = GPIOB;
	button_init.GPIO_Pin = GPIO_Pin_1;
	button_init.button_clicked_cb = OnButtonDown;
	My_Button_Init(&ButtonDown, &button_init);

	button_init.GPIOx = GPIOA;
	button_init.GPIO_Pin = GPIO_Pin_6;
	button_init.button_clicked_cb = OnButtonEnter;
	My_Button_Init(&ButtonEnter, &button_init);

	Delay(3000);
	screen_needs_refresh = 1;

	while(1)
	{
		My_Button_Proc(&ButtonUp);
		My_Button_Proc(&ButtonDown);
		My_Button_Proc(&ButtonEnter);

		if(screen_needs_refresh)
		{
			OLED_ShowMenu(&OLED);
			screen_needs_refresh = 0;
		}

		Delay(5);
	}
}
