#include "main.h"
#include "j_type.h"


static struct op_j fetch_op(u32 instr)
{
	struct op_j op_j;
	op_j.C = instr & 0x3FFFFFF;
	return op_j;
}


//OP_J
static void exec_j(struct op_j op_j)
{
	vcpu.PC = (vcpu.PC & 0xF0000000) | op_j.C;
}

void simu_j(u32 instr)
{
	struct op_j op_j = fetch_op(instr);
	exec_j(op_j);
}

//OP_JAL
static void exec_jal(struct op_j op_j)
{
	vcpu.reg[31] = vcpu.PC << 2;
	vcpu.PC = (vcpu.PC & 0xF0000000) | op_j.C;
}


void simu_jal(u32 instr)
{
	struct op_j op_j = fetch_op(instr);
	exec_jal(op_j);
}
