#ifndef H_I_TYPE
#define H_I_TYPE

void decode_i(u32 instr);

void exec_beq(u32 input_1, u32 input_2);
void exec_bne(u32 input_1, u32 input_2);
void exec_addi(u32 input_1, u32 input_2);
void exec_addiu(u32 input_1, u32 input_2);
void exec_slti(u32 input_1, u32 input_2);
void exec_andi(u32 input_1, u32 input_2);
void exec_ori(u32 input_1, u32 input_2);
void exec_nori(u32 input_1, u32 input_2);
void exec_lui(u32 input_1, u32 input_2);
void exec_load(u32 input_1, u32 input_2);
void exec_sb(u32 input_1, u32 input_2);
void exec_sh(u32 input_1, u32 input_2);
void exec_sw(u32 input_1, u32 input_2);
void mem_lb(void);
void mem_lh(void);
void mem_lw(void);
void mem_lbu(void);
void mem_lhu(void);

#endif
