#include "main.h"
#include "r_type.h"
#include <math.h>

static void store_dest_reg(u32 instr, u32 dest_value, struct op_r op_r)
{
	if (op_r.rd != 0)
	         vcpu.reg[op_r.rd] = dest_value;
	else {
		vcpu.err_flag = WT_REG_0;
		err_handler(instr);
	}
}

static u32 decode_funct(u32 instr)
{
	return instr & 0x3F;
}

static struct op_r fetch_op(u32 instr)
{
	struct op_r op_r;
	op_r.rs = (instr >> 21) & 0x1F;
	op_r.rt = (instr >> 16) & 0x1F;
	op_r.rd = (instr >> 11) & 0x1F;
	op_r.C = (instr >> 6) & 0x1F;
	
	return op_r;
}


//OP_SLL
static u32 exec_sll(struct op_r op_r)
{
	return  (vcpu.reg[op_r.rt] << op_r.C);
}

static void simu_sll(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_sll(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SRL
static u32 exec_srl(struct op_r op_r)
{
	return (vcpu.reg[op_r.rt] >> op_r.C);

}

static void simu_srl(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_srl(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SRA
static u32 exec_sra(struct op_r op_r)
{
	u32 signed_bit = (vcpu.reg[op_r.rt] >> 31);
	u32 sign_exten_bit;
	sign_exten_bit = 0xFFFFFFFF << (32 - op_r.C);
	if (signed_bit == 1)
		return ((vcpu.reg[op_r.rt] >> op_r.C)+ sign_exten_bit);
	else
		return (vcpu.reg[op_r.rt] >> op_r.C);
}

static void simu_sra(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_sra(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_JR
static void exec_jr(struct op_r op_r)
{
	vcpu.PC = vcpu.reg[op_r.rs] >> 2;
}

static void simu_jr(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	check_data_mis_instr(instr,vcpu.reg[op_r.rs]);
	exec_jr(op_r);
}

//OP_ADD
static u32 exec_add(struct op_r op_r)
{
	signed int value = (signed int) vcpu.reg[op_r.rs] + vcpu.reg[op_r.rt];
	u32 result = value;
	return result;
}

static void simu_add(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	check_num_OF_add(instr, vcpu.reg[op_r.rs], vcpu.reg[op_r.rt]);
	u32 dest_value = exec_add(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_ADDU
static u32 exec_addu(struct op_r op_r)
{
	return (vcpu.reg[op_r.rs] + vcpu.reg[op_r.rt]);
}

static void simu_addu(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	check_num_OF_addu(instr, vcpu.reg[op_r.rs], vcpu.reg[op_r.rt]);
	u32 dest_value = exec_addu(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SUB
static u32 exec_sub(struct op_r op_r)
{
	signed int value = (signed int) vcpu.reg[op_r.rs] - vcpu.reg[op_r.rt];
	u32 result = value;
	return result;
}

static void simu_sub(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	check_num_OF_sub(instr, vcpu.reg[op_r.rs], vcpu.reg[op_r.rt]);
	u32 dest_value = exec_sub(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SUBU
static u32 exec_subu(struct op_r op_r)
{
	return (vcpu.reg[op_r.rs] - vcpu.reg[op_r.rt]);
}

static void simu_subu(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	check_num_OF_subu(instr, vcpu.reg[op_r.rs], vcpu.reg[op_r.rt]);
	u32 dest_value = exec_subu(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_AND
static u32 exec_and(struct op_r op_r)
{
	return (vcpu.reg[op_r.rs] & vcpu.reg[op_r.rt]);
}

static void simu_and(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_and(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_OR
static u32 exec_or(struct op_r op_r)
{
	return (vcpu.reg[op_r.rs] | vcpu.reg[op_r.rt]);
}

static void simu_or(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_or(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_XOR
static u32 exec_xor(struct op_r op_r)
{
	return (vcpu.reg[op_r.rs] ^ vcpu.reg[op_r.rt]);
}

static void simu_xor(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_xor(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_NOR
static u32 exec_nor(struct op_r op_r)
{
	return ~(vcpu.reg[op_r.rs] | vcpu.reg[op_r.rt]);
}

static void simu_nor(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_nor(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_NAND
static u32 exec_nand(struct op_r op_r)
{
	return ~(vcpu.reg[op_r.rs] & vcpu.reg[op_r.rt]);
}

static void simu_nand(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_nand(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SLT
static u32 exec_slt(struct op_r op_r)
{
	signed int RS = vcpu.reg[op_r.rs];
	signed int RT = vcpu.reg[op_r.rt];
	if (RS < RT)
		return 1;
	else
		return 0;
}

static void simu_slt(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_slt(op_r);
	store_dest_reg(instr, dest_value, op_r);
}

//OP_SLTU
static u32 exec_sltu(struct op_r op_r)
{
	u32 RS = vcpu.reg[op_r.rs];
	u32 RT = vcpu.reg[op_r.rt];
	if (RS < RT)
		return 1;
	else
		return 0;
}

static void simu_sltu(u32 instr)
{
	struct op_r op_r = fetch_op(instr);
	u32 dest_value = exec_sltu(op_r);
	store_dest_reg(instr, dest_value, op_r);
}


void simu_r(u32 instr)
{
	u32 funct;
	funct = decode_funct(instr);
	switch (funct) {
		case OP_SLL:
			simu_sll(instr);
			break;
		case OP_SRL:
			simu_srl(instr);
			break;
		case OP_SRA:
			simu_sra(instr);
			break;
		case OP_JR:
			simu_jr(instr);
			break;
		case OP_ADD:
			simu_add(instr);
			break;
		case OP_ADDU:
			simu_addu(instr);
			break;
		case OP_SUB:
			simu_sub(instr);
			break;
		case OP_SUBU:
			simu_subu(instr);
			break;
		case OP_AND:
			simu_and(instr);
			break;
		case OP_OR:
			simu_or(instr);
			break;
		case OP_XOR:
			simu_xor(instr);
			break;
		case OP_NOR:
			simu_nor(instr);
			break;
		case OP_NAND:
			simu_nand(instr);
			break;
		case OP_SLT:
			simu_slt(instr);
			break;
		case OP_SLTU:
			simu_sltu(instr);
			break;
	}
}
