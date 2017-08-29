#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

struct TimerControl {
  void init();
  void ms_delay(unsigned short us);
  uint16_t spendTime(uint16_t first, uint16_t last);
};

#endif