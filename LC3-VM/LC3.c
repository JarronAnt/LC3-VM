#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit

//registers
enum
{
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    PC, /* program counter */
    COND,
    COUNT
};

// condition flags
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

//opcodes
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

//memory mapped registers 
enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
};

//trap codes
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

//define the memory for the machine 
#define MAX_MEM (1 << 16)
uint16_t memory[MAX_MEM];  /* 65536 memory locations */

//array to hold registers
uint16_t reg[COUNT];

HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;


/*
Below is a group of helper functions 
*/

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
    fdwMode = fdwOldMode
        ^ ENABLE_ECHO_INPUT  /* no input echo */
        ^ ENABLE_LINE_INPUT; /* return when one or
                                more characters are available */
    SetConsoleMode(hStdin, fdwMode); /* set new mode */
    FlushConsoleInputBuffer(hStdin); /* clear buffer */
}

void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}

uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        reg[COND] = FL_NEG;
    }
    else
    {
        reg[COND] = FL_POS;
    }
}
void read_image_file(FILE* file)
{
    /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = MAX_MEM - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}
int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}
void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

int main(int argc, const char* argv[]) {

    //load our arguments aka our lc3 VM image 
    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    //our setup
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    /* since exactly one condition flag should be set at any given time, set the Z flag */
    reg[COND] = FL_ZRO;

    /* set the PC to starting position */
    /* 0x3000 is the default */
    enum { PC_START = 0x3000 };
    reg[PC] = PC_START;

    int running = 1;
    while (running)
    {
        /* FETCH */
        uint16_t instr = mem_read(reg[PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
            //TODO: implement op codes
        }

    }
}