#include "main.h"
#include "pipe_stages.h"
#include "helper.h"
#include "r_type.h"
#include "branch_pre.h"

//pipeline stages
void IF(void)
{
	if (if_nop == 0) {
		if_instr = i_memory[vcpu.PC];
		if_id.instr = if_instr;
		if_op = decoder(if_instr);
		if (((if_op == OP_BEQ) || (if_op == OP_BNE))) {
			branch_pre(if_instr);
		}
		if_id.PC = vcpu.PC + 1;	
	}
	else {	
		if_id.PC = vcpu.PC;
		if_instr = i_memory[vcpu.PC];
		if_op = decoder(if_instr);
		if (((if_op == OP_BEQ) || (if_op == OP_BNE))) {
			branch_pre(if_instr);
		}
		if_nop = 0;
	}		
}

void ID(void)
{
	u32 instr = if_id.instr;	
	u32 op = decoder(instr);
	u32 funct = instr & 0x3F;
	if (instr == 0x00000000) {
		if_id.nop = 1;
	}
	
	if (if_id.nop == 0) {
		if (op == OP_BNE || op == OP_BEQ || (op == R_TYPE && funct == OP_JR)) {
			branch_jr_if(instr);	
			forward_branch();
		}
		id_ex.funct = instr & 0x3F;
		id_ex.rs = (instr >> 21) & 0x1F;
		id_ex.rt = (instr >> 16) & 0x1F;
		id_ex.funct = instr & 0x3F;
		if (op == OP_JAL) {
			id_ex.re_addr = if_id.PC << 2; 
		}
		if (op == R_TYPE && (id_ex.funct == OP_SLL || id_ex.funct == OP_SRL || id_ex.funct == OP_SRA))
			id_ex.RD1 = (instr >> 6) & 0x1F;
		else
			id_ex.RD1 = vcpu.reg[id_ex.rs];
		id_ex.RD2 = vcpu.reg[id_ex.rt];	
		if (((instr >> 15) & 0x1) == 0)
			id_ex.shift_addr = instr & 0xFFFF;
		else
			id_ex.shift_addr = 0xFFFF0000 + (instr & 0xFFFF);
		id_ex.dest1 = (instr >> 16) & 0x1F;
		id_ex.dest0 = (instr >> 11) & 0x1F;
		id_ex.PC = if_id.PC;
		op_name(instr);
		switch(op) {
			case R_TYPE:
				decode_r(instr);
				break;
			case OP_J:
				decode_j(instr);
				break;
			case OP_JAL:
				decode_j(instr);
				break;
			case OP_BEQ:
				decode_i(instr);
				break;
			case OP_BNE:
				decode_i(instr);
				break;
			case OP_ADDI:
				decode_i(instr);
				break;
			case OP_ADDIU:
				decode_i(instr);
				break;
			case OP_SLTI:
				decode_i(instr);
				break;
			case OP_ANDI:
				decode_i(instr);
				break;
			case OP_ORI:
				decode_i(instr);
				break;
			case OP_NORI:
				decode_i(instr);
				break;
			case OP_LUI:
				decode_i(instr);
				break;
			case OP_LB:
				decode_i(instr);
				break;
			case OP_LH:
				decode_i(instr);
				break;
			case OP_LW:
				decode_i(instr);
				break;
			case OP_LBU:
				decode_i(instr);
				break;
			case OP_LHU:
				decode_i(instr);
				break;
			case OP_SB:
				decode_i(instr);
				break;
			case OP_SH:
				decode_i(instr);
				break;
			case OP_SW:
				decode_i(instr);
				break;
			case OP_HALT:
				decode_halt(instr);
				break;
		}
		id_ex.ctrl = control;
		id_ex.name = id_name;
		if ((op != OP_BEQ) && (op != OP_BNE) && ((op != R_TYPE) || (id_ex.funct != OP_JR)) && (op != OP_HALT) && (op != OP_LUI))
			forward();
		branch_forwardA = if_id.forwardA;
		branch_forwardB = if_id.forwardB;
		branch_forwardA_reg = if_id.forwardA_reg;
		branch_forwardB_reg = if_id.forwardB_reg;
		if_id.forwardA = 0x00;
		if_id.forwardB = 0x00;
		if ((id_ex.ctrl.jump == 1) && (id_stall == 0))
			flush = 1;
		else
			flush = 0;
		if ((op == OP_BEQ) || (op == OP_BNE)) {
			id_ex.taken_flag = if_id.taken_flag;
			id_ex.branch_no_PC = if_id.branch_no_PC;
			correct_check();
		}
		if (id_stall == 1) {
			id_ex.forwardA = 0;
			id_ex.forwardB = 0;
		}
	}
	else {
		struct ID_EX temp = {0};
		id_ex = temp;
		id_name = "NOP";
		if_id.nop = 0;
		id_ex.nop = 1;
	}
}

void EX(void)
{
	u32 input_1,input_2;
	if (id_ex.nop == 0) {
		if (id_ex.ctrl.ALUOp != OP_HALT) {
			input_1 = id_ex.RD1;
			if ((id_ex.ctrl.ALUSrc == 0) && ((id_ex.ctrl.ALUOp != OP_SW) && (id_ex.ctrl.ALUOp != OP_SH) && (id_ex.ctrl.ALUOp != OP_SB))) {
				input_2 = id_ex.RD2;
				ex_mem.dest = id_ex.dest0;
			}
			else {
				input_2 = id_ex.shift_addr;
				if ((id_ex.ctrl.ALUOp != OP_SW) && (id_ex.ctrl.ALUOp != OP_SH) && (id_ex.ctrl.ALUOp != OP_SB))
					ex_mem.dest = id_ex.dest1;
				else
					ex_mem.dest = 0;
			}
			switch(id_ex.ctrl.ALUOp) {
				case R_TYPE:
					exec_r(input_1,input_2);
					break;
				case OP_ADDI:
					exec_addi(input_1,input_2);
					break;
				case OP_ADDIU:
					exec_addiu(input_1,input_2);
					break;
				case OP_SLTI:
					exec_slti(input_1,input_2);
					break;
				case OP_ANDI:
					exec_andi(input_1,input_2);
					break;
				case OP_ORI:
					exec_ori(input_1,input_2);
					break;
				case OP_NORI:
					exec_nori(input_1,input_2);
					break;
				case OP_LUI:
					exec_lui(input_1,input_2);
					break;
				case OP_LB:
					exec_load(input_1,input_2);
					break;
				case OP_LH:
					exec_load(input_1,input_2);
					break;
				case OP_LW:
					exec_load(input_1,input_2);
					break;
				case OP_LBU:
					exec_load(input_1,input_2);
					break;
				case OP_LHU:
					exec_load(input_1,input_2);
					break;
				case OP_SB:
					exec_sb(input_1,input_2);
					break;
				case OP_SH:
					exec_sh(input_1,input_2);
					break;
				case OP_SW:
					exec_sw(input_1,input_2);
					break;
			}
		}
			ex_name = id_ex.name;
			ex_mem.name = ex_name;
			ex_mem.ctrl = id_ex.ctrl;
			if (id_ex.ctrl.ALUOp == OP_JAL)
				ex_mem.re_addr = id_ex.re_addr;
			forwardA = id_ex.forwardA;
			forwardB = id_ex.forwardB;
			forwardA_reg = id_ex.forwardA_reg;
			forwardB_reg = id_ex.forwardB_reg;
			id_ex.forwardA = 0x00;
			id_ex.forwardB = 0x00;
			if ((id_ex.ctrl.ALUOp == OP_J) || (id_ex.ctrl.ALUOp == OP_JAL) || ((id_ex.ctrl.ALUOp == R_TYPE) && (id_ex.funct == OP_JR)))
				flush = 0;
	}
	else {
		struct EX_MEM temp = {0};
		ex_mem = temp;
		ex_name = "NOP";
		id_ex.nop = 0;
		ex_mem.nop = 1;
	}
}

void DM(void)
{
	if (ex_mem.nop == 0) {	
		if (ex_mem.ctrl.ALUOp != OP_HALT) {
			if (ex_mem.ctrl.MemWrite == 1 && ex_mem.ctrl.MemRead == 0) {
				int index = ex_mem.alu_result >> 2;
				d_memory[index] = ex_mem.WDtoMem;
			}
			else if (ex_mem.ctrl.MemWrite == 0 && ex_mem.ctrl.MemRead == 1) {
				switch(ex_mem.ctrl.ALUOp) {
					case OP_LB:
						mem_lb();
						break;
					case OP_LH:
						mem_lh();
						break;
					case OP_LW:
						mem_lw();
						break;
					case OP_LBU:
						mem_lbu();
						break;
					case OP_LHU:
						mem_lhu();
						break;
				}

			}
			mem_wb.dest = ex_mem.dest;
			mem_wb.alu_result = ex_mem.alu_result;
			mem_wb.name = ex_mem.name;
			if (ex_mem.ctrl.ALUOp == OP_JAL)
				mem_wb.re_addr = ex_mem.re_addr;
		}
		mem_wb.ctrl = ex_mem.ctrl;
		mem_name = ex_mem.name;
		mem_wb.name = mem_name;
	}
	else {
		struct MEM_WB temp = {0};
		mem_wb = temp;
		mem_name = "NOP";
		ex_mem.nop = 0;
		mem_wb.nop = 1;
	}
}

void WB(void)
{
	if (mem_wb.nop == 0) {
		wb_name = mem_wb.name;
		if (mem_wb.ctrl.ALUOp == OP_HALT) {
			vcpu.halt_flag = 1;
		}
		else {
			if (mem_wb.ctrl.ALUOp == OP_JAL)
				vcpu.reg[31] = mem_wb.re_addr;
			if ((mem_wb.ctrl.RegWrite == 1) && (mem_wb.dest != 0)) {
				if (mem_wb.ctrl.MemtoReg == 1)
					vcpu.reg[mem_wb.dest] = mem_wb.RD;
				else
					vcpu.reg[mem_wb.dest] = mem_wb.alu_result;
			}
		}
	}
	else {
		wb_name = "NOP";
		mem_wb.nop = 0;
	}
}


