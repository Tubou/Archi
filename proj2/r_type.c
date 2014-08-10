#include "main.h"
#include "r_type.h"
#include <math.h>

void decode_r(u32 instr)
{
	u32 op = decoder(instr);
	u32 funct = instr & 0x3F;
	control.RegDst = 1;
	if (funct == OP_JR)
		control.jump = 1;
	else
		control.jump = 0;
	control.branch = 0;
	control.MemRead = 0;
	control.MemtoReg = 0;
	control.ALUOp = op;
	control.MemWrite = 0;
	control.ALUSrc = 0;
	if (funct == OP_JR)
		control.RegWrite = 0;
	else
		control.RegWrite = 1;
}

//OP_SLL
static void exec_sll(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = (input_2 << input_1);
}

//OP_SRL
static void exec_srl(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = (input_2 >> input_1);
}

//OP_SRA
static void exec_sra(u32 input_1, u32 input_2)
{
	u32 signed_bit = (input_2 >> 31);
	u32 sign_exten_bit;
	sign_exten_bit = 0xFFFFFFFF << (32 - input_1);
	if (signed_bit == 1)
		ex_mem.alu_result = ((input_2 >> input_1)+ sign_exten_bit);
	else
		ex_mem.alu_result = (input_2 >> input_1);
}

//OP_ADD
static void exec_add(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;
}

//OP_ADDU
static void exec_addu(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 + input_2;
}

//OP_SUB
static void exec_sub(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 - input_2;
	ex_mem.alu_result = value;
}

//OP_SUBU
static void exec_subu(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 - input_2;
}

//OP_AND
static void exec_and(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 & input_2;
}

//OP_OR
static void exec_or(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 | input_2;
}

//OP_XOR
static void exec_xor(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 ^ input_2;
}

//OP_NOR
static void exec_nor(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = ~(input_1 | input_2);
}

//OP_NAND
static void exec_nand(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = ~(input_1 & input_2);
}

//OP_SLT
static void exec_slt(u32 input_1, u32 input_2)
{
	signed int RS = input_1;
	signed int RT = input_2;
	if (RS < RT)
		ex_mem.alu_result = 1;
	else
		ex_mem.alu_result = 0;
}

//OP_SLTU
static void exec_sltu(u32 input_1, u32 input_2)
{
	u32 RS = input_1;
	u32 RT = input_2;
	if (RS < RT)
		ex_mem.alu_result = 1;
	else
		ex_mem.alu_result = 0;
}

void exec_r(u32 input_1, u32 input_2)
{
	switch (id_ex.funct) {
		case OP_SLL:
			exec_sll(input_1,input_2);
			break;
		case OP_SRL:
			exec_srl(input_1,input_2);
			break;
		case OP_SRA:
			exec_sra(input_1,input_2);
			break;
		case OP_ADD:
			exec_add(input_1,input_2);
			break;
		case OP_ADDU:
			exec_addu(input_1,input_2);
			break;
		case OP_SUB:
			exec_sub(input_1,input_2);
			break;
		case OP_SUBU:
			exec_subu(input_1,input_2);
			break;
		case OP_AND:
			exec_and(input_1,input_2);
			break;
		case OP_OR:
			exec_or(input_1,input_2);
			break;
		case OP_XOR:
			exec_xor(input_1,input_2);
			break;
		case OP_NOR:
			exec_nor(input_1,input_2);
			break;
		case OP_NAND:
			exec_nand(input_1,input_2);
			break;
		case OP_SLT:
			exec_slt(input_1,input_2);
			break;
		case OP_SLTU:
			exec_sltu(input_1,input_2);
			break;
	}
}
