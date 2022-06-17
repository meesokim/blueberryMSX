#ifndef RPMC

#define RPMC
#define RPMCV8

#define RD0		0
#define RD1		1
#define RD2		2
#define RD3		3
#define RD4		4
#define RD5		5
#define RD6		6
#define RD7		7
#define RA8		8
#define RA9		9
#define RA10	10
#define RA11	11
#define RA12	12
#define RA13	13
#define RA14	14
#define RA15	15
#define RC16	16
#define RC17	17
#define RC18	18
#define RC19	19
#define RC20	20
#define RC21	21
#define RC22	22
#define RC23	23
#define RC24	24
#define RC25	25
#define RC26	26
#define RC27	27

#define MD00_PIN 	0
#define SLTSL1_PIN 	RA8
#define SLTSL3_PIN	RA9
#define CS1_PIN		RA10
#define CS2_PIN 	RA11
#define RD_PIN		RA12
#define WR_PIN		RA13
#define IORQ_PIN	RA14
#define MREQ_PIN	RA15
#define LE_A_PIN	RC16
#define LE_C_PIN	RC17
#define LE_D_PIN	RC18
#define RESET_PIN	RC19
#define CLK_PIN		RC20
#define INT_PIN		RC24
#define WAIT_PIN	RC25
#define BUSDIR_PIN	RC26
#define SW1_PIN		RC27

#define MSX_SLTSL1 (1 << SLTSL1_PIN)
#define MSX_SLTSL3 (1 << SLTSL3_PIN)
#define MSX_CS1	(1 << CS1_PIN)
#define MSX_CS2 (1 << CS2_PIN)
#define MSX_CS12 (1 << CS12_PIN)
#define MSX_RD	(1 << RD_PIN)
#define MSX_WR  (1 << WR_PIN)
#define MSX_IORQ  (1 << IORQ_PIN)
#define MSX_MREQ	(1 << MREQ_PIN)
#define MSX_RESET (1 << RESET_PIN)
#define MSX_WAIT	(1 << WAIT_PIN)
#define MSX_INT		(1 << INT_PIN)
#define LE_A	(1 << LE_A_PIN)
#define LE_C	(1 << LE_C_PIN)
#define LE_D	(1 << LE_D_PIN)
#define DAT_DIR (1 << RC21)

#define MSX_CONTROLS	(MSX_SLTSL1 | MSX_SLTSL3 | MSX_MREQ | MSX_IORQ | MSX_RD | MSX_WR | MSX_CS1 | MSX_CS2)

extern volatile unsigned int* gpio;

inline void GPIO_SET(unsigned int b)
{
	gpio[GPIO_GPSET0] = b;
}

inline void GPIO_CLR(unsigned int b)
{
	gpio[GPIO_GPCLR0] = b;
}

inline void GPIO_PUT(unsigned int a, unsigned int b)
{
	gpio[GPIO_GPSET0] = a;
	gpio[GPIO_GPCLR0] = b;
}

#define GPIO_GET() (gpio[GPIO_GPLEV0])

#endif