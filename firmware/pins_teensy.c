/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "core_pins.h"
#include "pins_arduino.h"

#define GPIO_BITBAND_ADDR(reg, bit) (((uint32_t)&(reg) - 0x40000000) * 32 + (bit) * 4 + 0x42000000)
#define GPIO_BITBAND_PTR(reg, bit) ((uint32_t *)GPIO_BITBAND_ADDR((reg), (bit)))
//#define GPIO_SET_BIT(reg, bit) (*GPIO_BITBAND_PTR((reg), (bit)) = 1)
//#define GPIO_CLR_BIT(reg, bit) (*GPIO_BITBAND_PTR((reg), (bit)) = 0)

const struct digital_pin_bitband_and_config_table_struct digital_pin_to_info_PGM[] = {
    {GPIO_BITBAND_PTR(CORE_PIN0_PORTREG, CORE_PIN0_BIT), &CORE_PIN0_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN1_PORTREG, CORE_PIN1_BIT), &CORE_PIN1_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN2_PORTREG, CORE_PIN2_BIT), &CORE_PIN2_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN3_PORTREG, CORE_PIN3_BIT), &CORE_PIN3_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN4_PORTREG, CORE_PIN4_BIT), &CORE_PIN4_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN5_PORTREG, CORE_PIN5_BIT), &CORE_PIN5_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN6_PORTREG, CORE_PIN6_BIT), &CORE_PIN6_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN7_PORTREG, CORE_PIN7_BIT), &CORE_PIN7_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN8_PORTREG, CORE_PIN8_BIT), &CORE_PIN8_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN9_PORTREG, CORE_PIN9_BIT), &CORE_PIN9_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN10_PORTREG, CORE_PIN10_BIT), &CORE_PIN10_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN11_PORTREG, CORE_PIN11_BIT), &CORE_PIN11_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN12_PORTREG, CORE_PIN12_BIT), &CORE_PIN12_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN13_PORTREG, CORE_PIN13_BIT), &CORE_PIN13_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN14_PORTREG, CORE_PIN14_BIT), &CORE_PIN14_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN15_PORTREG, CORE_PIN15_BIT), &CORE_PIN15_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN16_PORTREG, CORE_PIN16_BIT), &CORE_PIN16_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN17_PORTREG, CORE_PIN17_BIT), &CORE_PIN17_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN18_PORTREG, CORE_PIN18_BIT), &CORE_PIN18_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN19_PORTREG, CORE_PIN19_BIT), &CORE_PIN19_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN20_PORTREG, CORE_PIN20_BIT), &CORE_PIN20_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN21_PORTREG, CORE_PIN21_BIT), &CORE_PIN21_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN22_PORTREG, CORE_PIN22_BIT), &CORE_PIN22_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN23_PORTREG, CORE_PIN23_BIT), &CORE_PIN23_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN24_PORTREG, CORE_PIN24_BIT), &CORE_PIN24_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN25_PORTREG, CORE_PIN25_BIT), &CORE_PIN25_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN26_PORTREG, CORE_PIN26_BIT), &CORE_PIN26_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN27_PORTREG, CORE_PIN27_BIT), &CORE_PIN27_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN28_PORTREG, CORE_PIN28_BIT), &CORE_PIN28_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN29_PORTREG, CORE_PIN29_BIT), &CORE_PIN29_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN30_PORTREG, CORE_PIN30_BIT), &CORE_PIN30_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN31_PORTREG, CORE_PIN31_BIT), &CORE_PIN31_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN32_PORTREG, CORE_PIN32_BIT), &CORE_PIN32_CONFIG},
    {GPIO_BITBAND_PTR(CORE_PIN33_PORTREG, CORE_PIN33_BIT), &CORE_PIN33_CONFIG}
};

unsigned long rtc_get(void)
{
    return RTC_TSR;
}

void rtc_set(unsigned long t)
{
    RTC_SR = 0;
    RTC_TPR = 0;
    RTC_TSR = t;
    RTC_SR = RTC_SR_TCE;
}


// adjust is the amount of crystal error to compensate, 1 = 0.1192 ppm
// For example, adjust = -100 is slows the clock by 11.92 ppm
//
void rtc_compensate(int adjust)
{
    uint32_t comp, interval, tcr;

    // This simple approach tries to maximize the interval.
    // Perhaps minimizing TCR would be better, so the
    // compensation is distributed more evenly across
    // many seconds, rather than saving it all up and then
    // altering one second up to +/- 0.38%
    if (adjust >= 0) {
        comp = adjust;
        interval = 256;
        while (1) {
            tcr = comp * interval;
            if (tcr < 128*256) break;
            if (--interval == 1) break;
        }
        tcr = tcr >> 8;
    } else {
        comp = -adjust;
        interval = 256;
        while (1) {
            tcr = comp * interval;
            if (tcr < 129*256) break;
            if (--interval == 1) break;
        }
        tcr = tcr >> 8;
        tcr = 256 - tcr;
    }
    RTC_TCR = ((interval - 1) << 8) | tcr;
}

extern void usb_init(void);


// create a default PWM at the same 488.28 Hz as Arduino Uno
#if F_BUS == 48000000
#define DEFAULT_FTM_MOD (49152 - 1)
#define DEFAULT_FTM_PRESCALE 1
#else
#define DEFAULT_FTM_MOD (49152 - 1)
#define DEFAULT_FTM_PRESCALE 0
#endif

//void init_pins(void)
void _init_Teensyduino_internal_(void)
{
    //SIM_SCGC6 |= SIM_SCGC6_FTM0;  // TODO: use bitband for atomic read-mod-write
    //SIM_SCGC6 |= SIM_SCGC6_FTM1;
    FTM0_CNT = 0;
    FTM0_MOD = DEFAULT_FTM_MOD;
    FTM0_C0SC = 0x28; // MSnB:MSnA = 10, ELSnB:ELSnA = 10
    FTM0_C1SC = 0x28;
    FTM0_C2SC = 0x28;
    FTM0_C3SC = 0x28;
    FTM0_C4SC = 0x28;
    FTM0_C5SC = 0x28;
    FTM0_C6SC = 0x28;
    FTM0_C7SC = 0x28;
    FTM0_SC = FTM_SC_CLKS(1) | FTM_SC_PS(DEFAULT_FTM_PRESCALE);
    FTM1_CNT = 0;
    FTM1_MOD = DEFAULT_FTM_MOD;
    FTM1_C0SC = 0x28;
    FTM1_C1SC = 0x28;
    FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(DEFAULT_FTM_PRESCALE);

    usb_init();
}



// SOPT4 is SIM select clocks?
// FTM is clocked by the bus clock, either 24 or 48 MHz
// input capture can be FTM1_CH0, CMP0 or CMP1 or USB start of frame
// 24 MHz with reload 49152 to match Arduino's speed = 488.28125 Hz

static const uint8_t analog_write_res = 8;

void analogWrite(uint8_t pin, int val)
{
    uint32_t cval, max;

    max = 1 << analog_write_res;
    if (val <= 0) {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);   // TODO: implement OUTPUT_LOW
        return;
    } else if (val >= max) {
        digitalWrite(pin, HIGH);
        pinMode(pin, OUTPUT);   // TODO: implement OUTPUT_HIGH
        return;
    }

    //serial_print("analogWrite\n");
    //serial_print("val = ");
    //serial_phex32(val);
    //serial_print("\n");
    //serial_print("analog_write_res = ");
    //serial_phex(analog_write_res);
    //serial_print("\n");
    if (pin == 3 || pin == 4) {
        cval = ((uint32_t)val * (uint32_t)(FTM1_MOD + 1)) >> analog_write_res;
        //serial_print("FTM1_MOD = ");
        //serial_phex32(FTM1_MOD);
        //serial_print("\n");
    } else {
        cval = ((uint32_t)val * (uint32_t)(FTM0_MOD + 1)) >> analog_write_res;
        //serial_print("FTM0_MOD = ");
        //serial_phex32(FTM0_MOD);
        //serial_print("\n");
    }
    //serial_print("cval = ");
    //serial_phex32(cval);
    //serial_print("\n");
    switch (pin) {
      case 3: // PTA12, FTM1_CH0
        FTM1_C0V = cval;
        CORE_PIN3_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 4: // PTA13, FTM1_CH1
        FTM1_C1V = cval;
        CORE_PIN4_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 5: // PTD7, FTM0_CH7
        FTM0_C7V = cval;
        CORE_PIN5_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 6: // PTD4, FTM0_CH4
        FTM0_C4V = cval;
        CORE_PIN6_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 9: // PTC3, FTM0_CH2
        FTM0_C2V = cval;
        CORE_PIN9_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 10: // PTC4, FTM0_CH3
        FTM0_C3V = cval;
        CORE_PIN10_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 20: // PTD5, FTM0_CH5
        FTM0_C5V = cval;
        CORE_PIN20_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 21: // PTD6, FTM0_CH6
        FTM0_C6V = cval;
        CORE_PIN21_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 22: // PTC1, FTM0_CH0
        FTM0_C0V = cval;
        CORE_PIN22_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      case 23: // PTC2, FTM0_CH1
        FTM0_C1V = cval;
        CORE_PIN23_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;
        break;
      default:
        digitalWrite(pin, (val > 127) ? HIGH : LOW);
        pinMode(pin, OUTPUT);
    }
}

void analogWriteFrequency(uint8_t pin, uint32_t frequency)
{
    uint32_t minfreq, prescale, mod;

    //serial_print("analogWriteFrequency: pin = ");
    //serial_phex(pin);
    //serial_print(", freq = ");
    //serial_phex32(frequency);
    //serial_print("\n");
    for (prescale = 0; prescale < 7; prescale++) {
        minfreq = (F_BUS >> 16) >> prescale;
        if (frequency > minfreq) break;
    }
    //serial_print("F_BUS = ");
    //serial_phex32(F_BUS >> prescale);
    //serial_print("\n");
    //serial_print("prescale = ");
    //serial_phex(prescale);
    //serial_print("\n");
    //mod = ((F_BUS >> prescale) / frequency) - 1;
    mod = (((F_BUS >> prescale) + (frequency >> 1)) / frequency) - 1;
    if (mod > 65535) mod = 65535;
    //serial_print("mod = ");
    //serial_phex32(mod);
    //serial_print("\n");
    if (pin == 3 || pin == 4) {
        FTM1_SC = 0;
        FTM1_CNT = 0;
        FTM1_MOD = mod;
        FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(prescale);
    } else if (pin == 5 || pin == 6 || pin == 9 || pin == 10 ||
      (pin >= 20 && pin <= 23)) {
        FTM0_SC = 0;
        FTM0_CNT = 0;
        FTM0_MOD = mod;
        FTM0_SC = FTM_SC_CLKS(1) | FTM_SC_PS(prescale);
    }
}
void digitalWrite(uint8_t pin, uint8_t val)
{
    if (pin >= CORE_NUM_DIGITAL) return;
    if (*portModeRegister(pin)) {
        if (val) {
            *portSetRegister(pin) = 1;
        } else {
            *portClearRegister(pin) = 1;
        }
    } else {
        volatile uint32_t *config = portConfigRegister(pin);
        if (val) {
            // TODO use bitband for atomic read-mod-write
            *config |= (PORT_PCR_PE | PORT_PCR_PS);
            //*config = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;
        } else {
            // TODO use bitband for atomic read-mod-write
            *config &= ~(PORT_PCR_PE);
            //*config = PORT_PCR_MUX(1);
        }
    }

}

uint8_t digitalRead(uint8_t pin)
{
    if (pin >= CORE_NUM_DIGITAL) return 0;
    return *portInputRegister(pin);
}



void pinMode(uint8_t pin, uint8_t mode)
{
    volatile uint32_t *config;

    if (pin >= CORE_NUM_DIGITAL) return;
    config = portConfigRegister(pin);

    if (mode == OUTPUT) {
        *portModeRegister(pin) = 1;
        *config = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
    } else {
        *portModeRegister(pin) = 0;
        if (mode == INPUT) {
            *config = PORT_PCR_MUX(1);
        } else {
            *config = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS; // pullup
        }
    }
}

// the systick interrupt is supposed to increment this at 1 kHz rate
volatile uint32_t systick_millis_count = 0;

//uint32_t systick_current, systick_count, systick_istatus;  // testing only

uint32_t micros(void)
{
    uint32_t count, current, istatus;

    __disable_irq();
    current = SYST_CVR;
    count = systick_millis_count;
    istatus = SCB_ICSR; // bit 26 indicates if systick exception pending
    __enable_irq();
     //systick_current = current;
     //systick_count = count;
     //systick_istatus = istatus & SCB_ICSR_PENDSTSET ? 1 : 0;
    if ((istatus & SCB_ICSR_PENDSTSET) && current > 50) count++;
    current = ((F_CPU / 1000) - 1) - current;
    return count * 1000 + current / (F_CPU / 1000000);
}

void delay(uint32_t ms)
{
    uint32_t start = micros();

    if (ms > 0) {
        while (1) {
            if ((micros() - start) >= 1000) {
                ms--;
                if (ms == 0) return;
                start += 1000;
            }
        }
    }
}



























