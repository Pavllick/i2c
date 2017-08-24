#include "i2c.h"

#include "stm32f0xx_rcc.h"
#include "stm32f0xx_tim.h"
#include "stm32f0xx_misc.h"
#include "stm32f0xx_gpio.h"
#include "sysclk.h"

#include "timer.h"
#include "dev_memory.h"

static TimerControl Timer;
static uint16_t prev_resist = 0;

struct PortConfig {
  GPIO_TypeDef *gpio;
  uint32_t pin;
  
  void initAsInput() const {
    GPIO_InitTypeDef init;
    init.GPIO_Pin   = pin;
    init.GPIO_Mode  = GPIO_Mode_IN;
    init.GPIO_Speed = GPIO_Speed_Level_3;
    init.GPIO_OType = GPIO_OType_PP;
    init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(gpio, &init);
  }
  
  void initAsOutput() const {
    GPIO_InitTypeDef init;
    init.GPIO_Pin   = pin;
    init.GPIO_Mode  = GPIO_Mode_OUT;
    init.GPIO_Speed = GPIO_Speed_Level_3;
    init.GPIO_OType = GPIO_OType_PP;
    init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(gpio, &init);
  }
  
  inline bool inputState() const {return (gpio->IDR & pin) != 0;}
  inline bool outputState() const {return (gpio->ODR & pin) != 0;}
  inline void setHigh() const {gpio->BSRR = pin;}
  inline void setLow() const {gpio->BRR = pin;}
};

static PortConfig const Port[] = {
  {GPIOB, GPIO_Pin_3 },
  {GPIOB, GPIO_Pin_4 }
};

struct I2CConnection {
  I2CConnection() {
    /* I2C ports start condition */
    Port[I2C::SDA].initAsOutput();
    Port[I2C::SDA].setHigh();
    
    Port[I2C::SCL].initAsOutput();
    Port[I2C::SCL].setLow();
  }
  
  void setData(uint8_t data) {
    uint8_t addrWrite = 0x5E;   // MCP4017 write address
    //uint8_t addrRead = 0x5F;  // MCP4017 read  address
    
    open();
    sendData(addrWrite);        // Set i2c device connection
    sendData(data);
    close();
  }
  
  void sendData(uint8_t data) {
    ///uint8_t bit = 0x80;       // b1000 0000
    uint8_t rr;
    for(int i = 0; i <= 7; i++) {
      uint8_t bit = 0x80;       // b1000 0000
      bit >>= i;
      if(data & bit) {
        Port[I2C::SDA].setHigh();
        
        Timer.ms_delay(1700);
        Port[I2C::SCL].setHigh();
        Timer.ms_delay(4000);
        Port[I2C::SCL].setLow();
        
        Timer.ms_delay(3000);
        Port[I2C::SDA].setLow();
        
        rr += 1;
      } else {
        Timer.ms_delay(1700);
        Port[I2C::SCL].setHigh();
        Timer.ms_delay(4000);
        Port[I2C::SCL].setLow();
        Timer.ms_delay(3000);
      }
      
      //bit >>= 1;
    }
    
    /* Response bit */
    Port[I2C::SDA].initAsInput();
    
    Timer.ms_delay(1700);
    Port[I2C::SCL].setHigh();
    if(!bitAvalidation(4000)) { // if A bit validation failed restart connection
      Port[I2C::SCL].setLow();
      Timer.ms_delay(1700);
      open();
      sendData(data);
    }
    Port[I2C::SCL].setLow();
    Timer.ms_delay(3000);
    
    Port[I2C::SDA].initAsOutput();
  }
  
  void open() {
    Port[I2C::SDA].initAsOutput();
    Port[I2C::SDA].setHigh();
    
    Port[I2C::SCL].setHigh();
    Timer.ms_delay(4700);
    
    Port[I2C::SDA].setLow();
    Timer.ms_delay(4000);
    
    Port[I2C::SCL].setLow();
    Timer.ms_delay(3000);
  }
  
  void close() {
    Timer.ms_delay(1700);
    Port[I2C::SCL].setHigh();
    Timer.ms_delay(4000);
    
    Port[I2C::SDA].setHigh();
    Timer.ms_delay(4000);
    
    Port[I2C::SCL].setLow();
  }
  
  bool bitAvalidation(int time) {
    int spend_time = 0;
    while(spend_time < time) {
      if(Port[I2C::SDA].inputState()) return false;
      Timer.ms_delay(time/100);
      spend_time += time/100;
    }
    
    return true;
  }
};

static I2CConnection I2CCon;
void I2C::init() {
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  Timer.init();
  I2CConnection I2CCon;
  
  I2CCon.setData(0);   // Init resistance = 0
}

void I2C::idle() {
  uint16_t resist = state.resistance;
  if(resist != prev_resist) {
    if(resist > 0x7F) {
      I2CCon.setData(0x7F);
    } else {
      I2CCon.setData((uint8_t)resist);
    }
    prev_resist = resist;
  } else return;
}