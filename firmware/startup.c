/*
 * Initialization code for an FC-Boot application.
 * Copyright (c) 2013 Micah Elizabeth Scott
 *
 * Originally based on:
 *
 * Teensyduino Core Library
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

#include "mk20dx128.h"

typedef void (*initFunc_t)(void);

extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;
extern unsigned long _flexram_begin;
extern unsigned long _flexram_end;
extern initFunc_t __init_array_start;
extern initFunc_t __init_array_end;

extern int main (void);
void ResetHandler(void);
void _init_Teensyduino_internal_(void);


void fault_isr(void)
{
        while (1); // die
}

void unused_isr(void)
{
        while (1); // die
}

extern volatile uint32_t systick_millis_count;
void systick_default_isr(void)
{
    systick_millis_count++;
}

void nmi_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void hard_fault_isr(void)   __attribute__ ((weak, alias("unused_isr")));
void memmanage_fault_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void bus_fault_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void usage_fault_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void svcall_isr(void)       __attribute__ ((weak, alias("unused_isr")));
void debugmonitor_isr(void) __attribute__ ((weak, alias("unused_isr")));
void pendablesrvreq_isr(void)   __attribute__ ((weak, alias("unused_isr")));
void systick_isr(void)      __attribute__ ((weak, alias("systick_default_isr")));

void dma_ch0_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void dma_ch1_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void dma_ch2_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void dma_ch3_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void dma_error_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void flash_cmd_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void flash_error_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void low_voltage_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void wakeup_isr(void)       __attribute__ ((weak, alias("unused_isr")));
void watchdog_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void i2c0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void spi0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void i2s0_tx_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void i2s0_rx_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void uart0_lon_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void uart0_status_isr(void) __attribute__ ((weak, alias("unused_isr")));
void uart0_error_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void uart1_status_isr(void) __attribute__ ((weak, alias("unused_isr")));
void uart1_error_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void uart2_status_isr(void) __attribute__ ((weak, alias("unused_isr")));
void uart2_error_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void adc0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void cmp0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void cmp1_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void ftm0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void ftm1_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void cmt_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void rtc_alarm_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void rtc_seconds_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void pit0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void pit1_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void pit2_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void pit3_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void pdb_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void usb_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void usb_charge_isr(void)   __attribute__ ((weak, alias("unused_isr")));
void tsi0_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void mcg_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void lptmr_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void porta_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void portb_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void portc_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void portd_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void porte_isr(void)        __attribute__ ((weak, alias("unused_isr")));
void software_isr(void)     __attribute__ ((weak, alias("unused_isr")));


// TODO: create AVR-stype ISR() macro, with default linkage to undefined handler
//
__attribute__ ((section(".vectors"), used))
void (* const gVectors[])(void) =
{
        (void (*)(void))((unsigned long)&_estack),  //  0 ARM: Initial Stack Pointer
        ResetHandler,                   //  1 ARM: Initial Program Counter
    nmi_isr,                    //  2 ARM: Non-maskable Interrupt (NMI)
    hard_fault_isr,                 //  3 ARM: Hard Fault
    memmanage_fault_isr,                //  4 ARM: MemManage Fault
    bus_fault_isr,                  //  5 ARM: Bus Fault
    usage_fault_isr,                //  6 ARM: Usage Fault
    fault_isr,                  //  7 --
    fault_isr,                  //  8 --
    fault_isr,                  //  9 --
    fault_isr,                  // 10 --
    svcall_isr,                 // 11 ARM: Supervisor call (SVCall)
    debugmonitor_isr,               // 12 ARM: Debug Monitor
    fault_isr,                  // 13 --
    pendablesrvreq_isr,             // 14 ARM: Pendable req serv(PendableSrvReq)
    systick_isr,                    // 15 ARM: System tick timer (SysTick)
    dma_ch0_isr,                    // 16 DMA channel 0 transfer complete
    dma_ch1_isr,                    // 17 DMA channel 1 transfer complete
    dma_ch2_isr,                    // 18 DMA channel 2 transfer complete
    dma_ch3_isr,                    // 19 DMA channel 3 transfer complete
    dma_error_isr,                  // 20 DMA error interrupt channel
    unused_isr,                 // 21 DMA --
    flash_cmd_isr,                  // 22 Flash Memory Command complete
    flash_error_isr,                // 23 Flash Read collision
    low_voltage_isr,                // 24 Low-voltage detect/warning
    wakeup_isr,                 // 25 Low Leakage Wakeup
    watchdog_isr,                   // 26 Both EWM and WDOG interrupt
    i2c0_isr,                   // 27 I2C0
    spi0_isr,                   // 28 SPI0
    i2s0_tx_isr,                    // 29 I2S0 Transmit
    i2s0_rx_isr,                    // 30 I2S0 Receive
    uart0_lon_isr,                  // 31 UART0 CEA709.1-B (LON) status
    uart0_status_isr,               // 32 UART0 status
    uart0_error_isr,                // 33 UART0 error
    uart1_status_isr,               // 34 UART1 status
    uart1_error_isr,                // 35 UART1 error
    uart2_status_isr,               // 36 UART2 status
    uart2_error_isr,                // 37 UART2 error
    adc0_isr,                   // 38 ADC0
    cmp0_isr,                   // 39 CMP0
    cmp1_isr,                   // 40 CMP1
    ftm0_isr,                   // 41 FTM0
    ftm1_isr,                   // 42 FTM1
    cmt_isr,                    // 43 CMT
    rtc_alarm_isr,                  // 44 RTC Alarm interrupt
    rtc_seconds_isr,                // 45 RTC Seconds interrupt
    pit0_isr,                   // 46 PIT Channel 0
    pit1_isr,                   // 47 PIT Channel 1
    pit2_isr,                   // 48 PIT Channel 2
    pit3_isr,                   // 49 PIT Channel 3
    pdb_isr,                    // 50 PDB Programmable Delay Block
    usb_isr,                    // 51 USB OTG
    usb_charge_isr,                 // 52 USB Charger Detect
    tsi0_isr,                   // 53 TSI0
    mcg_isr,                    // 54 MCG
    lptmr_isr,                  // 55 Low Power Timer
    porta_isr,                  // 56 Pin detect (Port A)
    portb_isr,                  // 57 Pin detect (Port B)
    portc_isr,                  // 58 Pin detect (Port C)
    portd_isr,                  // 59 Pin detect (Port D)
    porte_isr,                  // 60 Pin detect (Port E)
    software_isr,                   // 61 Software interrupt
};

static unsigned ftfl_busy()
{
    // Is the flash memory controller busy?
    return 0 == (FTFL_FSTAT_CCIF & FTFL_FSTAT);
}

static void ftfl_busy_wait()
{
    // Wait for the flash memory controller to finish any pending operation.
    while (ftfl_busy());
}

static void ftfl_launch_command()
{
    // Begin a flash memory controller command
    FTFL_FSTAT = FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_RDCOLERR;
    FTFL_FSTAT = FTFL_FSTAT_CCIF;
}

static void ftfl_set_flexram_function(uint8_t control_code)
{
    // Issue a Set FlexRAM Function command. Busy-waits until the command is done.
    
    ftfl_busy_wait();
    FTFL_FCCOB0 = 0x81;
    FTFL_FCCOB1 = control_code;
    ftfl_launch_command();
    ftfl_busy_wait();
}

void ResetHandler(void)
{
    // Init data RAM
    uint32_t *src = &_etext;
    uint32_t *dest = &_sdata;
    while (dest < &_edata) *dest++ = *src++;
    dest = &_sbss;
    while (dest < &_ebss) *dest++ = 0;

    // initialize the SysTick counter
    SYST_RVR = (F_CPU / 1000) - 1;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;

    // Use FlexRAM as normal RAM, and zero it
    ftfl_set_flexram_function(0xFF);
    dest = &_flexram_begin;
    while (dest < &_flexram_end) *dest++ = 0;

    __enable_irq();
    _init_Teensyduino_internal_();

    initFunc_t *p = &__init_array_start;
    for (; p != &__init_array_end; p++)
        p[0]();

    main();
    while (1) ;
}

