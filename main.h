#ifndef MAIN_H_
#define MAIN_H_

#include "driverlib/driverlib.h"

#define TIMER_A_PERIOD  1000 //T = 1/f = (TIMER_A_PERIOD * 1 us)
#define HIGH_COUNT      500  //Number of cycles signal is high (Duty Cycle = HIGH_COUNT / TIMER_A_PERIOD)

//Output pin to buzzer
#define PWM_PORT        GPIO_PORT_P1
#define PWM_PIN         GPIO_PIN7
//LaunchPad LED1 - note unavailable if UART is used
#define LED1_PORT       GPIO_PORT_P1
#define LED1_PIN        GPIO_PIN0
//LaunchPad LED2
#define LED2_PORT       GPIO_PORT_P4
#define LED2_PIN        GPIO_PIN0
//LaunchPad Pushbutton Switch 1
#define SW1_PORT        GPIO_PORT_P1
#define SW1_PIN         GPIO_PIN2
//LaunchPad Pushbutton Switch 2
#define SW2_PORT        GPIO_PORT_P2
#define SW2_PIN         GPIO_PIN6
//Input to ADC - in this case input A9 maps to pin P8.1
#define ADC_IN_PORT     GPIO_PORT_P8
#define ADC_IN_PIN      GPIO_PIN0
#define ADC_IN_CHANNEL  ADC_INPUT_A8

#define ADC_READY 0
#define ADC_BUSY 1
#define ADC_DONE 2

#define TEMP_MUX_1 6
#define TEMP_MUX_2 4
#define MOISTURE_MUX_1 5
#define MOISTURE_MUX_2 7
#define BRIGHTNESS_MUX 3

#define DAYLIGHT_THRESHOLD 15

void Init_GPIO(void);
void Init_Clock(void);
void Init_UART(void);
void Init_PWM(void);
void Init_ADC(void);

void setLED(int led, bool value);
void setMotor(int motor, bool value);
void setMUX(unsigned short control_signal);
void updatePeripherals();
void saveReading(int what, int value);

typedef struct zone {
    int temperature;
    int moisture;
    bool fanEnabled;
    bool pumpEnabled;
} zone_t;

Timer_A_outputPWMParam param; //Timer configuration data structure for PWM

#endif /* MAIN_H_ */
