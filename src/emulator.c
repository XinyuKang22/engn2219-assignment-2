#include <stdio.h>
#include <stdlib.h>

// Macros
#define ADDRSPACE (1 << 16)
#define RZ 0
#define FL 5
#define PC 7

// Global variables for ram and the register file
unsigned short ram[ADDRSPACE];
unsigned short registers[8];
unsigned short Zflag, Nflag, Cflag, Vflag;

void parse_inst(unsigned short inst, unsigned short* p_op, unsigned short* p_z,
                unsigned short* p_rd, unsigned short* p_ra, unsigned short* p_rb,
                unsigned short* p_imm8) {
    *p_op =  inst >> 12;
    *p_z =  (inst & 0b0000100000000000) >> 11;
    *p_rd = (inst & 0b0000011100000000) >> 8;
    if (*p_op < 4) {
        *p_imm8 = inst & 0b0000000011111111;
    } else {
        *p_ra = (inst & 0b0000000001110000) >> 4;
        *p_rb = inst & 0b0000000000000111;
    }
    return;
}



// FIXME: Instruction Execution (30%)
void exec_inst() {
    unsigned short inst = ram[registers[PC]];
    registers[PC] = registers[PC] + 1;

    unsigned short op, z, rd, ra, rb, imm8;
    parse_inst(inst, &op, &z, &rd, &ra, &rb, &imm8);
    disasm(op, z, rd, ra, rb, imm8);
    if ((z == 1) & (Zflag == 0)) {
        return;
    }
    
    signed short a, b, c;
    int c_raw;
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
            c_raw = a + b;
            c = (signed short) c_raw;
            registers[rd] = c;

            Zflag = (c == 0);
            Nflag = (c < 0);
            Cflag = c_raw << 16;
            Vflag = (c_raw > 32767) | (c_raw < -32768);
            registers[FL] = Zflag + (Nflag << 1) + (Cflag << 2) + (Vflag << 3);
            break;
        case 0b1001:    // sub
            a = registers[ra];
            b = registers[rb];
            c_raw = a - b;
            c = (signed short) c_raw;
            registers[rd] = c;

            Zflag = (c == 0);
            Nflag = (c < 0);
            Cflag = c_raw << 16;
            Vflag = (c_raw > 32767) | (c_raw < -32768);
            registers[FL] = Zflag + (Nflag << 1) + (Cflag << 2) + (Vflag << 3);
            break;
        case 0b1010:    // and
            a = registers[ra];
            b = registers[rb];
            c_raw = a & b;
            c = (signed short) c_raw;
            registers[rd] = c;

            Zflag = (c == 0);
            Nflag = (c < 0);
            Cflag = c_raw << 16;
            Vflag = (c_raw > 32767) | (c_raw < -32768);
            registers[FL] = Zflag + (Nflag << 1) + (Cflag << 2) + (Vflag << 3);
            break;
        case 0b1011:    // orr
            a = registers[ra];
            b = registers[rb];
            c_raw = a | b;
            c = (signed short) c_raw;
            registers[rd] = c;

            Zflag = (c == 0);
            Nflag = (c < 0);
            Cflag = c_raw << 16;
            Vflag = (c_raw > 32767) | (c_raw < -32768);
            registers[FL] = Zflag + (Nflag << 1) + (Cflag << 2) + (Vflag << 3);
            break;
        default:
            break;
    }
    return;
}

// FIXME: Print disassembly (15%)
// Note: you will almost certainly need to change the arguments and/or return
// type for your disasm function
void disasm(unsigned short op, unsigned short z, unsigned short rd, unsigned short ra, unsigned short rb, unsigned short imm8) {
    switch (op) {
        case 0b0000:    // movl
            if ((rd == 0) & (imm8 == 0)){
                printf("nop\n");
            } else {
                printf("movl{%i} r%i, 0x%02X\n", z, rd, imm8);
            }
            break;
        case 0b0001:    // seth
            printf("seth{%i} r%i, 0x%02X\n", z, rd, imm8);
            break;
        case 0b0100:    // str
            printf("str{%i} r%i, [r%i]\n", z, rd, ra);
            break;
        case 0b0101:    // ldr
            printf("ldr{%i} r%i, [r%i]\n", z, rd, ra);
            break;
        case 0b1000:    // add
            printf("add{%i} r%i, r%i, r%i\n", z, rd, ra, rb);
            break;
        case 0b1001:    // sub
            printf("sub{%i} r%i, r%i, r%i\n", z, rd, ra, rb);
            break;
        case 0b1010:    // and
            printf("and{%i} r%i, r%i, r%i\n", z, rd, ra, rb);
            break;
        case 0b1011:    // orr
            printf("orr{%i} r%i, r%i, r%i\n", z, rd, ra, rb);
            break;
        default:
            break;
    }
    return;
}

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
                printf("Zero Register:   %i\n", registers[0]);
                printf("Register 1:      %i\n", registers[1]);
                printf("Register 2:      %i\n", registers[2]);
                printf("Register 3:      %i\n", registers[3]);
                printf("Register 4:      %i\n", registers[4]);
                printf("Flag register:   %i\n", registers[5]);
                printf("undefined:       %i\n", registers[6]);
                printf("Program Counter: %i\n", registers[7]);
                break;

            case 'm':;  // View memory
                unsigned short start;
                unsigned short end;
                if (sscanf(line, "m %hi %hi", &start, &end) != 2) {
                    printf("Usage: m START_ADDR END_ADDR\n");
                    break;
                }

                // FIXME: Memory Viewer (15%)
                    printf("---------------------------------------------\n");
                    printf("Address       Value        Instruction\n");
                    printf("---------------------------------------------\n");
                for (int i = start; i < end; i++) {
                    unsigned short inst_t = ram[i];
                    printf("0x%04X        0x%04X       ", i, inst_t);
                    unsigned short op_t, z_t, rd_t, ra_t, rb_t, imm8_t;
                    parse_inst(inst_t, &op_t, &z_t, &rd_t, &ra_t, &rb_t, &imm8_t);
                    disasm(op_t, z_t, rd_t, ra_t, rb_t, imm8_t);
                }
                    printf("-------------------------------------------\n");
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
