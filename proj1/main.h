#ifndef H_MAIN
#define H_MAIN

typedef unsigned int u32;

extern u32 i_memory[0x100];
extern u32 d_memory[0x100];
extern u32 instr_max;
extern u32 data_max;

struct VCPU {
	u32 PC;
	u32 reg[32];
	u32 err_flag;
	u32 halt_flag;
};

extern struct VCPU vcpu;

extern int cycle;


#define R_TYPE 0x00
#define OP_J 0x02
#define OP_JAL 0x03
#define OP_BEQ 0x04
#define OP_BNE 0x05
#define OP_ADDI 0x08
#define OP_ADDIU 0x09
#define OP_SLTI 0x0A
#define OP_ANDI 0x0C
#define OP_ORI 0x0D
#define OP_NORI 0x0E
#define OP_LUI 0x0F
#define OP_LB 0x20
#define OP_LH 0x21
#define OP_LW 0x23
#define OP_LBU 0x24
#define OP_LHU 0x25
#define OP_SB 0x28
#define OP_SH 0x29
#define OP_SW 0x2B
#define OP_HALT 0x3F

enum err_vec {
	WT_REG_0,
	NUM_OF,
	MEM_OF,
	DATA_MIS
};


void simu_halt(void);
void err_handler(u32 instr);
void check_num_OF_subu(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_addu(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_sub(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_add(u32 instr, u32 op_a, u32 op_b);
void check_mem_OF(u32 instr, u32 index, int offset);
void check_data_mis(u32 instr, u32 index, int offset);
void check_data_mis_instr(u32 instr, u32 index);

#endif
