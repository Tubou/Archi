#include "main.h"
#include "branch_pre.h"


void branch_pre(u32 instr)
{
	u32 shift;
	int value;
	u32 index;
	u32 branch_PC;
	u32 taken;
	if (((instr >> 15) & 0x1) == 0)
		shift = instr & 0xFFFF;
	else
		shift = 0xFFFF0000 + (instr & 0xFFFF);
	value = (signed int) vcpu.PC + 1 + shift;
	branch_PC = (u32) value;
	index = vcpu.PC & 0xF;
	taken = BHT[index];
	if (if_nop == 0)
		if_id.taken = taken;
	//printf("cycle:%d index:%d taken:%d\n",cycle,index,taken);
	if ((taken == 0) || (taken == 1)) {
		taken_flag = 0;
		if (if_nop == 0) {
			if_id.branch_no_PC = branch_PC;
			vcpu.PC = vcpu.PC;
		}
	}
	else {
		taken_flag = 1;
		if (if_nop == 0) {
			if_id.branch_no_PC = vcpu.PC + 1;
			vcpu.PC = branch_PC - 1;
		}
	}
	if (if_nop == 0) {
		if_id.taken_flag = taken_flag;
		if_id.index = index;
	}
}

static void not_taken_0(void)
{
	if (id_ex.branch == 1)
		id_ex.taken = 1;
}

static void not_taken_1(void)
{
	if (id_ex.branch == 1)
		id_ex.taken = 2;
	else
		id_ex.taken = 0;
}

static int taken_0(void)
{
	if (id_ex.branch == 1)
		return 3;
	else
		return 1;
}

static void taken_1(void)
{
	if (id_ex.branch == 0)
		id_ex.taken = 2;
}

void correct_check(void)
{
	id_ex.index = if_id.index;
	if (id_ex.branch == id_ex.taken_flag)
		correct = 1;
	else {
		correct = 0;
		if (id_stall == 0) {	
			if_id.nop = 1;
			flush = 1;
		}
	}
	id_ex.index = if_id.index;
	id_ex.taken = if_id.taken;
}

void branch_state(void)
{
	if ((id_stall == 0) && ((id_ex.ctrl.ALUOp == OP_BNE) || (id_ex.ctrl.ALUOp == OP_BEQ))) {
		switch(id_ex.taken) {
			case 0:
				not_taken_0();
				break;
			case 1:
				not_taken_1();
				break;
			case 2:
				id_ex.taken = taken_0();
				break;
			case 3:
				taken_1();
				break;
		}
		BHT[id_ex.index] = id_ex.taken;
	}
}

void decode_branch(u32 op)
{
	if (op == OP_BEQ) { 
		if (if_id.branch_RD1 == if_id.branch_RD2)
			id_ex.zero = 1;
		else
			id_ex.zero = 0;
	}
	if (op == OP_BNE) {
		if (if_id.branch_RD1 != if_id.branch_RD2)
			id_ex.zero = 1;
		else
			id_ex.zero = 0;
	}
	id_ex.branch = id_ex.zero & control.branch;
}

