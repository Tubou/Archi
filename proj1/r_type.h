#ifndef H_R_TYPE
#define H_R_TYPE

#define OP_SLL 0x00
#define OP_SRL 0x02
#define OP_SRA 0x03
#define OP_JR 0x08
#define OP_ADD 0x20
#define OP_ADDU 0x21
#define OP_SUB 0x22
#define OP_SUBU 0x23
#define OP_AND 0x24
#define OP_OR 0x25
#define OP_XOR 0x26
#define OP_NOR  0x27
#define OP_NAND 0x28
#define OP_SLT 0x2A
#define OP_SLTU 0x2B

struct op_r {
 	u32 rs;
	u32 rt;
	u32 rd;
	u32 C;
};



void simu_r(u32 instr);

#endif
