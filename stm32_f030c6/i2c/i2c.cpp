#include "i2c.h"

#include "stm32f0xx_rcc.h"
#include "stm32f0xx_tim.h"
#include "stm32f0xx_misc.h"
#include "stm32f0xx_gpio.h"
#include "sysclk.h"

#include "timer.h"
#include "dev_memory.h"

static TimerControl Timer;
static uint16_t prev_resist[RESISTOR_QUANTITY];

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

static PortConfig const Port1[] = {
  {GPIOB, GPIO_Pin_3 },
  {GPIOB, GPIO_Pin_4 }
};

static PortConfig const Port2[] = {
  {GPIOB, GPIO_Pin_5 },
  {GPIOB, GPIO_Pin_6 }
};

static PortConfig const *Port[] {
  Port1, Port2
};

struct I2CConnection {
  uint8_t port;
  
  I2CConnection(uint8_t p) {
    port = p;
    /* I2C ports start condition */
    Port[port][I2C::SDA].initAsOutput();
    Port[port][I2C::SDA].setHigh();
    
    Port[port][I2C::SCL].initAsOutput();
    Port[port][I2C::SCL].setLow();
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
        Port[port][I2C::SDA].setHigh();
        
        Timer.ms_delay(1700);
        Port[port][I2C::SCL].setHigh();
        Timer.ms_delay(4000);
        Port[port][I2C::SCL].setLow();
        
        Timer.ms_delay(3000);
        Port[port][I2C::SDA].setLow();
        
        rr += 1;
      } else {
        Timer.ms_delay(1700);
        Port[port][I2C::SCL].setHigh();
        Timer.ms_delay(4000);
        Port[port][I2C::SCL].setLow();
        Timer.ms_delay(3000);
      }
      
      //bit >>= 1;
    }
    
    /* Response bit */
    Port[port][I2C::SDA].initAsInput();
    
    Timer.ms_delay(1700);
    Port[port][I2C::SCL].setHigh();
    if(!bitAvalidation(4000)) { // if A bit validation failed restart connection
      Port[port][I2C::SCL].setLow();
      Timer.ms_delay(1700);
      open();
      sendData(data);
    }
    Port[port][I2C::SCL].setLow();
    Timer.ms_delay(3000);
    
    Port[port][I2C::SDA].initAsOutput();
  }
  
  void open() {
    Port[port][I2C::SDA].initAsOutput();
    Port[port][I2C::SDA].setHigh();
    
    Port[port][I2C::SCL].setHigh();
    Timer.ms_delay(4700);
    
    Port[port][I2C::SDA].setLow();
    Timer.ms_delay(4000);
    
    Port[port][I2C::SCL].setLow();
    Timer.ms_delay(3000);
  }
  
  void close() {
    Timer.ms_delay(1700);
    Port[port][I2C::SCL].setHigh();
    Timer.ms_delay(4000);
    
    Port[port][I2C::SDA].setHigh();
    Timer.ms_delay(4000);
    
    Port[port][I2C::SCL].setLow();
  }
  
  bool bitAvalidation(int time) {
    int spend_time = 0;
    while(spend_time < time) {
      if(Port[port][I2C::SDA].inputState()) return false;
      Timer.ms_delay(time/100);
      spend_time += time/100;
    }
    
    return true;
  }
};

  static I2CConnection I2CCon[RESISTOR_QUANTITY] = {0, 1}; // Number of resistors
void I2C::init() {
  static I2CConnection I2CCon[RESISTOR_QUANTITY] = {0, 1}; // Number of resistors
  
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  Timer.init();
  for(int i = 0; i < RESISTOR_QUANTITY; i++) {
    I2CCon[i].setData(0);   // Init resistance = 0
  }
}

void I2C::idle() {
  for(int i = 0; i < RESISTOR_QUANTITY; i++) {
    uint16_t resist = state.resistance[i];
    
    if(state.resistance[i] != prev_resist[i]) {
      if(resist > 0x7F) {
        I2CCon[i].setData(0x7F);
      } else {
        I2CCon[i].setData((uint8_t)resist);
      }
      prev_resist[i] = resist;
    }
  }
}