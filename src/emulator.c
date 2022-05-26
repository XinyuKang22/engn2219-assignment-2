#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros
#define ADDRSPACE (1 << 16)
#define RZ 0
#define FL 5
#define PC 7

// Functions
void save_state();
void parse_inst();
void exec_inst();
void process_str();
void disasm();

// Global variables for ram and the register file
unsigned short ram[ADDRSPACE];
unsigned short registers[8];
unsigned short Zflag, Nflag, Cflag, Vflag;
unsigned short inst, op, z, rd, ra, rb, imm8;
unsigned short bp = 0;
char s_z[2];
char s_rd[3];
char s_ra[3];
char s_rb[3];
char* disasm_str[20];
struct trace { // linked list node storing trace of each execution
    char* disasm_str;
    unsigned short rz;
    unsigned short r1;
    unsigned short r2;
    unsigned short r3;
    unsigned short r4;
    unsigned short fl;
    unsigned short pc;
    struct trace* next;
};
typedef struct trace new_trace;
new_trace head = {NULL, 0, 0, 0, 0, 0, 0, NULL};  // head of the linked list
new_trace* p_tail = &head;  // pointer to the tail of the linked list


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
                printf("-----------------------\n");
                printf("Index Mnemonic Value\n");
                printf("-----------------------\n");
                printf("0     rz       %-06hi\n", registers[0]);
                printf("1     r1       %-06hi\n", registers[1]);
                printf("2     r2       %-06hi\n", registers[2]);
                printf("3     r3       %-06hi\n", registers[3]);
                printf("4     r4       %-06hi\n", registers[4]);
                printf("5     fl       %-06hi\n", registers[5]);
                printf("6     -        -\n");
                printf("7     pc       %-05u\n", registers[7]);
                printf("-----------------------\n");
                break;

            case 'm':;  // View memory
                unsigned short start;
                unsigned short end;
                if (sscanf(line, "m %hi %hi", &start, &end) != 2) {
                    printf("Usage: m START_ADDR END_ADDR\n");
                    break;
                }
                printf("---------------------------------------------\n");
                printf("Address       Value        Instruction\n");
                printf("---------------------------------------------\n");
                for (int i = start; i < end; i++) {
                    inst = ram[i];
                    printf("0x%04X        0x%04X       ", i, inst);
                    disasm();
                }
                printf("-------------------------------------------\n");
                printf("Showing memory from addresses %i to %i\n", start, end);
                break;

            case 'c':  // Continue until next breakpoint
                printf("continuing until next breakpoint...\n");
                int count = 0;
                while ((registers[PC] - bp) != 0) {
                    exec_inst();
                    count++;
                }
                printf("\nHalted on address 0x%04X\n", registers[PC]);
                printf("Executed %i instruction(s)\n", count);
                break;

            case 'b':;  // Enable/disable breakpoints
                unsigned short bp_addr;
                if (sscanf(line, "b %hi", &bp_addr) != 1) {
                    printf("Usage: b BREAKPOINT_ADDR\n");
                    break;
                }
                if (bp_addr == bp) {
                    bp = 0;  // removes if already there
                } else {
                    bp = bp_addr;
                }
                printf("Toggling breakpoint at address %i\n", bp);
                break;

            case 't':  // Print execution trace.
                printf("------------------------------------------------------------\n");
                printf("pc    rz     r1     r2     r3     r4     fl    Instruction\n");
                printf("------------------------------------------------------------\n");
                // starting from the head of the linked list
                // keep printing the content of the node until reach the tail node
                new_trace* curr = head.next;
                while (curr != NULL) {
                    printf("%-05u %-06hi %-06hi %-06hi %-06hi %-06hi %-05u %s\n", curr->pc, 
                    curr->rz, curr->r1, curr->r2, curr->r3, curr->r4, curr->fl, curr->disasm_str);
                    curr = curr->next;
                }
                printf("------------------------------------------------------------\n");
                break;

            default:
                printf("Unrecognised command '%c'\n", cmd);
                break;
        }
    }
}


// Execute one instruction
void exec_inst() {
    inst = ram[registers[PC]];
    registers[PC] = registers[PC] + 1;
    // print the disassembled instruction
    disasm();
    // Writing/reading the undefined register will be ignored
    if (rd == 6 || ra == 6 || rb == 6) {
        save_state();
        return;
    }
    // Conditional execution on z-bit
    if (z == 1 && Zflag == 0) {
        save_state();
        return;
    }
    signed short a, b, c;
    int c_raw;
    switch (op) {
        case 0b0000:    // movl
            if (rd == RZ) {
                break;
            }
            registers[rd] = imm8;
            break;

        case 0b0001:    // seth
            if (rd == RZ) {
                break;
            }
            registers[rd] = (imm8 << 8) + (registers[rd] & 0b0000000011111111);
            break;

        case 0b0100:    // str
            ram[registers[ra]] = registers[rd];
            break;

        case 0b0101:    // ldr
            if (rd == RZ) {
                break;
            }
            registers[rd] = ram[registers[ra]];
            break;

        case 0b1000:    // add
            a = registers[ra];
            b = registers[rb];
            c_raw = a + b;
            c = (signed short) c_raw;
            if (rd != 0) {
                registers[rd] = c_raw;
            }
            Zflag = ((unsigned short) c_raw == 0);
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
            if (rd != 0) {
                registers[rd] = c_raw;
            }
            Zflag = ((unsigned short) c_raw == 0);
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
            if (rd != 0) {
                registers[rd] = c_raw;
            }
            Zflag = ((unsigned short) c_raw == 0);
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
            if (rd != 0) {
                registers[rd] = c_raw;
            }
            Zflag = ((unsigned short) c_raw == 0);
            Nflag = (c < 0);
            Cflag = c_raw << 16;
            Vflag = (c_raw > 32767) | (c_raw < -32768);
            registers[FL] = Zflag + (Nflag << 1) + (Cflag << 2) + (Vflag << 3);
            break;

        default:
            break;
    }
    save_state();
    return;
}


// Print disassembly
void disasm() {
    parse_inst();
    if (rd == 6 || ra == 6 || rb == 6) {
        return sprintf(disasm_str, ".word %#X", inst);
    }
    process_str();
    int res_sprintf = 0;
    switch (op) {
        case 0b0000:    // movl
            switch (rd) {
                case 0:
                    res_sprintf = sprintf(disasm_str, ".word %i", inst);
                    break;
                case PC: // jp -- alias for `MOVL PC, IMM8`
                    res_sprintf = sprintf(disasm_str, "jp%s %i", s_z, imm8);
                    break;
                default:
                    res_sprintf = sprintf(disasm_str, "movl%s %02s, %i", s_z, s_rd, imm8);
                    break;
            }
            break;

        case 0b0001:    // seth
            res_sprintf = sprintf(disasm_str, "seth%s %02s, %i", s_z, s_rd, imm8);
            break;

        case 0b0100:    // str
            res_sprintf = sprintf(disasm_str, "str%s %02s, [%02s]", s_z, s_rd, s_ra);
            break;

        case 0b0101:    // ldr
            if (rd == PC) {  // jpm -- alias for `LDR PC, [RA]`
                res_sprintf = sprintf(disasm_str, "jpm%s [%02s]", s_z, s_ra);
            } else {
                res_sprintf = sprintf(disasm_str, "ldr%s %02s, [%02s]", s_z, s_rd, s_ra);
            }
            break;

        case 0b1000:    // add
            if (rb == RZ) {
                switch (rd) {
                    case 0: // tst -- alias for `ADD RZ, RA, RZ`
                        res_sprintf = sprintf(disasm_str, "tst%s %02s", s_z, s_ra);
                        break;
                    case PC: // jpr -- alias for `ADD PC, RA, RZ`
                        res_sprintf = sprintf(disasm_str, "jpr%s %02s", s_z, s_ra);
                        break;
                    default: // mov -- alias for `ADD RD, RA, RZ`
                        res_sprintf = sprintf(disasm_str, "mov%s %02s, %02s", s_z, s_rd, s_ra);
                        break;
                }
            } else {
                if (rd == ra) {
                    res_sprintf = sprintf(disasm_str, "add%s %02s, %02s", s_z, s_ra, s_rb);
                } else {
                    res_sprintf = sprintf(disasm_str, "add%s %02s, %02s, %02s", s_z, s_rd, s_ra, s_rb);
                }
            }
            break;

        case 0b1001:    // sub
            if (rd == RZ) {  // cmp -- alias for `SUB RZ, RA, RB`
                res_sprintf = sprintf(disasm_str, "cmp%s %02s, %02s", s_z, s_ra, s_rb);
            } else {
                if (rd == ra) {
                    res_sprintf = sprintf(disasm_str, "sub%s %02s, %02s", s_z, s_ra, s_rb);
                } else {
                    res_sprintf = sprintf(disasm_str, "sub%s %02s, %02s, %02s", s_z, s_rd, s_ra, s_rb);
                }
            }
            break;

        case 0b1010:    // and
            if (rd == ra) {
                res_sprintf = sprintf(disasm_str, "and%s %02s, %02s", s_z, s_ra, s_rb);
            } else {
                res_sprintf = sprintf(disasm_str, "and%s %02s, %02s, %02s", s_z, s_rd, s_ra, s_rb);
            }
            break;

        case 0b1011:    // orr
            if (rd == ra) {
                res_sprintf = sprintf(disasm_str, "orr%s %02s, %02s", s_z, s_ra, s_rb);
            } else {
                res_sprintf = sprintf(disasm_str, "orr%s %02s, %02s, %02s", s_z, s_rd, s_ra, s_rb);
            }
            break;
            
        default:
            res_sprintf = sprintf(disasm_str, ".word %i", inst);
            break;
    }
    if (res_sprintf > 0) {
        printf("%s\n", disasm_str);
    }
    return;
}


// Save a copy of the CPU’s architectural state
void save_state() {
    new_trace* item = malloc(sizeof(new_trace));
    char* the_disasm_str = malloc(sizeof(disasm_str));
    strcpy(the_disasm_str, disasm_str);
    item->disasm_str = the_disasm_str;
    item->rz = registers[0];
    item->r1 = registers[1];
    item->r2 = registers[2];
    item->r3 = registers[3];
    item->r4 = registers[4];
    item->fl = registers[5];
    item->pc = registers[7];
    p_tail->next = item;
    p_tail = item;
}


// Parse the machine code into {op, z, rd, ra, rb}
void parse_inst() {
    op =  inst >> 12;
    z =  (inst & 0b0000100000000000) >> 11;
    rd = (inst & 0b0000011100000000) >> 8;
    if (op < 4) {
        imm8 = inst & 0b0000000011111111;
    } else {
        ra = (inst & 0b0000000001110000) >> 4;
        rb = inst & 0b0000000000000111;
    }
    return;
}


// Process the strings representing {z, rd, ra, rb}
void process_str() {
    s_z[1] = '\0';
    if (z) {
        s_z[0] = 'z';
    } else {
        s_z[0] = '\0';
    }
    
    s_rd[2] = '\0';
    switch (rd) {
        case 0:
            s_rd[0] = 'r';
            s_rd[1] = 'z';
            break;
        case 5:
            s_rd[0] = 'f';
            s_rd[1] = 'l';
            break;
        case 7:
            s_rd[0] = 'p';
            s_rd[1] = 'c';
            break;
        default:
            s_rd[0] = 'r';
            s_rd[1] = rd + '0';
            break;
    }

    s_ra[2] = '\0';
    switch (ra) {
        case 0:
            s_ra[0] = 'r';
            s_ra[1] = 'z';
            break;
        case 5:
            s_ra[0] = 'f';
            s_ra[1] = 'l';
            break;
        case 7:
            s_ra[0] = 'p';
            s_ra[1] = 'c';
            break;
        default:
            s_ra[0] = 'r';
            s_ra[1] = ra + '0';
            break;
    }

    s_rb[2] = '\0';
    switch (rb) {
        case 0:
            s_rb[0] = 'r';
            s_rb[1] = 'z';
            break;
        case 5:
            s_rb[0] = 'f';
            s_rb[1] = 'l';
            break;
        case 7:
            s_rb[0] = 'p';
            s_rb[1] = 'c';
            break;
        default:
            s_rb[0] = 'r';
            s_rb[1] = rb + '0';
            break;
    }
    return;
}