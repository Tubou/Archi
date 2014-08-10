#include "main.h"
#include "i_type.h"

static void store_result_to_rt(u32 result, struct op_i op_i, u32 instr)
{
	if (op_i.rt != 0)
		vcpu.reg[op_i.rt] = result;
}

static struct op_i fetch_op(u32 instr)
{
	struct op_i op_i;
	op_i.rs = (instr >> 21) & 0x1F;
	op_i.rt = (instr >> 16) & 0x1F;
	op_i.C = instr & 0xFFFF;
	if ((instr >> 15) & 0x1 == 1)
		op_i.sign_C = (instr & 0xFFFF) + 0xFFFF0000;
	else
		op_i.sign_C = (instr & 0xFFFF);
	return op_i;
}


//OP_BEQ
static void exec_beq(struct op_i op_i)
{
	if (vcpu.reg[op_i.rs] == vcpu.reg[op_i.rt]) {
		vcpu.PC = (signed int)vcpu.PC + op_i.sign_C;
	}
}

void simu_beq(u32 instr)
{
	struct op_i op_i = fetch_op(instr);

	exec_beq(op_i);
}

//OP_BNE
static void exec_bne(struct op_i op_i)
{
	if (vcpu.reg[op_i.rs] != vcpu.reg[op_i.rt])
		vcpu.PC = (signed int) vcpu.PC + op_i.sign_C;
}

void simu_bne(u32 instr)
{
	struct op_i op_i = fetch_op(instr);

	exec_bne(op_i);
}

//ADDi
static u32 exec_addi(struct op_i op_i)
{
	signed int value = (signed int) vcpu.reg[op_i.rs] + op_i.sign_C;
	u32 result = value;
	return result;
}

void simu_addi(u32 instr)
{
	struct op_i op_i = fetch_op(instr);

	u32 result = exec_addi(op_i);
	store_result_to_rt(result, op_i, instr);
}

//ADDIU
static u32 exec_addiu(struct op_i op_i)
{
	return vcpu.reg[op_i.rs] + op_i.C;
}

void simu_addiu(u32 instr)
{
	struct op_i op_i = fetch_op(instr);

	u32 result = exec_addiu(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_SLTI
static u32 exec_slti(struct op_i op_i)
{
	signed int RS = vcpu.reg[op_i.rs];
	if (RS < op_i.sign_C)
		return 1;
	else
		return 0;
}

void simu_slti(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_slti(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_ANDI
static u32 exec_andi(struct op_i op_i)
{
	return vcpu.reg[op_i.rs] & op_i.C;
}

void simu_andi(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_andi(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_ORI
static u32 exec_ori(struct op_i op_i)
{
	return vcpu.reg[op_i.rs] | op_i.C;
}

void simu_ori(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_ori(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_NORI
static u32 exec_nori(struct op_i op_i)
{
	return ~(vcpu.reg[op_i.rs] | op_i.C);
}

void simu_nori(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_nori(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_LUI
static u32 exec_lui(struct op_i op_i)
{
	return op_i.C << 16;
}

void simu_lui(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lui(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_LB
static u32 exec_lb(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 data = read_Dmemory(VAddr);
	if ((((data >> (24 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) >> 7) & 0x1) == 1)
		return ((data >> (24 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x000000FF) + 0xFFFFFF00;
	else
		return ((data >> (24 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x000000FF);
}

void simu_lb(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lb(op_i);	
	store_result_to_rt(result, op_i, instr);
}

//OP_LH
static u32 exec_lh(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 data = read_Dmemory(VAddr);
	if((((data >> (16 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) >> 15) & 0x1) == 1)
		return ((data >> (16 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x0000FFFF) + 0xFFFF0000;
	else
		return ((data >> (16 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x0000FFFF);
}

void simu_lh(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lh(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_LW
static u32 exec_lw(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 data = read_Dmemory(VAddr);
	return data;
}

void simu_lw(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lw(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_LBU
static u32 exec_lbu(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 data = read_Dmemory(VAddr);
	return ((data >> (24 - 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x000000FF);
}

void simu_lbu(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lbu(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_LHU
static u32 exec_lhu(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 data = read_Dmemory(VAddr);
	return ((data >> (16 - 8 * ((vcpu.reg[op_i.rs] + op_i.sign_C) % 4))) & 0x0000FFFF);
}

void simu_lhu(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	u32 result = exec_lhu(op_i);
	store_result_to_rt(result, op_i, instr);
}

//OP_SB
static void exec_sb(struct op_i op_i)
{
	int VAddr = ((int) vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 shift = 8 * (((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4);
	u32 shift_byte = ((int) vcpu.reg[op_i.rs] + op_i.sign_C) % 4;
	u32 top_bit = (0xFF000000 * (shift_byte * (2 - shift_byte) * (3 - shift_byte) >> 1)) + (0xFFFF0000 * (shift_byte * (shift_byte - 1) * (3 - shift_byte) >> 1))+(0xFFFFFF00 * (shift_byte * (shift_byte - 1) * (shift_byte - 2) / 6));
	u32 bit_save = (vcpu.reg[op_i.rt] & 0x000000FF) << (24 - shift);
	write_Dmemory(VAddr,shift,top_bit,bit_save,OP_SB);
}

void simu_sb(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	exec_sb(op_i);
}

//OP_SH
static void exec_sh(struct op_i op_i)
{
	int VAddr = (vcpu.reg[op_i.rs] + op_i.sign_C);
	u32 shift = 8 * ((vcpu.reg[op_i.rs] + op_i.sign_C) % 4);
	u32 bit_save = (vcpu.reg[op_i.rt] & 0x0000FFFF) << (16 - shift);
	write_Dmemory(VAddr,shift,0,bit_save,OP_SH);
}

void simu_sh(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	exec_sh(op_i);
}

//OP_SW
static void exec_sw(struct op_i op_i)
{
	int VAddr = (vcpu.reg[op_i.rs] + op_i.sign_C);
	write_Dmemory(VAddr,0,0,vcpu.reg[op_i.rt],OP_SW);	
}

void simu_sw(u32 instr)
{
	struct op_i op_i = fetch_op(instr);
	exec_sw(op_i);
}

