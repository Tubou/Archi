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
	u32 halt_flag;
};

struct CTRL {
	int RegDst;
	int jump;
	int branch;
	int MemRead;
	int MemtoReg;
	u32 ALUOp;
	int MemWrite;
	int ALUSrc;
	int RegWrite;
};

struct IF_ID {
	u32 instr;
	u32 PC;
	u32 branch_RD1;
	u32 branch_RD2;
	u32 forwardA;
	u32 forwardB;
	int forwardA_reg;
	int forwardB_reg;
	u32 index;
	int nop;
	u32 branch_no_PC;
	int taken_flag;
	u32 taken;
};

struct ID_EX {
	struct CTRL ctrl;
	u32 PC;
	u32 rs;
	u32 rt;
	u32 RD1;
	u32 RD2;
	u32 shift_addr;
	u32 dest0;
	u32 dest1;
	u32 jump_PC;
	u32 funct;
	int branch;
	int zero;
	char *name;
	int nop;
	int forwardA_reg;
	int forwardB_reg;
	u32 forwardA;
	u32 forwardB;
	u32 re_addr;
	int taken_flag;
	u32 branch_no_PC;
	u32 taken;
	u32 index;
};

struct EX_MEM {
	struct CTRL ctrl;
	u32 alu_result;
	u32 WDtoMem;
	u32 dest;
	char *name;
	int nop;
	u32 re_addr;
};

struct MEM_WB {
	struct CTRL ctrl;
	u32 RD;
	u32 dest;
	u32 alu_result;
	char *name;
	int nop;
	u32 re_addr;
};

extern struct VCPU vcpu;
extern struct CTRL control;
extern struct IF_ID if_id;
extern struct ID_EX id_ex;
extern struct EX_MEM ex_mem;
extern struct MEM_WB mem_wb;

extern int cycle;
extern int id_stall;
extern int if_nop;
extern char *id_name;
extern u32 if_instr;
extern u32 forwardA;
extern u32 forwardB;
extern int forwardA_reg;
extern int forwardB_reg;
extern u32 branch_forwardA;
extern u32 branch_forwardB;
extern int branch_forwardA_reg;
extern int branch_forwardB_reg;
extern char *id_name;
extern char *wb_name;
extern char *ex_name;
extern char *mem_name;
extern int correct;
extern u32 if_op;
extern int flush;
extern u32 BHT[16];
extern int taken_flag;

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

u32 decoder(u32 instr);
void branch_jr_if(u32 instr);
void decode_halt(u32 instr);
#endif
