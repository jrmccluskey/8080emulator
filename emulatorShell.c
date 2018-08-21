/* Code to emulate 8080 processor assembly code
  Jack R. McCluskey
  7-31-2018
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include"disassembler.c"

/* Struct emulating the flags of the 8080 processor
  All flags default to 1
*/
typedef struct conditionCodes {
  uint8_t z:1; // Zero Flag
  uint8_t s:1; // Sign flag
  uint8_t p:1; // Parity flag
  uint8_t cy:1; // Carryout flag
  uint8_t ac:1; // Auxillary carry
  uint8_t pad:1;
} conditionCodes;

/* Struct emulating the state of the 8080 processor
  Features registers A-L, the stack pointer, program counter,
  the memory, condition codes, etc.
*/
typedef struct state8080 {
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t sp; // Stack Pointer
  uint16_t pc; // Program Counter
  uint8_t *memory;
  struct conditionCodes cc;
  uint8_t intEnable;
} state8080;

/* Exception for unimplimented instructions
  Input: a state8080 struct
  Output: void
  Exits the program with code 1
*/
void unimplementedInstruction(state8080* state) {
    printf("ERROR: Unimplemented instruction\n");
    exit(1);
}

/* Helper function for calculating parity of an 8-bit input
  Input: an unsigned 8-bit int
  Output: 0 if even parity, 1 if odd parity
*/
int parity(uint8_t input) {
  int count = 0;
  int i,b = 1;
  for(i = 0; i < 8; i++) {
    if(input & (b << i)) {
      count++;
    }
  }
  return count % 2;
}

// Array of cycle counts for each operation
unsigned char cycles[] = {
  4, 10, 7, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
  4, 10, 7, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
  4, 10, 16, 5, 5, 5, 7, 4, 4, 10, 16, 5, 5, 5, 7, 4,
  4, 10, 13, 5, 10, 10, 10, 4, 4, 10, 13, 5, 5, 5, 7, 4,
  5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
  5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
  5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
  7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,
  4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
  4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
  4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
  4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
  11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11,
  11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11,
  11, 10, 10, 18, 17, 11, 7, 11, 11, 5, 10, 5, 17, 17, 7, 11,
  11, 10, 10, 4, 17, 11, 7, 11, 11, 5, 10, 4, 17, 17, 7, 11
};

/* Function to control writes to state memory
  Input: state8080 struct, value to write, address to write to
  Output: void
  Does not write if the address is in the first 2k of memory
  Fails if address is out of memory range
*/
void writeToMemory(state8080* state, uint8_t value, uint8_t topBits, uint8_t botBits) {
  uint16_t address = (topBits << 8) | botBits;
  if(address <= 2000) {
    printf("This is ROM. Can't write here.");
  } else if(address >= 0x10000) {
    printf("Address out of bounds.");
    exit(1);
  } else {
    state->memory[address] = value;
  }
}

/*  Function to facilitate reads from memory
  Input: state8080 struct, address to read from memory
  Output: uint8_t value
  Fails if address is out of memory range
*/
uint8_t readFromMemory(state8080* state, uint8_t topBits, uint8_t botBits) {
  uint16_t address = (topBits << 8) | botBits;
  if(address >= 0x10000) {
    printf("Address out of bounds.");
    exit(1);
  } else {
    return state->memory[address];
  }
}

/* Code implementation of the 8080 op codes
  Input: state8080 Struct
  Output: void
  Changes fields in state8080 struct
  TODO: Debug and refactor with helper functions
*/
int emulateOp(state8080* state) {
  unsigned char *opCode = &state->memory[state->pc];
  switch(*opCode) {
    case 0x00:
    case 0x08:
    case 0x10:
    case 0x18:
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38:
    case 0xcb:
    case 0xd9:
    case 0xdd:
    case 0xed:
    case 0xfd:
      break; // NOP

    // Register Manipulation
    case 0x01:
      state->c = opCode[1];
      state->b = opCode[2];
      state->pc +=2;
      break; // LXI B,D1

    // TODO: Review STAX B in data book
    case 0x02:
      writeToMemory(state, state->a, state->b, state->c);
      break; // STAX B

    case 0x03: {
      // Put BC into one value
      uint16_t combined = (state->b << 8) | state->c;
      // Increment by 1
      combined += 1;
      // Replace c with new value, masking top bits
      state->c = combined & 0xff;
      // Replace b with the shifted value
      state->b = (combined >> 8) & 0xff;
      break; // INX B
    }

    case 0x04: {
      uint16_t val = (uint16_t) state->b + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->b = val & 0xff;
      break; // INR B
    }

    case 0x05: {
      uint16_t val = (uint16_t) state->b - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->b = val & 0xff;
      break; // DCR B
    }

    case 0x06:
      state->b = opCode[1];
      state->pc += 1;
      break; // MVI B,D8

    case 0x07: {
      uint8_t leftMost = (state->a >> 7) & 0x01;
      state->a = (state->a << 1) | leftMost;
      state->cc.cy = leftMost;
      break; // RLC
    }

    case 0x09: {
      uint16_t hl = (state->h << 8) | state->l;
      uint16_t bc = (state->b << 8) | state->c;
      uint32_t total = hl + bc;
      if(total > 0xffff) {
        state->cc.cy = 1;
      } else {
        state->cc.cy = 0;
      }
      state->h = (total >> 8) & 0xff;
      state->l = total & 0xff;
      break; // DAD B
    }

    case 0x0a:
      state->a = readFromMemory(state, state->b, state->c);
      break; // LDAX B

    case 0x0b: {
      // Put BC into one value
      uint16_t combined = (state->b << 8) | state->c;
      // Decrement by 1
      combined -= 1;
      // Replace c with new value, masking top bits
      state->c = combined & 0xff;
      // Replace b with the shifted value
      state->b = (combined >> 8) & 0xff;
      break; // DCX B
    }

    case 0x0c: {
      uint16_t val = (uint16_t) state->c + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->c = val & 0xff;
      break; // INR C
    }

    case 0x0d: {
      uint16_t val = (uint16_t) state->c - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->c = val & 0xff;;
      break; // DCR C
    }

    case 0x0e:
      state->c = opCode[1];
      state->pc += 1;
      break; // MVI C,D8

    case 0x0f: {
      uint8_t rightMost = state->a & 0x01;
      state->a = (state->a >> 1) | (rightMost << 7);
      state->cc.cy = rightMost;
      break; // RRC
    }

    case 0x11:
      state->e = opCode[1];
      state->d = opCode[2];
      state->pc += 2;
      break; // LXI D,D16

    case 0x12:
      writeToMemory(state, state->a, state->d, state->e);
      break; // STAX D

    case 0x13: {
      uint16_t combined = (state->d << 8) | state->e;
      combined += 1;
      state->e = combined & 0xff;
      state->d = (combined >> 8) & 0xff;
      break; // INX D
    }

    case 0x14: {
      uint16_t val = (uint16_t) state->d + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->d = val & 0xff;
      break; // INR D
    }

    case 0x15: {
      uint16_t val = (uint16_t) state->d - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->d = val & 0xff;
      break; // DCR D
    }

    case 0x16:
      state->d = opCode[1];
      state->pc += 1;
      break; // MVI D,D8

    case 0x17: {
      uint8_t leftMost = (state->a >> 7) & 0x01;
      state->a = (state->a << 1) | state->cc.cy;
      state->cc.cy = leftMost;
      break; // RAL
    }

    case 0x19: {
      uint16_t hl = (state->h << 8) | state->l;
      uint16_t de = (state->d << 8) | state->e;
      uint32_t total = hl + de;
      if(total > 0xffff) {
        state->cc.cy = 1;
      } else {
        state->cc.cy = 0;
      }
      state->h = (total >> 8) & 0xff;
      state->l = total & 0xff;
      break; // DAD D
    }

    case 0x1a:
      state->a = readFromMemory(state, state->d, state->e);
      break; // LDAX D

    case 0x1b: {
      uint16_t combined = (state->d << 8) | state->e;
      combined -= 1;
      state->e = combined & 0xff;
      state->d = (combined >> 8) & 0xff;
      break; // DCX D
    }

    case 0x1c: {
      uint16_t val = (uint16_t) state->e + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->e = val & 0xff;
      break; // ICR E
    }

    case 0x1d: {
      uint16_t val = (uint16_t) state->e - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->e = val & 0xff;
      break; // DCR E
    }

    case 0x1e:
      state->e = opCode[1];
      state->pc += 1;
      break; // MVI E,D8

    case 0x1f: {
      uint8_t rightMost = state->a & 0x01;
      uint8_t leftMost = state->a & 0x80;
      state->a = (state->a >> 1) | leftMost;
      state->cc.cy = rightMost;
      break; // RAR
    }

    case 0x21:
      state->l = opCode[1];
      state->h = opCode[2];
      state->pc += 2;
      break; // LXIH,D16

    case 0x22:
      state->memory[(opCode[2] << 8) | opCode[1]] = state->l;
      state->memory[((opCode[2] << 8) | opCode[1]) + 1] = state->h;
      state->pc += 2;
      break; // SHLD adr

    case 0x23: {
      uint16_t combined = (state->h << 8) | state->l;
      combined += 1;
      state->l = combined & 0xff;
      state->h = (combined >> 8) & 0xff;
      break; // INX H
    }

    case 0x24: {
      uint16_t val = (uint16_t) state->h + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->h = val & 0xff;
      break; // INR H
    }

    case 0x25: {
      uint16_t val = (uint16_t) state->h - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->h = val & 0xff;
      break; // DCR H
    }

    case 0x26:
      state->h = opCode[1];
      state->pc += 1;
      break; // MVI L,D8

    case 0x27: unimplementedInstruction(state); break; // DAA

    case 0x29: {
      uint16_t hl = (state->h << 8) | state->l;
      uint32_t total = hl + hl;
      if(total > 0xffff) {
        state->cc.cy = 1;
      } else {
        state->cc.cy = 0;
      }
      state->h = (total >> 8) & 0xff;
      state->l = total & 0xff;
      break; // DAD H
    }

    case 0x2a:
      state->l = state->memory[(opCode[2] << 8) | opCode[1]];
      state->h = state->memory[((opCode[2] << 8) | opCode[1]) + 1];
      state->pc += 2;
      break; // LHLD adr

    case 0x2b: {
      uint16_t combined = (state->h << 8) | state->l;
      combined -= 1;
      state->l = combined & 0xff;
      state->h = (combined >> 8) & 0xff;
      break; // DCX H
    }

    case 0x2c: {
      uint16_t val = (uint16_t) state->l + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->l = val & 0xff;
      break; // INR L
    }

    case 0x2d: {
      uint16_t val = (uint16_t) state->l - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->l = val & 0xff;
      break; // DCR L
    }

    case 0x2e:
      state->l = opCode[1];
      state->pc += 1;
      break; // MVI L,D8

    case 0x2f:
      state->a = ~(state->a);
      break; // CMA

    case 0x31:
      state->sp = (opCode[2] << 8) | opCode[1];
      state->pc += 2;
      break; // LXI SP,D16

    case 0x32:
      writeToMemory(state, state->a, opCode[2], opCode[1]);
      state->pc += 2;
      break; // STA adr

    case 0x33:
      state->sp += 1;
      break; // INX SP

    case 0x34: {
      uint16_t val = state->memory[(state->h << 8) | state->l] + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->memory[(state->h << 8) | state->l] += 1;
      break; // INR M
    }

    case 0x35: {
      uint16_t val = state->memory[(state->h << 8) | state->l] - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->memory[(state->h << 8) | state->l] -= 1;
      break; // DCR M
    }

    case 0x36:
      writeToMemory(state, opCode[1], state->h, state->l);
      state->pc += 1;
      break; // MVI M,D8

    case 0x37:
      state->cc.cy = 1;
      break; // STC

    case 0x39: {
      uint32_t val = ((state->h << 8) | state->l) + state->sp;
      if(val > 0xffff){
        state->cc.cy = 1;
      }
      state->h = (val >> 8) & 0xff;
      state->l = val & 0xff;
      break; // DAD SP
    }

    case 0x3a:
      state->a readFromMemory(state, opCode[2], opCode[1]);
      state->pc += 2;
      break; // LDA adr

    case 0x3b:
      state->sp -= 1;
      break; // DCX SP

    case 0x3c: {
      uint16_t val = (uint16_t) state->a + 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // INR A
    }

    case 0x3d: {
      uint16_t val = (uint16_t) state->a - 1;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // DCR A
    }

    case 0x3e:
      state->a = opCode[1];
      state->pc += 1;
      break; // MVI A,D8

    case 0x3f:
      state->cc.cy = ~(state->cc.cy);
      break; // CMC

    // Data Transfer Operations
    case 0x40: state->b = state->b; break; // MOV B,B

    case 0x41: state->b = state->c; break; // MOV B,C

    case 0x42: state->b = state->d; break; // MOV B,D

    case 0x43: state->b = state->e; break; // MOV B,E

    case 0x44: state->b = state->h; break; // MOV B,H

    case 0x45: state->b = state->l; break; // MOV B,L

    case 0x46:
      state->b = readFromMemory(state, state->h, state->l);
      break; // MOV B,M

    case 0x47: state->b = state->a; break; // MOV B,A

    case 0x48: state->c = state->b; break;

    case 0x49: state->c = state->c; break;

    case 0x4a: state->c = state->d; break;

    case 0x4b: state->c = state->e; break;

    case 0x4c: state->c = state->h; break;

    case 0x4d: state->c = state->l; break;

    case 0x4e:
      state->c = readFromMemory(state, state->h, state->l);
      break; // MOV C,M

    case 0x4f: state->c = state->a; break;

    case 0x50: state->d = state->b; break;

    case 0x51: state->d = state->c; break;

    case 0x52: state->d = state->d; break;

    case 0x53: state->d = state->e; break;

    case 0x54: state->d = state->h; break;

    case 0x55: state->d = state->l; break;

    case 0x56:
      state->d = readFromMemory(state, state->h, state->l);
      break; // MOV D,M

    case 0x57: state->d = state->a; break;

    case 0x58: state->e = state->b; break;

    case 0x59: state->e = state->c; break;

    case 0x5a: state->e = state->d; break;

    case 0x5b: state->e = state->e; break;

    case 0x5c: state->e = state->h; break;

    case 0x5d: state->e = state->l; break;

    case 0x5e:
      state->e = readFromMemory(state, state->h, state->l);
      break; // MOV E,M

    case 0x5f: state->e = state->a; break;

    case 0x60: state->h = state->b; break;

    case 0x61: state->h = state->c; break;

    case 0x62: state->h = state->d; break;

    case 0x63: state->h = state->e; break;

    case 0x64: state->h = state->h; break;

    case 0x65: state->h = state->l; break;

    case 0x66:
      state->h = readFromMemory(state, state->h, state->l);
      break; // MOV H,M

    case 0x67: state->h = state->a; break;

    case 0x68: state->l = state->b; break;

    case 0x69: state->l = state->c; break;

    case 0x6a: state->l = state->d; break;

    case 0x6b: state->l = state->e; break;

    case 0x6c: state->l = state->h; break;

    case 0x6d: state->l = state->l; break;

    case 0x6e:
      state->l = readFromMemory(state, state->h, state->l);
      break; // MOV L,M

    case 0x6f: state->l = state->a; break;

    case 0x70:
      writeToMemory(state, state->b, state->h, state->l);
      break; // MOV M,B

    case 0x71:
      writeToMemory(state, state->c, state->h, state->l);
      break; // MOV M,C

    case 0x72:
      writeToMemory(state, state->d, state->h, state->l);
      break; // MOV M,D

    case 0x73:
      writeToMemory(state, state->e, state->h, state->l);
      break; // MOV M,E

    case 0x74:
      writeToMemory(state, state->h, state->h, state->l);
      break; // MOV M,H

    case 0x75:
      writeToMemory(state, state->l, state->h, state->l);
      break; // MOV M,L

    case 0x76: exit(0); break; // HLT

    case 0x77:
      writeToMemory(state, state->a, state->h, state->l);
      break; // MOV M,A

    case 0x78: state->a = state->b; break;

    case 0x79: state->a = state->c; break;

    case 0x7a: state->a = state->d; break;

    case 0x7b: state->a = state->e; break;

    case 0x7c: state->a = state->h; break;

    case 0x7d: state->a = state->l; break;

    case 0x7e:
      state->a = readFromMemory(state, state->h, state->l);
      break; // MOV A,M

    case 0x7f: state->a = state->a; break;

    // Arithmatic
    case 0x80: {
      uint16_t val = state->a + state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD B
    }

    case 0x81: {
      uint16_t val = state->a + state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD C
    }

    case 0x82: {
      uint16_t val = state->a + state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD D
    }

    case 0x83: {
      uint16_t val = state->a + state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD E
    }

    case 0x84: {
      uint16_t val = state->a + state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD B
    }

    case 0x85: {
      uint16_t val = state->a + state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD L
    }

    case 0x86: {
      uint16_t val = state->a + readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD M
    }

    case 0x87: {
      uint16_t val = state->a + state->a;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADD A
    }

    case 0x88: {
      uint16_t val = state->a + state->b + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC B
    }

    case 0x89: {
      uint16_t val = state->a + state->c + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC C
    }

    case 0x8a: {
      uint16_t val = state->a + state->d + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC D
    }

    case 0x8b: {
      uint16_t val = state->a + state->e + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC E
    }
    case 0x8c: {
      uint16_t val = state->a + state->h + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC H
    }

    case 0x8d: {
      uint16_t val = state->a + state->l + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC L
    }

    case 0x8e: {
      uint16_t val = state->a + readFromMemory(state, state->h, state->l) + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC M
    }

    case 0x8f: {
      uint16_t val = state->a + state->a + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ADC A
    }

    case 0x90: {
      uint16_t val = state->a - state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB B
    }

    case 0x91: {
      uint16_t val = state->a - state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB C
    }

    case 0x92: {
      uint16_t val = state->a - state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB D
    }

    case 0x93: {
      uint16_t val = state->a - state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB E
    }

    case 0x94: {
      uint16_t val = state->a - state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB H
    }

    case 0x95: {
      uint16_t val = state->a - state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB L
    }

    case 0x96: {
      uint16_t val = state->a - readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB M
    }

    case 0x97: {
      uint16_t val = state->a - state->a;
      state->cc.z = 1;
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SUB A
    }

    case 0x98: {
      uint16_t val = state->a - state->b - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB B
    }

    case 0x99: {
      uint16_t val = state->a - state->c - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBCBC
    }

    case 0x9a: {
      uint16_t val = state->a - state->d - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB D
    }

    case 0x9b: {
      uint16_t val = state->a - state->e - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB E
    }

    case 0x9c: {
      uint16_t val = state->a - state->h - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB H
    }

    case 0x9d: {
      uint16_t val = state->a - state->l - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB L
    }

    case 0x9e: {
      uint16_t val = state->a - readFromMemory(state, state->h, state->l) - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB M
    }

    case 0x9f: {
      uint16_t val = state->a - state->a - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // SBB A
    }

    // Logic
    case 0xa0: {
      uint16_t val = state->a & state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND B
    }

    case 0xa1: {
      uint16_t val = state->a & state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND C
    }

    case 0xa2: {
      uint16_t val = state->a & state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND D
    }

    case 0xa3: {
      uint16_t val = state->a & state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND E
    }

    case 0xa4: {
      uint16_t val = state->a & state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND H
    }

    case 0xa5: {
      uint16_t val = state->a & state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND L
    }

    case 0xa6: {
      uint16_t val = state->a & readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND M
    }

    case 0xa7: {
      uint16_t val = state->a & state->a;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // AND A
    }

    case 0xa8: {
      uint16_t val = state->a ^ state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA B
    }

    case 0xa9: {
      uint16_t val = state->a ^ state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA C
    }

    case 0xaa: {
      uint16_t val = state->a ^ state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA D
    }

    case 0xab: {
      uint16_t val = state->a ^ state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA E
    }

    case 0xac: {
      uint16_t val = state->a ^ state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA H
    }

    case 0xad:{
      uint16_t val = state->a ^ state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA L
    }

    case 0xae: {
      uint16_t val = state->a ^ readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA M
    }

    case 0xaf: {
      uint16_t val = state->a ^ state->a;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // XRA A
    }

    case 0xb0: {
      uint16_t val = state->a | state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA B
    }

    case 0xb1: {
      uint16_t val = state->a | state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA C
    }

    case 0xb2: {
      uint16_t val = state->a | state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA D
    }

    case 0xb3: {
      uint16_t val = state->a | state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA E
    }

    case 0xb4: {
      uint16_t val = state->a | state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA H
    }

    case 0xb5: {
      uint16_t val = state->a | state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA L
    }

    case 0xb6: {
      uint16_t val = state->a | readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA M
    }

    case 0xb7: {
      uint16_t val = state->a | state->a;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      break; // ORA A
    }

    case 0xb8: {
      uint16_t val = state->a - state->b;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP B
    }

    case 0xb9: {
      uint16_t val = state->a - state->c;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP C
    }

    case 0xba: {
      uint16_t val = state->a - state->d;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP D
    }

    case 0xbb: {
      uint16_t val = state->a - state->e;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP E
    }

    case 0xbc: {
      uint16_t val = state->a - state->h;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP H
    }

    case 0xbd: {
      uint16_t val = state->a - state->l;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP L
    }

    case 0xbe: {
      uint16_t val = state->a - readFromMemory(state, state->h, state->l);
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP M
    }

    case 0xbf: {
      uint16_t val = state->a - state->a;
      state->cc.z = 1;
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      break; // CMP A
    }

    // Branches and Stack Management
    case 0xc0:
      if(state->cc.z == 0) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RNZ

    case 0xc1:
      state->c = state->memory[state->sp];
      state->b = state->memory[state->sp+1];
      state->sp += 2;
      break; // POP B

    case 0xc2:
      if(state->cc.z == 0) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JNZ adr

    case 0xc3:
      state->pc = (opCode[2] << 8) | opCode[1];
      state->pc += 2;
      break; // JMP adr

    case 0xc4:
      if(state->cc.z == 0) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CNZ

    case 0xc5:
      state->memory[state->sp-1] = state->b;
      state->memory[state->sp-2] = state->c;
      state->sp -= 2;
      break; // PUSH B

    case 0xc6: {
      uint16_t val = state->a + opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // ADI D8
    }

    case 0xc7:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x00;
      break; // RST 0

    case 0xc8:
      if(state->cc.z == 1) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RZ

    case 0xc9:
      state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
      state->sp += 2;
      break; // RET

    case 0xca:
      if(state->cc.z == 1) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JZ adr

    case 0xcc:
      if(state->cc.z == 1) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CZ adr

    case 0xcd:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = (opCode[2] << 8) | opCode[1];
      break; // CALL adr

    case 0xce: {
      uint16_t val = state->a + opCode[1] + state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // ACI D8
    }

    case 0xcf:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x08;
      break; // RST 1

    case 0xd0:
      if(state->cc.cy == 0) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; //RNC

    case 0xd1:
      state->e = state->memory[state->sp];
      state->d = state->memory[state->sp+1];
      state->sp += 2;
      break; // POP D

    case 0xd2:
      if(state->cc.cy == 0) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JNC

    case 0xd3: /*Skip for now */ break; // OUT D8

    case 0xd4:
      if(state->cc.cy == 0) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CNC

    case 0xd5:
      state->memory[state->sp-1] = state->d;
      state->memory[state->sp-2] = state->e;
      state->sp -= 2;
      break; // PUSH D

    case 0xd6: {
      uint16_t val = state->a - opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // SUI D8
    }

    case 0xd7:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x10;
      break; // RST 2

    case 0xd8:
      if(state->cc.cy == 1) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RC

    case 0xda:
      if(state->cc.cy == 1) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JC adr

    case 0xdb: /*Skip for now */  break; // IN D8

    case 0xdc:
      if(state->cc.cy == 1) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CC

    case 0xde: {
      uint16_t val = state->a - opCode[1] - state->cc.cy;
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // SUI D8
    }

    case 0xdf:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x18;
      break; // RST 3

    case 0xe0:
      if(state->cc.p == 1) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RPO

    case 0xe1:
      state->l = state->memory[state->sp];
      state->h = state->memory[state->sp+1];
      state->sp += 2;
      break; // POP H

    case 0xe2:
      if(state->cc.p == 1) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JPO adr

    case 0xe3: {
      uint8_t storage = state->memory[state->sp];
      state->memory[state->sp] = state->l;
      state->l = storage;
      storage = state->memory[state->sp+1];
      state->memory[state->sp+1] = state->h;
      break; // XTHL
    }

    case 0xe4:
      if(state->cc.p == 1) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CPO adr

    case 0xe5:
      state->memory[state->sp-1] = state->h;
      state->memory[state->sp-2] = state->l;
      state->sp -= 2;
      break; // PUSH H

    case 0xe6: {
      uint16_t val = state->a & opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // ANI D8
    }

    case 0xe7:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x20;
      break; // RST 4

    case 0xe8:
      if(state->cc.p == 0)
      {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RPE

    case 0xe9:
      state->pc = (state->h << 8) | state->l;
      break; // PCHL

    case 0xea:
      if(state->cc.p == 0) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JPE adr

    case 0xeb: {
      uint8_t storage = state->h;
      state->h = state->d;
      state->d = storage;
      storage = state->l;
      state->l = state->e;
      state->e = storage;
      break; // XCHG
    }

    case 0xec:
      if(state->cc.p == 0) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CPE adr

    case 0xee:  {
      uint16_t val = state->a ^ opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // XRI D8
    }

    case 0xef:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x28;
      break; // RST 5

    case 0xf0:
      if(state->cc.cy == 0) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RP

    case 0xf1: {
      state->a = state->memory[state->sp + 1];
      uint8_t psw = state->memory[state->sp];
      state->cc.z = psw & 0xfe;
      state->cc.s = psw & 0xfd;
      state->cc.p = psw & 0xfb;
      state->cc.cy = psw & 0xf7;
      state->cc.ac = psw & 0x2f;
      state->cc.pad = psw & 0x1f;
      break; // POP PSW
    }

    case 0xf2:
      if(state->cc.p == 0) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JP adr

    case 0xf3: state->intEnable = 0; break; // DI

    case 0xf4:
      if(state->cc.p == 0) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CP adr

    case 0xf5:
      state->memory[state->sp - 1] = state->a;
      state->memory[state->sp - 2] = (state->cc.z | (state->cc.s << 1) | (state->cc.p << 2) |
        (state->cc.cy << 3) | (state->cc.ac << 4) | (state->cc.pad << 5));
      state->sp -=2;
      break; // PUSH PSW

    case 0xf6: {
      uint16_t val = state->a | opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->a = val & 0xff;
      state->pc += 1;
      break; // ORI D8
    }

    case 0xf7:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x30;
      break; // RST 6

    case 0xf8:
      if(state->cc.s == 1) {
        state->pc = (state->memory[(state->sp) + 1] << 8 ) | (state->memory[state->sp]);
        state->sp += 2;
      }
      break; // RM

    case 0xf9:
      state->sp = (state->h << 8) | state->l;
      break;// SPHL

    case 0xfa:
      if(state->cc.s == 1) {
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // JM adr

    case 0xfb: state->intEnable = 1; break; // EI

    case 0xfc:
      if(state->cc.s == 1) {
        state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
        state->memory[state->pc - 2] = state->pc & 0xff;
        state->sp += 2;
        state->pc = (opCode[2] << 8) | opCode[1];
      } else {
        state->pc += 2;
      }
      break; // CM adr

    case 0xfe: {
      uint16_t val = state->a - opCode[1];
      state->cc.z = ((val & 0xff) == 0);
      state->cc.s = ((val & 0x80) != 0);
      state->cc.cy = (val > 0xff);
      state->cc.p = parity(val & 0xff);
      state->pc += 1;
      break; // CPI D8
    }

    case 0xff:
      state->memory[state->pc - 1] = (state->pc >> 8) & 0xff;
      state->memory[state->pc - 2] = state->pc & 0xff;
      state->sp += 2;
      state->pc = 0x38;
      break; // RST 7
  }
  // Increment program counter
  state->pc+=1;
  // Return number of cucles for op
  return cycles[*opCode];
}

/* Code to facilitate interrupts in code
  Input: state8080 struct, int
  Output: void
  Pushes program counter onto Stack
  Changes program counter
*/
void generateInterrupt(state8080* state, int number) {
  state->memory[state->sp-2] = state->pc & 0xff;
  state->memory[state->sp-1] = (state->pc >> 8) & 0xff;
  state->pc = 8 * number;
}

/* Code to read files into state memory
  Input: state8080 struct, filename, 32 bit memory location
  Output: Void
*/
void readFileIntoMemory(state8080* state, char* fileName, uint32_t location) {
  // Get pointer to file/open file
  FILE *f = fopen(fileName, "rb");
  // Check for null
  if(f == NULL) {
    printf("ERROR: Cannot open %s\n", fileName);
    exit(1);
  } else {
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // Allocate memory for opCodes
    uint8_t *buffer = &state->memory[location];

    // Put file into buffer, close original file
    fread(buffer, fsize, 1, f);
    fclose(f);
  }
}

/* Function to creat 8080 state
  Input: void
  Output: new state8080 struct w/ 16k=K bits of memory
*/
state8080* initializeState() {
  state8080* state = calloc(1, sizeof(state8080));
  state->memory = malloc(0x10000);
  return state;
}

/* Main function for 8080 emulator
  Loads space invaders into memory of state
  Runs emulator operations
*/
int main(int argc, char const *argv[]) {
  int finished = 0;
  state8080* state = initializeState();
  readFileIntoMemory(state, "invaders.h", 0x100);
  readFileIntoMemory(state, "invaders.g", 0x900);
  readFileIntoMemory(state, "invaders.f", 0x1100);
  readFileIntoMemory(state, "invaders.e", 0x1900);

  while(finished == 0) {
    emulateOp(state);
  }
}
