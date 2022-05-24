#include <stdio.h>
#include <stdlib.h>

// Macros
#define ADDRSPACE (1 << 16)
#define RZ 0
#define FL 5
#define PC 7
#define MAX = 32767;
#define MIN = -32768;

// Global variables for ram and the register file
unsigned short ram[ADDRSPACE];
unsigned short registers[8];
unsigned short pc = 0;
unsigned short Zflag = 0;

// FIXME: Instruction Execution (30%)
void exec_inst() {
    unsigned short inst = ram[pc++];
    printf("inst = %u\n", inst);
    unsigned short op =  inst >> 12;
    unsigned short z =  (inst & 0b0000100000000000) >> 11;
    if ((z == 1) & (Zflag == 0)) {
        return;
    }
    unsigned short rd = (inst & 0b0000011100000000) >> 8;
    unsigned short ra, rb, imm8;
    if (op < 4) {
        imm8 =           inst & 0b0000000011111111;
    } else {
        ra   =          (inst & 0b0000000001110000) >> 4;
        rb   =           inst & 0b0000000000000111;
    }
    
    signed short a, b, c;
    switch (op) {
        case 0b0000:    // movl
            registers[rd] = imm8;
            break;
        case 0b0001:    // seth
            registers[rd] = (imm8 << 8) + (registers[rd] & 0b0000000011111111);
            break;
        case 0b0100:    // str
            ram[registers[ra]] = registers[rd];
            break;
        case 0b0101:    // ldr
            registers[rd] = ram[registers[ra]];
            break;
        case 0b1000:    // add
            a = registers[ra];
            b = registers[rb];
            c = a + b;
            Zflag = (c != 0);
            registers[rd] = c;
            break;
        case 0b1001:    // sub
            a = registers[ra];
            b = registers[rb];
            c = a - b;
            Zflag = (c != 0);
            registers[rd] = c;
            break;
        case 0b1010:    // and
            a = registers[ra];
            b = registers[rb];
            c = a & b;
            Zflag = (c != 0);
            registers[rd] = c;
            break;
        case 0b1011:    // orr
            a = registers[ra];
            b = registers[rb];
            c = a | b;
            Zflag = (c != 0);
            registers[rd] = c;
            break;
        default:
            break;
    }
    return;
}


// Note: you will almost certainly need to change the arguments and/or return
// type for your disasm function
void disasm() {

    // FIXME: Print disassembly (15%)

    return;
}

/*
argc[1]: (char*) program_path
ram[i] = (unsigned short) ((ch << 8) | cl);
*/
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("\nUsage: %s PATH_TO_PROGRAM\n", argv[0]);
        if (argc < 2) {
            printf("Error: Missing PATH_TO_PROGRAM\n");
        } else {
            printf("Error: Too many arguments\n");
        }
        return 1;
    }

    // Initialize ram to zero
    for (int i = 0; i < ADDRSPACE; i++) {
        ram[i] = 0;
    }

    // Initialize registers to zero
    for (int i = 0; i < 8; i++) {
        registers[i] = 0;
    }

    char *program_path = argv[1];
    printf("Loading program '%s'\n", program_path);

    FILE *fp = fopen(program_path, "r");
    int cl, ch;
    for (int i = 0;; i++) {
        // Read two chars from the file, assemble them into a 16 bit
        // word, and insert in to the `ram` array
        if ((cl = fgetc(fp)) == EOF || (ch = fgetc(fp)) == EOF) {
            printf("Read %d bytes in to memory\n", i * 2);
            fclose(fp);
            break;
        }

        ram[i] = (unsigned short) ((ch << 8) | cl);

        if (i == ADDRSPACE) {
            printf("Error: input file too large (more than %i bytes)\n", ADDRSPACE);
            fclose(fp);
            return 1;
        }
    }

    // Main emulator loop
    while (1) {
        char buffer[256];
        printf("\nCommand (? for help): ");

        char *line = fgets(buffer, sizeof(buffer), stdin);
        if (line == NULL) {
            printf("\nError: failed to read command line\n");
            return 1;
        }

        char cmd = line[0];
        if (cmd == '\n' || cmd == '\r' || cmd == '\0') {  // skip empty lines
            continue;
        }

        switch (cmd) {
            case '?':  // Help
                printf("? - Show this help\n");
                printf("q - Quit\n");
                printf("s - Step CPU, executing one instruction\n");
                printf("r - Print register contents\n");
                printf("m - View memory\n");
                printf("c - Continue program execution until a breakpoint is reached\n");
                printf("b - Toggle a breakpoint\n");
                printf("t - Print execution trace\n");
                break;

            case 'q':  // Quit
                return 0;

            case 's':  // Step (execute one instruction)
                exec_inst();
                break;

            case 'r':  // Print register contents

                // FIXME: Register Viewer (15%)

                break;

            case 'm':;  // View memory
                unsigned short start;
                unsigned short end;
                if (sscanf(line, "m %hi %hi", &start, &end) != 2) {
                    printf("Usage: m START_ADDR END_ADDR\n");
                    break;
                }

                // FIXME: Memory Viewer (15%)
                printf("Showing memory from addresses %i to %i\n", start, end);

                break;

            case 'c':  // Continue until next breakpoint
                printf("Continuing until next breakpoint\n");

                // FIXME: Breakpoints (15%) (part 1/2)

                printf("Halted on address 0x%#06x\n", registers[PC]);
                break;

            case 'b':;  // Enable/disable breakpoints
                unsigned short bp_addr;
                if (sscanf(line, "b %hi", &bp_addr) != 1) {
                    printf("Usage: b BREAKPOINT_ADDR\n");
                    break;
                }

                // FIXME: Breakpoints (15%) (part 2/2)
                printf("Toggling breakpoint at address %i\n", bp_addr);

                break;

            case 't':  // Print execution trace.

                // FIXME: Execution Trace (10%)

                break;

            default:
                printf("Unrecognised command '%c'\n", cmd);
                break;
        }
    }
}
