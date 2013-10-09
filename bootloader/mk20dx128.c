/* Fadecandy Bootloader
 * Copyright (c) 2013 Micah Elizabeth Scott
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


extern unsigned long _eflash;
extern unsigned long _sdtext;
extern unsigned long _edtext;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;

extern int main (void);
void ResetHandler(void);
void _init_Teensyduino_internal_(void);
void __libc_init_array(void);

void fault_isr(void)
{
    while (1); // die
}

void unused_isr(void)
{
    while (1); // die
}

void nmi_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void hard_fault_isr(void)   __attribute__ ((weak, alias("unused_isr")));
void memmanage_fault_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void bus_fault_isr(void)    __attribute__ ((weak, alias("unused_isr")));
void usage_fault_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void svcall_isr(void)       __attribute__ ((weak, alias("unused_isr")));
void debugmonitor_isr(void) __attribute__ ((weak, alias("unused_isr")));
void pendablesrvreq_isr(void)   __attribute__ ((weak, alias("unused_isr")));
void systick_isr(void)      __attribute__ ((weak, alias("unused_isr")));

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

// Relocated IVT in RAM
static uint32_t ramVectors[64] __attribute__ ((aligned (1024)));

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

__attribute__ ((section(".flashconfig"), used))
const uint8_t flashconfigbytes[16] =
{
    // Backdoor comparison key (disabled)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    /*
     * Program flash protection (FPROT)
     *
     * First 4K region is protected from erasure by any means other than mass-erase over JTAG.
     * This protects our bootloader from being modified except via hardware that could also replace it.
     *
     * OpenOCD doesn't give you an easy way to execute mass-erase currently. I'm lazy, I do this by patching
     * dap_syssec_kinetis_mdmap() in arm_adi_v5.c to always mass-erase, and I run that version of openocd
     * when necessary :(
     */
    0xfe, 0xff, 0xff, 0xff,

    0xfe,       // Data flash protection (FDPROT)
    0xff,       // EEPROM protection (FEPROT)
    0xff,       // Flash nonvolatile option byte (FOPT)
    0x40        // Flash security byte (FSEC)
};

__attribute__ ((section(".startup")))
void ResetHandler(void)
{
    /*
     * Enable watchdog timer. Allow settings to be changed later, in case the
     * application firmware wants to adjust its settings or disable it.
     *
     * Originally I tried using the 1 kHz low-power oscillator here, but that seemed to
     * run into an issue where refreshes weren't taking effect. It seems similar to
     * this problem on the Freescale forums, which didn't really have a satisfactory
     * solution:
     *
     *  https://community.freescale.com/thread/309519
     *
     * As a workaround, I'm using the "alternate" system clock.
     */
    {
        const uint32_t watchdog_timeout = F_BUS / 100;  // 10ms

        WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
        WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
        asm volatile ("nop");
        asm volatile ("nop");
        WDOG_STCTRLH = WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
            WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN | WDOG_STCTRLH_CLKSRC;
        WDOG_PRESC = 0;
        WDOG_TOVALH = watchdog_timeout >> 16;
        WDOG_TOVALL = watchdog_timeout;
    }

    // enable clocks to always-used peripherals
    SIM_SCGC5 = 0x00043F82;     // clocks active to all GPIO
    SIM_SCGC6 = SIM_SCGC6_RTC | SIM_SCGC6_FTM0 | SIM_SCGC6_FTM1 | SIM_SCGC6_ADC0 | SIM_SCGC6_FTFL;

    // release I/O pins hold, if we woke up from VLLS mode
    if (PMC_REGSC & PMC_REGSC_ACKISO) PMC_REGSC |= PMC_REGSC_ACKISO;

    // start in FEI mode
    // enable capacitors for crystal
    OSC0_CR = OSC_SC8P | OSC_SC2P;
    // enable osc, 8-32 MHz range, low power mode
    MCG_C2 = MCG_C2_RANGE0(2) | MCG_C2_EREFS;
    // switch to crystal as clock source, FLL input = 16 MHz / 512
    MCG_C1 =  MCG_C1_CLKS(2) | MCG_C1_FRDIV(4);
    // wait for crystal oscillator to begin
    while ((MCG_S & MCG_S_OSCINIT0) == 0) ;
    // wait for FLL to use oscillator
    while ((MCG_S & MCG_S_IREFST) != 0) ;
    // wait for MCGOUT to use oscillator
    while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(2)) ;
    // now we're in FBE mode
    // config PLL input for 16 MHz Crystal / 4 = 4 MHz
    MCG_C5 = MCG_C5_PRDIV0(3);
    // config PLL for 96 MHz output
    MCG_C6 = MCG_C6_PLLS | MCG_C6_VDIV0(0);

    // Copy things while we're waiting on the PLL
    {
        // Relocate data and text to RAM
        uint32_t *src = &_eflash;
        uint32_t *dest = &_sdtext;
        while (dest < &_edtext) *dest++ = *src++;

        // Clear BSS
        dest = &_sbss;
        while (dest < &_ebss) *dest++ = 0;

        // Copy IVT to RAM
        src = (uint32_t*) &gVectors[0];
        dest = &ramVectors[0];
        while (dest <= &ramVectors[63]) *dest++ = *src++;

        // Switch to ram IVT
        SCB_VTOR = (uint32_t) &ramVectors[0];
    }

    // wait for PLL to start using xtal as its input
    while (!(MCG_S & MCG_S_PLLST)) ;
    // wait for PLL to lock
    while (!(MCG_S & MCG_S_LOCK0)) ;
    // now we're in PBE mode

    // config divisors: 48 MHz core, 48 MHz bus, 24 MHz flash
    SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(1) | SIM_CLKDIV1_OUTDIV2(1) |  SIM_CLKDIV1_OUTDIV4(3);
    // switch to PLL as clock source, FLL input = 16 MHz / 512
    MCG_C1 = MCG_C1_CLKS(0) | MCG_C1_FRDIV(4);
    // wait for PLL clock to be used
    while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(3)) ;
    // now we're in PEE mode
    // configure USB for 48 MHz clock
    SIM_CLKDIV2 = SIM_CLKDIV2_USBDIV(1); // USB = 96 MHz PLL / 2
    // USB uses PLL clock, trace is CPU clock, CLKOUT=OSCERCLK0
    SIM_SOPT2 = SIM_SOPT2_USBSRC | SIM_SOPT2_PLLFLLSEL | SIM_SOPT2_TRACECLKSEL | SIM_SOPT2_CLKOUTSEL(6);

    __enable_irq();
    main();
}
