#include "main.h"
#include "i_type.h"
#include "branch_pre.h"



void decode_i(u32 instr)
{
	u32 op = decoder(instr);
	control.RegDst = 1;
	control.jump = 0;
	if (op == OP_BEQ || op == OP_BNE) {
		control.branch = 1;
	}
	else {
		control.branch = 0;
	}
	if (op == OP_LW || op == OP_LH || op == OP_LHU || op == OP_LB || op == OP_LBU) {
		control.MemRead = 1;
		control.MemtoReg = 1;
	}
	else {
		control.MemRead = 0;
		control.MemtoReg = 0;
	}
	control.ALUOp = op;
	if (op == OP_SW || op == OP_SH || op == OP_SB)
		control.MemWrite = 1;
	else
		control.MemWrite = 0;
	if (op == OP_BEQ || op == OP_BNE || op == OP_SW || op ==OP_SH || op == OP_SB) {
		control.RegWrite = 0;
		control.ALUSrc = 0;
	}
	else {
		control.RegWrite = 1;
		control.ALUSrc = 1;
	}
	if (op == OP_BEQ || op == OP_BNE) {
		decode_branch(op);
	}
}

//ADDi
void exec_addi(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;	
}

//ADDIU
void exec_addiu(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 + (input_2 & 0xFFFF);
}

//OP_SLTI
void exec_slti(u32 input_1, u32 input_2)
{
	signed int RS = input_1;
	signed int C = input_2;
	if (RS < C)
		ex_mem.alu_result = 1;
	else
		ex_mem.alu_result = 0;
}

//OP_ANDI
void exec_andi(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 & (input_2 & 0xFFFF);
}

//OP_ORI
void exec_ori(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_1 | (input_2 & 0xFFFF);
}

//OP_NORI
void exec_nori(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = ~(input_1 | (input_2 & 0xFFFF));
}

//OP_LUI
void exec_lui(u32 input_1, u32 input_2)
{
	ex_mem.alu_result = input_2 << 16;
}

//load exec
void exec_load(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;
}

//OP_LB
void mem_lb(void)
{
	int index = ex_mem.alu_result >> 2;
	if ((((d_memory[index] >> (24 - 8 * (ex_mem.alu_result % 4))) >> 7) & 0x1) == 1)
		mem_wb.RD = ((d_memory[index] >> (24 - 8 * (ex_mem.alu_result % 4))) & 0x000000FF) + 0xFFFFFF00;
	else
		mem_wb.RD = ((d_memory[index] >> (24 - 8 * (ex_mem.alu_result % 4))) & 0x000000FF);
}

//OP_LH
void mem_lh(void)
{
	int index = ex_mem.alu_result >> 2;
	if((((d_memory[index] >> (16 - 8 * (ex_mem.alu_result % 4))) >> 15) & 0x1) == 1)
		mem_wb.RD = ((d_memory[index] >> (16 - 8 * (ex_mem.alu_result % 4))) & 0x0000FFFF) + 0xFFFF0000;
	else
		mem_wb.RD = ((d_memory[index] >> (16 - 8 * (ex_mem.alu_result % 4))) & 0x0000FFFF);
}

//OP_LW
void mem_lw(void)
{
	int index = ex_mem.alu_result >> 2;
	mem_wb.RD = d_memory[index];
}

//OP_LBU
void mem_lbu(void)
{
	int index = ex_mem.alu_result >> 2;
	mem_wb.RD = ((d_memory[index] >> (24 - 8 * (ex_mem.alu_result % 4))) & 0x000000FF);
}

//OP_LHU
void mem_lhu(void)
{
	int index = ex_mem.alu_result >> 2;
	mem_wb.RD = ((d_memory[index] >> (16 - 8 * (ex_mem.alu_result % 4))) & 0x0000FFFF);
}

//OP_SB
void exec_sb(u32 input_1, u32 input_2)
{	
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;
	u32 index = value >> 2;
	u32 shift = 8 * (value % 4);
	u32 shift_byte = value % 4;
	u32 top_bit = (0xFF000000 * (shift_byte * (2 - shift_byte) * (3 - shift_byte) >> 1)) + (0xFFFF0000 * (shift_byte * (shift_byte - 1) * (3 - shift_byte) >> 1))+(0xFFFFFF00 * (shift_byte * (shift_byte - 1) * (shift_byte - 2) / 6));
	u32 bit_save = (id_ex.RD2 & 0x000000FF) << (24 - shift) ;
	u32 bit_reserve = d_memory[index] & ((0x00FFFFFF >> shift) + top_bit);
	ex_mem.WDtoMem = bit_save + bit_reserve;

}

//OP_SH
void exec_sh(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;
	u32 index = value >> 2;
	u32 shift = 8 * (value % 4);
	u32 bit_save = (id_ex.RD2 & 0x0000FFFF) << (16 - shift);
	u32 bit_reserve = d_memory[index] & (0x0000FFFF << shift);
	ex_mem.WDtoMem = bit_save + bit_reserve;
}

//OP_SW
void exec_sw(u32 input_1, u32 input_2)
{
	signed int value = (signed int) input_1 + input_2;
	ex_mem.alu_result = value;
	ex_mem.WDtoMem = id_ex.RD2;
}
