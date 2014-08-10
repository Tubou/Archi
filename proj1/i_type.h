#ifndef H_I_TYPE
#define H_I_TYPE

struct op_i {
	u32 rs;
	u32 rt;
	u32 C;
	signed int sign_C;
};



void simu_beq(u32 instr);
void simu_bne(u32 instr);
void simu_addi(u32 instr);
void simu_addiu(u32 instr);
void simu_slti(u32 instr);
void simu_andi(u32 instr);
void simu_ori(u32 instr);
void simu_nori(u32 instr);
void simu_lui(u32 instr);
void simu_lb(u32 instr);
void simu_lh(u32 instr);
void simu_lw(u32 instr);
void simu_lbu(u32 instr);
void simu_lhu(u32 instr);
void simu_sb(u32 instr);
void simu_sh(u32 instr);
void simu_sw(u32 instr);



#endif
