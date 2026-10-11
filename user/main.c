#include "delay.h"
#include "oled.h"
#include "menu.h"

int main(void)
{
	static OLED_TypeDef display;

	Delay_Init();
	if(BSP_OLED_Init(&display) != 0)
	{
		return -1;
	}
	Menu_Init(&display);

	while(1)
	{
		Menu_Process();
		Delay(5);
	}
}
