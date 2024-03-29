#include "main.h"
#include <stdlib.h>
#include "driverlib/driverlib.h"
#include "hal_LCD.h"
#include <stdbool.h>

char thresholdData[6][50] = {0};
char responseData[100];
unsigned int command = 0;
unsigned int thresholdIndex = 0;
unsigned int numThreshold = 0;
bool thresholdsSet = false;

char ADCState = ADC_READY; //Busy state of the ADC
int16_t ADCResult = 0; //Storage for the ADC conversion result

zone_t zone0 = {-1, -1, true, true};
zone_t zone1 = {-1, -1, true, true};
int brightness = -1;
int tempThreshold = 25;
int moistThreshold = 10;

void main(void) {
    bool buttons[2] = {true, true}; //Current button press state (to allow edge detection)

    //Turn off interrupts during initialization
    __disable_interrupt();

    //Stop watchdog timer unless you plan on using it
    WDT_A_hold(WDT_A_BASE);

    // Initializations - see functions for more detail
    Init_GPIO();    //Sets all pins to output low as a default
    Init_ADC();     //Sets up the ADC to sample
    Init_Clock();   //Sets up the necessary system clocks
    Init_UART();    //Sets up an echo over a COM port
    Init_LCD();     //Sets up the LaunchPad LCD display

    PMM_unlockLPM5(); //Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

    //All done initializations - turn interrupts back on.
    __enable_interrupt();

    displayScrollText("ECE 298");

    int currentZone = 0;
    int currentlyReading = TEMP_MUX_1;
    while(1) {
        if ((GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 1) & (buttons[0] == false)) {
            buttons[0] = true;                //Capture new button state
        }
        if ((GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 1) & (buttons[1] == false)) {
            buttons[1] = true;                //Capture new button state
        }
        // Button released
        if ((GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 0) & (buttons[0] == true)) {
            currentZone = !currentZone;
            buttons[0] = false;
        }
        if ((GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 0) & (buttons[1] == true)) {
            currentZone = !currentZone;
            buttons[1] = false;
        }

        //Start an ADC conversion (if it's not busy) in Single-Channel, Single Conversion Mode
        if (ADCState == ADC_READY) {
            ADCState = ADC_BUSY; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        } else if (ADCState == ADC_DONE) {
            saveReading(currentlyReading, ADCResult);

            currentlyReading = currentlyReading == 7 ? BRIGHTNESS_MUX : currentlyReading + 1;
            setMUX(currentlyReading);

            updatePeripherals();

            ADCState = ADC_READY;
        }

        // Display temperature and moisture for current zone
        showZone(currentZone);

        if (currentZone == 1) {
            showTemperature(zone1.temperature);
            showMoisture(zone1.moisture);
        } else {
            showTemperature(zone0.temperature);
            showMoisture(zone0.moisture);
        }

        __delay_cycles(50000);
    }
}

void saveReading(int what, int value) {
    if (what == TEMP_MUX_1) {
        zone0.temperature = (value * 3.22 - 500) / 10;
    } else if (what == TEMP_MUX_2) {
        zone1.temperature = (value * 3.22 - 500) / 10;
    } else if (what == MOISTURE_MUX_1) {
        zone0.moisture = value / 10.3;
    } else if (what == MOISTURE_MUX_2) {
        zone1.moisture = value / 10.3;
    } else if (what == BRIGHTNESS_MUX) {
        brightness = (1023 - value) / 10.3;
    }
}

void updatePeripherals() {


    if (brightness > DAYLIGHT_THRESHOLD) {
        if (zone0.temperature > tempThreshold && zone0.fanEnabled) {
            setLED(3, true);
            setMotor(3, true);
        } else {
            setLED(3, false);
            setMotor(3, false);
        }

        if (zone1.temperature > tempThreshold && zone1.fanEnabled) {
            setLED(4, true);
            setMotor(4, true);
        } else {
            setLED(4, false);
            setMotor(4, false);
        }

        // turn off pumps
        setLED(1, false);
        setLED(2, false);
        setMotor(1, false);
        setMotor(2, false);
    } else {
        if (zone0.moisture < moistThreshold && zone0.pumpEnabled) {
            setLED(1, true);
            setMotor(1, true);
        } else {
            setLED(1, false);
            setMotor(1, false);
        }

        if (zone1.moisture < moistThreshold && zone1.pumpEnabled) {
            setLED(2, true);
            setMotor(2, true);
        } else {
            setLED(2, false);
            setMotor(2, false);
        }

        // turn off fans
        setLED(3, false);
        setMotor(3, false);
        setLED(4, false);
        setMotor(4, false);
    }
}

void setLED(int led, bool value) {
    switch(led) {
        case 1:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN7) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7);
            break;
        case 2:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
            break;
        case 3:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0) : GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
            break;
        case 4:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2) : GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);
            break;
        default:
            // error
            break;
    }
}


void setMotor(int motor, bool value) {
    switch(motor) {
        case 1:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3) : GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
            break;
        case 2:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
            break;
        case 3:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
            break;
        case 4:
            value ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
            break;
        default:
            // error
            break;
    }
}

void setMUX(unsigned short control_signal) {
    bool s0 = control_signal & 0b1U;
    bool s1 = (control_signal >> 1U) & 0b1U;
    bool s2 = (control_signal >> 2U) & 0b1U;

    s0 ? GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5) : GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
    s1 ? GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2) : GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
    s2 ? GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN3) : GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);
}

void Init_GPIO(void)
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    //Set LaunchPad switches as inputs - they are active low, meaning '1' until pressed
    GPIO_setAsInputPinWithPullUpResistor(SW1_PORT, SW1_PIN);
    GPIO_setAsInputPinWithPullUpResistor(SW2_PORT, SW2_PIN);

    //Set LED1 and LED2 as outputs
    //GPIO_setAsOutputPin(LED1_PORT, LED1_PIN); //Comment if using UART
    GPIO_setAsOutputPin(LED2_PORT, LED2_PIN);
}

/* Clock System Initialization */
void Init_Clock(void)
{
    /*
     * The MSP430 has a number of different on-chip clocks. You can read about it in
     * the section of the Family User Guide regarding the Clock System ('cs.h' in the
     * driverlib).
     */

    /*
     * On the LaunchPad, there is a 32.768 kHz crystal oscillator used as a
     * Real Time Clock (RTC). It is a quartz crystal connected to a circuit that
     * resonates it. Since the frequency is a power of two, you can use the signal
     * to drive a counter, and you know that the bits represent binary fractions
     * of one second. You can then have the RTC module throw an interrupt based
     * on a 'real time'. E.g., you could have your system sleep until every
     * 100 ms when it wakes up and checks the status of a sensor. Or, you could
     * sample the ADC once per second.
     */
    //Set P4.1 and P4.2 as Primary Module Function Input, XT_LF
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768);
    // Set ACLK = XT1
    CS_initClockSignal(CS_ACLK, CS_XT1CLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Initializes the XT1 crystal oscillator
    CS_turnOnXT1LF(CS_XT1_DRIVE_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
}

/* UART Initialization */
void Init_UART(void)
{
    /* UART: It configures P1.0 and P1.1 to be connected internally to the
     * eSCSI module, which is a serial communications module, and places it
     * in UART mode. This let's you communicate with the PC via a software
     * COM port over the USB cable. You can use a console program, like PuTTY,
     * to type to your LaunchPad. The code in this sample just echos back
     * whatever character was received.
     */

    //Configure UART pins, which maps them to a COM port over the USB cable
    //Set P1.0 and P1.1 as Secondary Module Function Input.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

    /*
     * UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 9600 baud rate. These
     * values were calculated using the online calculator that TI provides at:
     * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
     */

    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 17, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
        param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
        param.clockPrescalar    = 6;
        param.firstModReg       = 8;
        param.secondModReg      = 17;
        param.parity            = EUSCI_A_UART_NO_PARITY;
        param.msborLsbFirst     = EUSCI_A_UART_LSB_FIRST;
        param.numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT;
        param.uartMode          = EUSCI_A_UART_MODE;
        param.overSampling      = 1;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    // Enable EUSCI_A0 RX interrupt
    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
}

/* EUSCI A0 UART ISR - Echoes data back to PC host */
#pragma vector=USCI_A0_VECTOR
__interrupt
void EUSCIA0_ISR(void)
{
    uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);

    if (RxStatus)
    {
        EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus);
        unsigned char res = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);

        if (res == '\r')
        {
            int value = atoi(&thresholdData[command][1]);
            if (command == 0) {
                tempThreshold = value;
            } else if (command == 1) {
                moistThreshold = value;
            } else if (command == 2) {
                zone0.fanEnabled = value == 1;
            } else if (command == 3) {
                zone0.pumpEnabled = value == 1;
            } else if (command == 4) {
                zone1.fanEnabled = value == 1;
            } else if (command == 5) {
                zone1.pumpEnabled = value == 1;
            }

            thresholdIndex = 0;
            numThreshold++;

            if (numThreshold == 6) {
                thresholdsSet = true;
                numThreshold = 0;
            }
        }
        else {
            if (res == 't') {
                command = 0;
            } else if (res == 'm') {
                command = 1;
            } else if (res == 'a') {
                command = 2;
            } else if (res == 'b') {
                command = 3;
            } else if (res == 'c') {
                command = 4;
            } else if (res == 'd') {
                command = 5;
            }

            thresholdData[command][thresholdIndex] = res;
            thresholdIndex++;
        }
    }
}

/* PWM Initialization */
void Init_PWM(void)
{
    /*
     * The internal timers (TIMER_A) can auto-generate a PWM signal without needing to
     * flip an output bit every cycle in software. The catch is that it limits which
     * pins you can use to output the signal, whereas manually flipping an output bit
     * means it can be on any GPIO. This function populates a data structure that tells
     * the API to use the timer as a hardware-generated PWM source.
     *
     */
    //Generate PWM - Timer runs in Up-Down mode
    param.clockSource           = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider    = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod           = TIMER_A_PERIOD; //Defined in main.h
    param.compareRegister       = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    param.compareOutputMode     = TIMER_A_OUTPUTMODE_RESET_SET;
    param.dutyCycle             = HIGH_COUNT; //Defined in main.h

    //PWM_PORT PWM_PIN (defined in main.h) as PWM output
    GPIO_setAsPeripheralModuleFunctionOutputPin(PWM_PORT, PWM_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
}

void Init_ADC(void)
{
    /*
     * To use the ADC, you need to tell a physical pin to be an analog input instead
     * of a GPIO, then you need to tell the ADC to use that analog input. Defined
     * these in main.h for A9 on P8.1.
     */

    //Set ADC_IN to input direction
    GPIO_setAsPeripheralModuleFunctionInputPin(ADC_IN_PORT, ADC_IN_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    //Initialize the ADC Module
    /*
     * Base Address for the ADC Module
     * Use internal ADC bit as sample/hold signal to start conversion
     * USE MODOSC 5MHZ Digital Oscillator as clock source
     * Use default clock divider of 1
     */
    ADC_init(ADC_BASE,
             ADC_SAMPLEHOLDSOURCE_SC,
             ADC_CLOCKSOURCE_ADCOSC,
             ADC_CLOCKDIVIDER_1);

    ADC_enable(ADC_BASE);

    /*
     * Base Address for the ADC Module
     * Sample/hold for 16 clock cycles
     * Do not enable Multiple Sampling
     */
    ADC_setupSamplingTimer(ADC_BASE,
                           ADC_CYCLEHOLD_16_CYCLES,
                           ADC_MULTIPLESAMPLESDISABLE);

    //Configure Memory Buffer
    /*
     * Base Address for the ADC Module
     * Use input ADC_IN_CHANNEL
     * Use positive reference of AVcc
     * Use negative reference of AVss
     */
    ADC_configureMemory(ADC_BASE,
                        ADC_IN_CHANNEL,
                        ADC_VREFPOS_AVCC,
                        ADC_VREFNEG_AVSS);

    ADC_clearInterrupt(ADC_BASE,
                       ADC_COMPLETED_INTERRUPT);

    //Enable Memory Buffer interrupt
    ADC_enableInterrupt(ADC_BASE,
                        ADC_COMPLETED_INTERRUPT);
}

//ADC interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt
void ADC_ISR(void)
{
    uint8_t ADCStatus = ADC_getInterruptStatus(ADC_BASE, ADC_COMPLETED_INTERRUPT_FLAG);

    ADC_clearInterrupt(ADC_BASE, ADCStatus);

    if (ADCStatus)
    {
        ADCState = ADC_DONE; //Not busy anymore
        ADCResult = ADC_getResults(ADC_BASE);
    }
}
