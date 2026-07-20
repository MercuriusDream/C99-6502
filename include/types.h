#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define CPU_STACK_BASE 0x0100
#define CPU_STACK_TOP 0x01FF

#define PHY_MEM_SIZE 65536
#define STACK_START CPU_STACK_BASE
#define STACK_END CPU_STACK_TOP

#define CPU_RESET_STACK_POINTER 0xFD
#define CPU_RESET_STATUS 0x24
#define CPU_RESET_VECTOR_ADDRESS 0xFFFC

#define CPU_TEST_RUN_LIMIT 10000
#define MAX_LOG_LENGTH 1024

typedef enum
{
    CPU_VARIANT_NMOS_6502 = 0,
    CPU_VARIANT_CMOS_65C02
} CPU_VARIANT;

#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_U 0x20
#define FLAG_V 0x40
#define FLAG_N 0x80

#define SET_FLAG(b) (REG.P |= (b))
#define CLR_FLAG(b) (REG.P &= ~(b))
#define GET_FLAG(b) ((REG.P & (b)) != 0)

#define CMD_LEN 5

typedef int8_t SIGNED_MEM_WORD;
typedef int16_t SIGNED_MEM_TWO_WORDS;
typedef unsigned char MEM_WORD;
typedef unsigned short MEM_TWO_WORDS;

typedef MEM_WORD (*bus_read_fn)(MEM_TWO_WORDS ADDR, void* CTX);
typedef void (*bus_write_fn)(MEM_TWO_WORDS ADDR, MEM_WORD DATA, void* CTX);

typedef struct
{
    bus_read_fn READ;
    bus_write_fn WRITE;
    void* CTX;
} T_BUS;

typedef enum
{
    MEM_REGION_NONE = 0,
    MEM_REGION_RAM,
    MEM_REGION_ROM,
    MEM_REGION_IO
} MEM_REGION_TYPE;

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LOGGING_TYPES;

typedef struct
{
    MEM_TWO_WORDS START;
    MEM_TWO_WORDS END;
    MEM_REGION_TYPE TYPE;
    MEM_WORD* DATA;
    bus_read_fn READ_HANDLER;
    bus_write_fn WRITE_HANDLER;
    void* CTX;
} MEM_REGION;

#define MAX_MEM_REGIONS 16

typedef enum
{
    ADDR_NONE = 0,
    ADDR_ACC,
    ADDR_IMM,
    ADDR_ZP,
    ADDR_ZPX,
    ADDR_ZPY,
    ADDR_ABS,
    ADDR_ABSX,
    ADDR_ABSY,
    ADDR_IND,
    ADDR_INDX,
    ADDR_INDY,
    ADDR_REL,
    ADDR_ZPREL,
    ADDR_ZP_IND,
    ADDR_ABS_IND_X
} ADDR_MODE;

typedef enum
{
    REG_NONE = 0,
    REG_A,
    REG_X,
    REG_Y,
    REG_SP,
    REG_ACC
} REG_TYPE;

typedef struct
{
    const char CMD[CMD_LEN];
    ADDR_MODE ADDR;
    REG_TYPE SRC;
    REG_TYPE DEST;
} INST;

typedef struct
{
    MEM_TWO_WORDS PC;
    MEM_WORD A;
    MEM_WORD P;
    MEM_WORD S;
    MEM_WORD X;
    MEM_WORD Y;
} T_REGISTER;

extern T_REGISTER REG;
extern T_BUS BUS;

#endif // EOF
