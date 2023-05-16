#ifndef ZMXBUS_HH
#define ZMXBUS_HH

enum {
    RD_MEM, RD_SLOT1, RD_SLOT2, RD_IO,
    WR_MEM, WR_SLOT1, WR_SLOT2, WR_IO
};

#define MSXREAD "msxread"
#define MSXWRITE "msxwrite"
#define MSXRESET "reset"
#define MSXINIT "init"

typedef unsigned char (*ReadfnPtr)(int, unsigned short);
typedef void (*WritefnPtr)(int, unsigned short, unsigned char);
typedef void (*InitfnPtr)(char *);
typedef void (*ResetfnPtr)(void);

#endif
