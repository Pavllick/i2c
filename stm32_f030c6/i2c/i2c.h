#ifndef I2C_H
#define I2C_H

#include <stdint.h>

namespace I2C {
  
  static uint8_t SDA = 0;
  static uint8_t SCL = 1;
  
  void init();
  void idle();
  void setResistance(int val);
  int getResistance();
}

#endif