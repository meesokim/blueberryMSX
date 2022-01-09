/*

    Part of the Raspberry-Pi Bare Metal Tutorials
    Copyright (c) 2013, Brian Sidebotham
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef RPI_GPIO_H
#define RPI_GPIO_H

#ifndef RASPPI
#define RASPPI 3
#endif

/* The base address of the GPIO peripheral (ARM Physical Address) */
#if RASPPI==2 || RASPPI==3
    #define GPIO_BASE       0x3F200000UL
    #define CLOCK_BASE      0x3F101000UL
#else
    #define GPIO_BASE       0x20200000UL
    #define CLOCK_BASE      0x20101000UL
#endif

#if defined( RPIBPLUS ) || RASPPI==2 || RASPPI==3
    #define LED_GPFSEL      GPIO_GPFSEL4
    #define LED_GPFBIT      21
    #define LED_GPSET       GPIO_GPSET1
    #define LED_GPCLR       GPIO_GPCLR1
    #define LED_GPIO_BIT    15
    #define LED_ON()        do { gpio[LED_GPCLR] = ( 1 << LED_GPIO_BIT ); } while( 0 )
    #define LED_OFF()       do { gpio[LED_GPSET] = ( 1 << LED_GPIO_BIT ); } while( 0 )
#else
    #define LED_GPFSEL      GPIO_GPFSEL1
    #define LED_GPFBIT      18
    #define LED_GPSET       GPIO_GPSET0
    #define LED_GPCLR       GPIO_GPCLR0
    #define LED_GPIO_BIT    16
    #define LED_ON()        do { gpio[LED_GPSET] = ( 1 << LED_GPIO_BIT ); } while( 0 )
    #define LED_OFF()       do { gpio[LED_GPCLR] = ( 1 << LED_GPIO_BIT ); } while( 0 )
#endif

#define GPIO_GPFSEL0    0
#define GPIO_GPFSEL1    1
#define GPIO_GPFSEL2    2
#define GPIO_GPFSEL3    3
#define GPIO_GPFSEL4    4
#define GPIO_GPFSEL5    5

#define GPIO_GPSET0     7
#define GPIO_GPSET1     8

#define GPIO_GPCLR0     10
#define GPIO_GPCLR1     11

#define GPIO_GPLEV0     13
#define GPIO_GPLEV1     14

#define GPIO_GPEDS0     16
#define GPIO_GPEDS1     17

#define GPIO_GPREN0     19
#define GPIO_GPREN1     20

#define GPIO_GPFEN0     22
#define GPIO_GPFEN1     23

#define GPIO_GPHEN0     25
#define GPIO_GPHEN1     26

#define GPIO_GPLEN0     28
#define GPIO_GPLEN1     29

#define GPIO_GPAREN0    31
#define GPIO_GPAREN1    32

#define GPIO_GPAFEN0    34
#define GPIO_GPAFEN1    35

#define GPIO_GPPUD      37
#define GPIO_GPPUDCLK0  38
#define GPIO_GPPUDCLK1  39

#define GPIO_FSEL0_IN    0x0 // GPIO Function Select: GPIO Pin X0 Is An Input
#define GPIO_FSEL0_OUT   0x1 // GPIO Function Select: GPIO Pin X0 Is An Output
#define GPIO_FSEL0_ALT0  0x4 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 0
#define GPIO_FSEL0_ALT1  0x5 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 1
#define GPIO_FSEL0_ALT2  0x6 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 2
#define GPIO_FSEL0_ALT3  0x7 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 3
#define GPIO_FSEL0_ALT4  0x3 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 4
#define GPIO_FSEL0_ALT5  0x2 // GPIO Function Select: GPIO Pin X0 Takes Alternate Function 5
#define GPIO_FSEL0_CLR   0x7 // GPIO Function Select: GPIO Pin X0 Clear Bits

#define GPIO_FSEL1_IN     0x0 // GPIO Function Select: GPIO Pin X1 Is An Input
#define GPIO_FSEL1_OUT    0x8 // GPIO Function Select: GPIO Pin X1 Is An Output
#define GPIO_FSEL1_ALT0  0x20 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 0
#define GPIO_FSEL1_ALT1  0x28 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 1
#define GPIO_FSEL1_ALT2  0x30 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 2
#define GPIO_FSEL1_ALT3  0x38 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 3
#define GPIO_FSEL1_ALT4  0x18 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 4
#define GPIO_FSEL1_ALT5  0x10 // GPIO Function Select: GPIO Pin X1 Takes Alternate Function 5
#define GPIO_FSEL1_CLR   0x38 // GPIO Function Select: GPIO Pin X1 Clear Bits

#define GPIO_FSEL2_IN      0x0 // GPIO Function Select: GPIO Pin X2 Is An Input
#define GPIO_FSEL2_OUT    0x40 // GPIO Function Select: GPIO Pin X2 Is An Output
#define GPIO_FSEL2_ALT0  0x100 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 0
#define GPIO_FSEL2_ALT1  0x140 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 1
#define GPIO_FSEL2_ALT2  0x180 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 2
#define GPIO_FSEL2_ALT3  0x1C0 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 3
#define GPIO_FSEL2_ALT4   0xC0 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 4
#define GPIO_FSEL2_ALT5   0x80 // GPIO Function Select: GPIO Pin X2 Takes Alternate Function 5
#define GPIO_FSEL2_CLR   0x1C0 // GPIO Function Select: GPIO Pin X2 Clear Bits

#define GPIO_FSEL3_IN      0x0 // GPIO Function Select: GPIO Pin X3 Is An Input
#define GPIO_FSEL3_OUT   0x200 // GPIO Function Select: GPIO Pin X3 Is An Output
#define GPIO_FSEL3_ALT0  0x800 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 0
#define GPIO_FSEL3_ALT1  0xA00 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 1
#define GPIO_FSEL3_ALT2  0xC00 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 2
#define GPIO_FSEL3_ALT3  0xE00 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 3
#define GPIO_FSEL3_ALT4  0x600 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 4
#define GPIO_FSEL3_ALT5  0x400 // GPIO Function Select: GPIO Pin X3 Takes Alternate Function 5
#define GPIO_FSEL3_CLR   0xE00 // GPIO Function Select: GPIO Pin X3 Clear Bits

#define GPIO_FSEL4_IN       0x0 // GPIO Function Select: GPIO Pin X4 Is An Input
#define GPIO_FSEL4_OUT   0x1000 // GPIO Function Select: GPIO Pin X4 Is An Output
#define GPIO_FSEL4_ALT0  0x4000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 0
#define GPIO_FSEL4_ALT1  0x5000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 1
#define GPIO_FSEL4_ALT2  0x6000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 2
#define GPIO_FSEL4_ALT3  0x7000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 3
#define GPIO_FSEL4_ALT4  0x3000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 4
#define GPIO_FSEL4_ALT5  0x2000 // GPIO Function Select: GPIO Pin X4 Takes Alternate Function 5
#define GPIO_FSEL4_CLR   0x7000 // GPIO Function Select: GPIO Pin X4 Clear Bits

#define GPIO_FSEL5_IN        0x0 // GPIO Function Select: GPIO Pin X5 Is An Input
#define GPIO_FSEL5_OUT    0x8000 // GPIO Function Select: GPIO Pin X5 Is An Output
#define GPIO_FSEL5_ALT0  0x20000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 0
#define GPIO_FSEL5_ALT1  0x28000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 1
#define GPIO_FSEL5_ALT2  0x30000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 2
#define GPIO_FSEL5_ALT3  0x38000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 3
#define GPIO_FSEL5_ALT4  0x18000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 4
#define GPIO_FSEL5_ALT5  0x10000 // GPIO Function Select: GPIO Pin X5 Takes Alternate Function 5
#define GPIO_FSEL5_CLR   0x38000 // GPIO Function Select: GPIO Pin X5 Clear Bits

#define GPIO_FSEL6_IN         0x0 // GPIO Function Select: GPIO Pin X6 Is An Input
#define GPIO_FSEL6_OUT    0x40000 // GPIO Function Select: GPIO Pin X6 Is An Output
#define GPIO_FSEL6_ALT0  0x100000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 0
#define GPIO_FSEL6_ALT1  0x140000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 1
#define GPIO_FSEL6_ALT2  0x180000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 2
#define GPIO_FSEL6_ALT3  0x1C0000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 3
#define GPIO_FSEL6_ALT4   0xC0000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 4
#define GPIO_FSEL6_ALT5   0x80000 // GPIO Function Select: GPIO Pin X6 Takes Alternate Function 5
#define GPIO_FSEL6_CLR   0x1C0000 // GPIO Function Select: GPIO Pin X6 Clear Bits

#define GPIO_FSEL7_IN         0x0 // GPIO Function Select: GPIO Pin X7 Is An Input
#define GPIO_FSEL7_OUT   0x200000 // GPIO Function Select: GPIO Pin X7 Is An Output
#define GPIO_FSEL7_ALT0  0x800000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 0
#define GPIO_FSEL7_ALT1  0xA00000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 1
#define GPIO_FSEL7_ALT2  0xC00000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 2
#define GPIO_FSEL7_ALT3  0xE00000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 3
#define GPIO_FSEL7_ALT4  0x600000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 4
#define GPIO_FSEL7_ALT5  0x400000 // GPIO Function Select: GPIO Pin X7 Takes Alternate Function 5
#define GPIO_FSEL7_CLR   0xE00000 // GPIO Function Select: GPIO Pin X7 Clear Bits

#define GPIO_FSEL8_IN          0x0 // GPIO Function Select: GPIO Pin X8 Is An Input
#define GPIO_FSEL8_OUT   0x1000000 // GPIO Function Select: GPIO Pin X8 Is An Output
#define GPIO_FSEL8_ALT0  0x4000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 0
#define GPIO_FSEL8_ALT1  0x5000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 1
#define GPIO_FSEL8_ALT2  0x6000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 2
#define GPIO_FSEL8_ALT3  0x7000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 3
#define GPIO_FSEL8_ALT4  0x3000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 4
#define GPIO_FSEL8_ALT5  0x2000000 // GPIO Function Select: GPIO Pin X8 Takes Alternate Function 5
#define GPIO_FSEL8_CLR   0x7000000 // GPIO Function Select: GPIO Pin X8 Clear Bits

#define GPIO_FSEL9_IN           0x0 // GPIO Function Select: GPIO Pin X9 Is An Input
#define GPIO_FSEL9_OUT    0x8000000 // GPIO Function Select: GPIO Pin X9 Is An Output
#define GPIO_FSEL9_ALT0  0x20000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 0
#define GPIO_FSEL9_ALT1  0x28000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 1
#define GPIO_FSEL9_ALT2  0x30000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 2
#define GPIO_FSEL9_ALT3  0x38000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 3
#define GPIO_FSEL9_ALT4  0x18000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 4
#define GPIO_FSEL9_ALT5  0x10000000 // GPIO Function Select: GPIO Pin X9 Takes Alternate Function 5
#define GPIO_FSEL9_CLR   0x38000000 // GPIO Function Select: GPIO Pin X9 Clear Bits

#define GZ_CLK_BUSY    (1 << 7)
#define GP_CLK0_CTL     0x1C
#define GP_CLK0_DIV     0x1D

#endif
