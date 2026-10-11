#include "encoder.h"

static Encoder_TypeDef *active_encoder;

static uint8_t ReadPin(uint16_t pin)
{
	return GPIO_ReadInputDataBit(GPIOA, pin) == Bit_SET ? 1 : 0;
}

void BSP_Encoder_Init(Encoder_TypeDef *Encoder)
{
	GPIO_InitTypeDef gpio_init;
	uint8_t raw_clk;
	uint8_t raw_dt;
	uint8_t raw_switch;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	gpio_init.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &gpio_init);

	raw_clk = ReadPin(GPIO_Pin_0);
	raw_dt = ReadPin(GPIO_Pin_1);
	raw_switch = (uint8_t)!ReadPin(GPIO_Pin_2);
	Encoder->CLK_State = raw_clk;
	Encoder->CLK_Candidate = raw_clk;
	Encoder->DT_State = raw_dt;
	Encoder->DT_Candidate = raw_dt;
	Encoder->SW_State = raw_switch;
	Encoder->SW_Candidate = raw_switch;
	Encoder->SW_StableSamples = 0;
	Encoder->EncoderState = (Encoder->CLK_State << 1) | Encoder->DT_State;
	Encoder->RotationAccumulator = 0;
	Encoder->PendingRotationSteps = 0;
	Encoder->PendingPress = 0;
	Encoder->Tick = 0;
	Encoder->LastRotationActivity = 0;
	active_encoder = Encoder;
}

void BSP_Encoder_Tick(void)
{
	static const int8_t transition_table[16] =
	{
		 0, -1,  1,  0,
		 1,  0,  0, -1,
		-1,  0,  0,  1,
		 0,  1, -1,  0
	};
	Encoder_TypeDef *Encoder;
	uint8_t current_clk;
	uint8_t current_dt;
	uint8_t current_switch;
	uint8_t previous_state;
	uint8_t current_state;
	int8_t step;

	Encoder = active_encoder;
	if(Encoder == 0)
	{
		return;
	}

	Encoder->Tick++;
	current_clk = ReadPin(GPIO_Pin_0);
	current_dt = ReadPin(GPIO_Pin_1);
	current_state = (current_clk << 1) | current_dt;
	previous_state = Encoder->EncoderState;

	if(current_state != previous_state)
	{
		step = transition_table[(previous_state << 2) | current_state];
		Encoder->EncoderState = current_state;

		if((previous_state ^ current_state) == 3 || step == 0)
		{
			Encoder->RotationAccumulator = 0;
		}
		else
		{
			Encoder->LastRotationActivity = Encoder->Tick;
			Encoder->RotationAccumulator += step;
			if(Encoder->RotationAccumulator >= 4)
			{
				Encoder->RotationAccumulator = 0;
				if(Encoder->PendingRotationSteps < ENCODER_ROTATION_EVENT_LIMIT)
				{
					Encoder->PendingRotationSteps++;
				}
			}
			else if(Encoder->RotationAccumulator <= -4)
			{
				Encoder->RotationAccumulator = 0;
				if(Encoder->PendingRotationSteps > -ENCODER_ROTATION_EVENT_LIMIT)
				{
					Encoder->PendingRotationSteps--;
				}
			}
		}
	}

	current_switch = (uint8_t)!ReadPin(GPIO_Pin_2);
	if(current_switch != Encoder->SW_Candidate)
	{
		Encoder->SW_Candidate = current_switch;
		Encoder->SW_StableSamples = 1;
	}
	else if(Encoder->SW_StableSamples < ENCODER_BUTTON_DEBOUNCE_SAMPLES)
	{
		Encoder->SW_StableSamples++;
	}

	if(Encoder->SW_StableSamples >= ENCODER_BUTTON_DEBOUNCE_SAMPLES &&
		Encoder->SW_State != Encoder->SW_Candidate)
	{
		Encoder->SW_State = Encoder->SW_Candidate;
		if(Encoder->SW_State &&
			(uint32_t)(Encoder->Tick - Encoder->LastRotationActivity) >=
				ENCODER_BUTTON_ROTATION_GUARD_MS)
		{
			Encoder->PendingPress = 1;
		}
	}
}

void BSP_Encoder_Process(Encoder_TypeDef *Encoder, Encoder_EventTypeDef *Events)
{
	uint32_t interrupt_state;

	Events->rotation = 0;
	Events->pressed = 0;

	interrupt_state = __get_PRIMASK();
	__disable_irq();
	if(Encoder->PendingRotationSteps > 0)
	{
		Encoder->PendingRotationSteps--;
		Events->rotation = ENCODER_ROTATE_RIGHT;
	}
	else if(Encoder->PendingRotationSteps < 0)
	{
		Encoder->PendingRotationSteps++;
		Events->rotation = ENCODER_ROTATE_LEFT;
	}
	if(Encoder->PendingPress)
	{
		Encoder->PendingPress = 0;
		Events->pressed = 1;
	}
	if((interrupt_state & 1U) == 0)
	{
		__enable_irq();
	}
}
