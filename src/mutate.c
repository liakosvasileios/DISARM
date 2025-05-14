#include "mutate.h"
#include "mba.h"

/*
    Supported instrutions:
        mov reg, 0 => xor reg, reg
        mov r/m64, r64 => mov r64, r/m64
        add reg, imm64 => sub reg, -imm64
        sub reg, imm64 => add reg, -imm64
        xor reg, reg => mov reg, 0
        push imm32 => sub rsp, 8 ; mov [rsp], imm32
        xchg reg, reg2 => xor reg, reg2 ; xor reg2, reg ; xor reg, reg2
        **new**
*/

void mutate_opcode(struct Instruction *inst) {

    // mov reg, 0 => xor reg, reg
    if (inst->opcode == 0xB8 && inst->imm == 0 && CHANCE(PERC)) {
        inst->opcode = 0x31;    // xor reg, reg
        inst->operand_type = OPERAND_REG | OPERAND_REG;
        inst->op2 = inst->op1;
        inst->rex |= 0x08;      // ensure 64-bit mode
    }

    // Mutate mov r/m64, r64 -> mov r64, r/m64
    else if (inst->opcode == 0x89 && inst->operand_type == (OPERAND_MEM | OPERAND_REG)) {
        if (CHANCE(PERC)) {
            inst->opcode = 0x8B;    // mov r64, r/m64
            inst->operand_type = OPERAND_REG | OPERAND_MEM;
            
            // Swap operands
            uint8_t tmp = inst->op1;
            inst->op1 = inst->op2;
            inst->op2 = tmp;
            
            // Handle REX bits for swapped operands
            if (inst->rex) {
                uint8_t rex_r = (inst->rex & 0x04); // Extract REX.R
                uint8_t rex_b = (inst->rex & 0x01); // Extract REX.B
                
                // Swap REX.R and REX.B bits
                inst->rex = (inst->rex & 0xFA) | (rex_b << 2) | (rex_r >> 2);
            }
        }
    }

    // ADD RAX, imm32 -> SUB RAX, -imm32
    else if (inst->opcode == 0x05 &&
            inst->operand_type == (OPERAND_REG | OPERAND_IMM) &&
            inst->op1 == RAX_REG &&
            CHANCE(PERC)) {
        inst->opcode = 0x2D;  // SUB RAX, imm32
        inst->imm =  ~inst->imm + 1;  // Proper 2's complement negation
    }

    // SUB RAX, imm32 -> ADD RAX, -imm32
    else if (inst->opcode == 0x2D &&
            inst->operand_type == (OPERAND_REG | OPERAND_IMM) &&
            inst->op1 == RAX_REG &&
            CHANCE(PERC)) {
        inst->opcode = 0x05;  // ADD RAX, imm32
        inst->imm = ~inst->imm + 1;  // Proper 2's complement negation
    }
    
    // xor reg, reg -> mov reg, 0
    else if (inst->opcode == 0x31 && inst->operand_type == (OPERAND_REG | OPERAND_REG) && CHANCE(PERC)) {
        inst->opcode = 0xB8;    // mov reg, imm32
        inst->operand_type = OPERAND_REG | OPERAND_IMM;
        inst->imm = 0;
    }

    // push imm32 -> sub rsp, 8 ; mov [rsp], imm32
    

    // xchg reg, reg -> xor swap trick
   

    // Optional: operand flip (only if both are regs)
    if ((inst->operand_type == (OPERAND_REG | OPERAND_REG)) && CHANCE(PERC)) {
        uint8_t tmp = inst->op1;
        inst->op1 = inst->op2;
        inst->op2 = tmp;
        
        // Swap REX.R and REX.B bits if both are extended registers
        if (inst->rex && (inst->op1 >= 8 || inst->op2 >= 8)) {
            uint8_t rex_r = (inst->rex & 0x04); // Extract REX.R
            uint8_t rex_b = (inst->rex & 0x01); // Extract REX.B
            
            // Swap REX.R and REX.B bits
            inst->rex = (inst->rex & 0xFA) | (rex_b << 2) | (rex_r >> 2);
        }
    }
}

int mutate_multi(const struct Instruction *input, struct Instruction *out_list, int max_count) {
    if (max_count < 3) return 0;    // We may need up to 3 slots

    // push imm32 => sub rsp, 8; mov [rsp], imm32
    if (input->opcode == 0x68 && input->operand_type == OPERAND_IMM && CHANCE(PERC)) {
        // sub rsp, 8
        struct Instruction sub_inst;
        memset(&sub_inst, 0, sizeof(sub_inst));
        sub_inst.opcode = 0x2D;     // sub reg, imm32
        sub_inst.operand_type = OPERAND_REG | OPERAND_IMM;
        sub_inst.op1 = RSP_REG;     // rsp
        sub_inst.imm = 8;           // 8 bytes
        sub_inst.rex = 0x48;

        // mov [rsp], imm32
        struct Instruction mov_inst;
        memset(&mov_inst, 0, sizeof(mov_inst));
        mov_inst.opcode = 0xC7;         // IS mov [reg], imm32 HANDLED BY ENCODE/DECODE????? IF NOT ADD THIS BEFORE YOU RUN
        mov_inst.operand_type = OPERAND_MEM | OPERAND_IMM;
        mov_inst.op1 = RSP_REG;         // rsp
        mov_inst.imm = input->imm;   
        mov_inst.rex = 0x48;

        out_list[0] = sub_inst;
        out_list[1] = mov_inst;
        return 2;
    }

    // xchg reg, reg -> xor swap trick
    if (input->opcode == 0x87 && input->operand_type == (OPERAND_REG | OPERAND_REG) && CHANCE(PERC)) {
        struct Instruction xor1, xor2, xor3;
        memset(&xor1, 0, sizeof(xor1));
        memset(&xor2, 0, sizeof(xor2));
        memset(&xor3, 0, sizeof(xor3));

        xor1.opcode = xor2.opcode = xor3.opcode = 0x31;     // xor reg, reg 
        xor1.operand_type = xor2.operand_type = xor3.operand_type = OPERAND_REG | OPERAND_REG;

        xor1.op1 = input->op1;
        xor1.op2 = input->op2;

        xor2.op1 = input->op2;
        xor2.op2 = input->op1;
        
        xor3.op1 = input->op1;
        xor3.op2 = input->op2;

        xor1.rex = xor2.rex = xor3.rex = 0x48;

        out_list[0] = xor1;
        out_list[1] = xor2;
        out_list[2] = xor3;

        return 3;

    }

    // add rax, imm32 => xor decomposition (MBA): mov rcx, imm^mask; xor rcx, mask; add rax, rcx
    if (input->opcode == 0x05 && input->operand_type == (OPERAND_REG | OPERAND_IMM) && input->op1 == RAX_REG && CHANCE(PERC)) {
        if (max_count < 3) return 0;  // ensure space

        uint32_t mask = rand();
        uint32_t encoded = input->imm ^ mask;

        struct Instruction mov_temp = {0};
        struct Instruction xor_temp = {0};
        struct Instruction add_target = {0};

        // mov rcx, encoded
        mov_temp.opcode = 0xB8;
        mov_temp.operand_type = OPERAND_REG | OPERAND_IMM;
        mov_temp.op1 = RCX_REG;
        mov_temp.imm = encoded;
        mov_temp.rex = 0x48;

        // xor rcx, mask
        xor_temp.opcode = 0x81;
        xor_temp.operand_type = OPERAND_REG | OPERAND_IMM;
        xor_temp.op1 = RCX_REG;
        xor_temp.imm = mask;
        xor_temp.rex = 0x48;

        // add rax, rcx
        add_target.opcode = 0x01;
        add_target.operand_type = OPERAND_REG | OPERAND_REG;
        add_target.op1 = RAX_REG;
        add_target.op2 = RCX_REG;
        add_target.rex = 0x48;

        out_list[0] = mov_temp;
        out_list[1] = xor_temp;
        out_list[2] = add_target;

        return 3;
    }

    // sub rax, imm32 => MBA: mov rcx, imm^mask; xor rcx, mask; sub rax, rcx
    if (input->opcode == 0x2D &&
        input->operand_type == (OPERAND_REG | OPERAND_IMM) &&
        input->op1 == RAX_REG &&
        CHANCE(PERC)) {

        if (max_count < 3) return 0;

        uint32_t mask = rand();
        uint32_t encoded = input->imm ^ mask;

        struct Instruction mov_temp = {0};
        struct Instruction xor_temp = {0};
        struct Instruction sub_target = {0};

        // mov rcx, encoded
        mov_temp.opcode = 0xB8;
        mov_temp.operand_type = OPERAND_REG | OPERAND_IMM;
        mov_temp.op1 = RCX_REG;
        mov_temp.imm = encoded;
        mov_temp.rex = 0x48;

        // xor rcx, mask
        xor_temp.opcode = 0x81;
        xor_temp.operand_type = OPERAND_REG | OPERAND_IMM;
        xor_temp.op1 = RCX_REG;
        xor_temp.imm = mask;
        xor_temp.rex = 0x48;

        // sub rax, rcx
        sub_target.opcode = 0x29;
        sub_target.operand_type = OPERAND_REG | OPERAND_REG;
        sub_target.op1 = RAX_REG;
        sub_target.op2 = RCX_REG;
        sub_target.rex = 0x48;

        out_list[0] = mov_temp;
        out_list[1] = xor_temp;
        out_list[2] = sub_target;

        return 3;
    }

    // mov reg, imm32 => MBA: mov reg, imm^mask; xor reg, mask
    if (input->opcode == 0xB8 &&
        input->operand_type == (OPERAND_REG | OPERAND_IMM) &&
        CHANCE(PERC)) {

        if (max_count < 2) return 0;

        uint32_t mask = rand();
        uint32_t encoded = input->imm ^ mask;

        struct Instruction mov_inst = {0};
        struct Instruction xor_inst = {0};

        // mov reg, encoded
        mov_inst.opcode = 0xB8;
        mov_inst.operand_type = OPERAND_REG | OPERAND_IMM;
        mov_inst.op1 = input->op1;
        mov_inst.imm = encoded;
        mov_inst.rex = 0x48;

        // xor reg, mask
        xor_inst.opcode = 0x81;
        xor_inst.operand_type = OPERAND_REG | OPERAND_IMM;
        xor_inst.op1 = input->op1;
        xor_inst.imm = mask;
        xor_inst.rex = 0x48;

        out_list[0] = mov_inst;
        out_list[1] = xor_inst;

        return 2;
    }

        // Jcc near (0F 8x) => SET!cc + TEST + JNZ + JMP
    if ((input->opcode & 0xFF00) == 0x0F00 && input->operand_type == OPERAND_IMM && CHANCE(PERC)) {
        if (max_count < 4) return 0;

        uint8_t jcc = input->opcode & 0xFF;
        uint8_t inverse_jcc = jcc ^ 0x01;  // flip lowest bit to get inverse (e.g. 0x84 <-> 0x85)
        uint8_t setcc_opcode = 0x0F00 | inverse_jcc;

        struct Instruction setcc = {0};
        setcc.opcode = setcc_opcode;
        setcc.operand_type = OPERAND_REG;
        setcc.op1 = AL_REG;

        struct Instruction test = {0};
        test.opcode = 0x84;  // TEST r/m8, r8
        test.operand_type = OPERAND_REG | OPERAND_REG;
        test.op1 = AL_REG;
        test.op2 = AL_REG;

        struct Instruction jnz = {0};
        jnz.opcode = 0x0F85;  // JNZ (a.k.a. JNE)
        jnz.operand_type = OPERAND_IMM;
        jnz.imm = 5;  // size of JMP below

        struct Instruction jmp = {0};
        jmp.opcode = 0xE9;  // JMP rel32
        jmp.operand_type = OPERAND_IMM;
        jmp.imm = input->imm;

        out_list[0] = setcc;
        out_list[1] = test;
        out_list[2] = jnz;
        out_list[3] = jmp;

        return 4;
    }


    // Unsupported/Invalid Instruction
    return 0;   
}