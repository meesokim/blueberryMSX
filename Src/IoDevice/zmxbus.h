#define MSXREAD "msxread"
#define MSXWRITE "msxwrite"
#define MSXRESET "reset"
#define MSXSTATUS "msxstatus"
#define MSXINIT "init"

typedef unsigned char (*ReadfnPtr)(int, unsigned short);
typedef void (*WritefnPtr)(int, unsigned short, unsigned char);
typedef void (*ResetfnPtr)(char );
typedef void (*InitfnPtr)(char *);
typedef unsigned char (*StatusfnPtr)(void);

#define RD_SLTSL1 0x00
#define RD_SLTSL2 0x10
#define RD_MEM    0x20
#define RD_IO     0x02
#define WR_SLTSL1 0x01
#define WR_SLTSL2 0x11
#define WR_MEM    0x21
#define WR_IO     0x03
