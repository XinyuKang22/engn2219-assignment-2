#include <stdio.h>
#include <stdlib.h>

// Macros
#define ADDRSPACE (1 << 16)
#define RZ 0
#define FL 5
#define PC 7

struct record {
	unsigned short value;
	struct record *next;
};
typedef struct record new_record;

// Global variables for ram and the register file
unsigned short ram[ADDRSPACE];
unsigned short registers[8];
unsigned short Zflag, Nflag, Cflag, Vflag;
new_record head = {0, NULL};
unsigned short top = 0;

void bp_add_del(unsigned short i) {
    new_record* item;
    new_record* prev = &head;
    new_record* curr = head.next;
    while (curr != NULL)
    {
        if (curr->value == i)
        {
            prev->next = curr->next;
            return;
        } else if (curr->value > i)
        {
            item = malloc(sizeof(new_record));
            item->value = i;
            item->next = curr;
            prev->next = item;
            return;
        } else
        {
            prev = curr;
            curr = curr->next;
        }
    }
    item = malloc(sizeof(new_record));
    item->value = i;
    prev->next = item;
    return;
}

void bp_pop() {
    if (head.next != NULL)
    {
        top = head.next->value;
        head.next = head.next->next;
    } else
    {
        top = 0;
    }
    return;
}

void printrecords() {
    printf("\n");
    printf(" the breakpoint list: \n");
    new_record* curr = head.next;
    while (curr != NULL)
    {
        printf(" %i\n", curr->value);
        curr = curr->next;
    }
}


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
    disasm(inst, op, z, rd, ra, rb, imm8);
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
void disasm(unsigned short inst, unsigned short op, unsigned short z, unsigned short rd, unsigned short ra, unsigned short rb, unsigned short imm8) {
    if (rd == 6 || ra == 6 || rb == 6)
    {
        printf(".word %#X\n", inst);
        return;
    }

    char s_z[2];
    s_z[1] = '\0';
    if (z)
    {
        s_z[0] = 'z';
    } else {
        s_z[0] = '\0';
    }
    
    char s_rd[3];
    s_rd[3] = '\0';
    switch (rd)
    {
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

    char s_ra[3];
    s_ra[3] = '\0';
    switch (ra)
    {
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

    char s_rb[3];
    s_rb[3] = '\0';
    switch (rb)
    {
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

    
    switch (op) {
        case 0b0000:    // movl
            // if (inst == 0){
            //     printf(".word 0\n");
            // } 
            switch (rd)
            {
            case 0:
                printf(".word %i\n", inst);
                break;
            case PC:
                printf("jp%s %i\n", s_z, imm8);
                break;
            default:
                printf("movl%s %02s, %i\n", s_z, s_rd, imm8);
                break;
            }
            break;
        case 0b0001:    // seth
            printf("seth%s %02s, %i\n", s_z, s_rd, imm8);
            break;
        case 0b0100:    // str
            printf("str%s %02s, [%02s]\n", s_z, s_rd, s_ra);
            break;
        case 0b0101:    // ldr
            if (rd == PC)
            {
                printf("jpm%s [%02s]\n", s_z, s_ra);
            } else
            {
                printf("ldr%s %02s, [%02s]\n", s_z, s_rd, s_ra);
            }
            break;
        case 0b1000:    // add
            if (rb == 0)
            {
                switch (rd)
                {
                case 0:
                    printf("tst%s %02s\n", s_z, s_ra);
                    break;
                case PC:
                    printf("jrp%s %02s\n", s_z, s_ra);
                    break;
                default:
                    printf("mov%s %02s, %02s\n", s_z, s_rd, s_ra);
                    break;
                }
            } else
            {
                if (rd == ra)
                {
                    printf("add%s %02s, %02s\n", s_z, s_ra, s_rb);
                } else
                {
                    printf("add%s %02s, %02s, %02s\n", s_z, s_rd, s_ra, s_rb);
                }
            }
            break;
        case 0b1001:    // sub
            if (rd == 0)
            {
                printf("cmp%s %02s, %02s\n", s_z, s_ra, s_rb);
            } else
            {
                if (rd == ra)
                {
                    printf("sub%s %02s, %02s\n", s_z, s_ra, s_rb);
                } else
                {
                    printf("sub%s %02s, %02s, %02s\n", s_z, s_rd, s_ra, s_rb);
                }
            }
            break;
        case 0b1010:    // and
            if (rd == ra)
            {
                printf("and%s %02s, %02s\n", s_z, s_ra, s_rb);
            } else
            {
                printf("and%s %02s, %02s, %02s\n", s_z, s_rd, s_ra, s_rb);
            }
            break;
        case 0b1011:    // orr
            if (rd == ra)
            {
                printf("orr%s %02s, %02s\n", s_z, s_ra, s_rb);
            } else
            {
                printf("orr%s %02s, %02s, %02s\n", s_z, s_rd, s_ra, s_rb);
            }
            break;
        default:
            printf(".word %i\n", inst);
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
                    disasm(inst_t, op_t, z_t, rd_t, ra_t, rb_t, imm8_t);
                }
                    printf("-------------------------------------------\n");
                printf("Showing memory from addresses %i to %i\n", start, end);

                break;

            case 'c':  // Continue until next breakpoint
                printf("Continuing until next breakpoint\n");

                // FIXME: Breakpoints (15%) (part 1/2)
                bp_pop();
                int count = 0;
                while (registers[PC] != top)
                {
                    exec_inst();
                    if (++count == ADDRSPACE)
                    {
                        printf("Error: Stack overflow\n");
                        break;
                    }
                    
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

                // FIXME: Breakpoints (15%) (part 2/2)
                printf("Toggling breakpoint at address %i\n", bp_addr);
                bp_add_del(bp_addr);
                printrecords();
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
