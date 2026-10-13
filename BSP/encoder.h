#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include "stm32f1xx_hal.h"

#define ENCODER_ROTATE_LEFT  (-1)
#define ENCODER_ROTATE_RIGHT (1)

#define ENCODER_BUTTON_DEBOUNCE_SAMPLES 30
#define ENCODER_BUTTON_ROTATION_GUARD_MS 100
#define ENCODER_ROTATION_EVENT_LIMIT 127

typedef struct
{
	int8_t rotation;
	uint8_t pressed;
} Encoder_EventTypeDef;

typedef struct
{
	uint8_t CLK_Candidate;
	uint8_t CLK_State;
	uint32_t CLK_ChangedAt;
	uint8_t DT_Candidate;
	uint8_t DT_State;
	uint32_t DT_ChangedAt;
	uint8_t SW_Candidate;
	uint8_t SW_State;
	uint8_t SW_StableSamples;
	uint8_t EncoderState;
	int8_t RotationAccumulator;
	uint32_t Tick;
	uint32_t LastRotationActivity;
	volatile int16_t PendingRotationSteps;
	volatile uint8_t PendingPress;
} Encoder_TypeDef;

void BSP_Encoder_Init(Encoder_TypeDef *Encoder);
void BSP_Encoder_Process(Encoder_TypeDef *Encoder, Encoder_EventTypeDef *Events);
void BSP_Encoder_Tick(void);

#endif
