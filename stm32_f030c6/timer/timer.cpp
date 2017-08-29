#include "timer.h"

#include "stm32f0xx_rcc.h"
#include "stm32f0xx_tim.h"

static const uint16_t TIM_PERIOD = 0xFFFF;

void TimerControl::init() {
  TIM_DeInit(TIM3);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  
  TIM_TimeBaseStructure.TIM_Period = TIM_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler = 48-1 ;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0000;
  
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  TIM_SetCounter(TIM3, 0);
  
  TIM_Cmd(TIM3, ENABLE);
}

void TimerControl::ms_delay(unsigned short us) {
  TIM3->CNT = 0;
  while(TIM3->CNT < us);
}

uint16_t TimerControl::spendTime(uint16_t first, uint16_t last) {
  return (last >= first) ? (last - first) : (TIM_PERIOD - first + last);
}
