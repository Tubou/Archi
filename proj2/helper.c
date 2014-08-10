#include "main.h"
#include "helper.h"
#include "r_type.h"

//forward function
static void nop_id(void)
{
	if_nop = 1;
	id_ex.nop = 1;
	id_stall = 1;
}

void forward_branch(void)
{
	u32 rs,rt;
	u32 op = decoder(if_id.instr);
	rs = (if_id.instr >> 21) & 0x1F;
	rt = (if_id.instr >> 16) & 0x1F;
	if ((ex_mem.ctrl.RegWrite == 1) && (ex_mem.dest != 0)) {
		if (rs == ex_mem.dest)
			nop_id();
		if (((op == OP_BEQ) || (op == OP_BNE)) && (rt == ex_mem.dest))
			nop_id();
	}
	if (id_stall == 0) {
		if ((mem_wb.ctrl.ALUOp == OP_JAL) && (ex_mem.dest != 31) && (ex_mem.ctrl.ALUOp != OP_JAL)) {
			if (rs == 31) {
				if ((op == OP_BEQ) || (op == OP_BNE))
					if_id.branch_RD1 = mem_wb.re_addr;
				else
					id_ex.jump_PC = (mem_wb.re_addr >> 2);
				if_id.forwardA = 0x10;
				if_id.forwardA_reg = 31;
			}
			if ( ((op == OP_BEQ) || (op == OP_BNE)) && (rt == 31)) {
				if_id.branch_RD2 = mem_wb.re_addr;
				if_id.forwardB = 0x10;
				if_id.forwardB_reg = 31;
			}
		}
		if ((mem_wb.ctrl.RegWrite == 1) && (mem_wb.dest != 0) && (mem_wb.dest != ex_mem.dest)) {
			if (rs == mem_wb.dest) {
				if (mem_wb.ctrl.MemtoReg == 0) {
					if ((op == OP_BEQ) || (op == OP_BNE))
						if_id.branch_RD1 = mem_wb.alu_result;
					else
						id_ex.jump_PC = (mem_wb.alu_result >> 2);
					if_id.forwardA = 0x10;
					if_id.forwardA_reg = mem_wb.dest;
				}
				else
					nop_id();
			}
			if (((op == OP_BEQ) || (op == OP_BNE)) && (rt == mem_wb.dest)) {
				if (mem_wb.ctrl.MemtoReg == 0) {
					if_id.branch_RD2 = mem_wb.alu_result;
					if_id.forwardB = 0x10;
					if_id.forwardB_reg = mem_wb.dest;
				}
				else
					nop_id();
			}
		}
	}
}

void forward(void)
{	
	if ((ex_mem.ctrl.RegWrite == 1) && (ex_mem.dest != 0)) {
		if (!((id_ex.ctrl.ALUOp == R_TYPE) && ((id_ex.funct == OP_SRA) || (id_ex.funct == OP_SLL) || (id_ex.funct == OP_SRL)))) {
			if (id_ex.rs == ex_mem.dest) {
				if (ex_mem.ctrl.MemtoReg == 0) {	
					id_ex.RD1 = ex_mem.alu_result;
					id_ex.forwardA = 0x10;
					id_ex.forwardA_reg = ex_mem.dest;
				}
				else
					nop_id();
			}
		}
		if ((id_ex.rt == ex_mem.dest) && (id_ex.ctrl.ALUSrc == 0)) {
			if (ex_mem.ctrl.MemtoReg == 0) {
				id_ex.RD2 = ex_mem.alu_result;
				id_ex.forwardB = 0x10;
				id_ex.forwardB_reg = ex_mem.dest;
			}
			else
				nop_id();
		}
	}
	if ((mem_wb.ctrl.ALUOp == OP_JAL) && (ex_mem.dest != 31) && (ex_mem.ctrl.ALUOp != OP_JAL)) {
		if (!((id_ex.ctrl.ALUOp == R_TYPE) && ((id_ex.funct == OP_SRA) || (id_ex.funct == OP_SLL) || (id_ex.funct == OP_SRL)))) {
			if (id_ex.rs == 31) {
				id_ex.RD1 = mem_wb.re_addr;
				id_ex.forwardA = 0x01;
				id_ex.forwardA_reg = 31;
			}
		}
		if ((id_ex.rt == 31) && (id_ex.ctrl.ALUSrc == 0)) {
			id_ex.RD2 = mem_wb.re_addr;
			id_ex.forwardB = 0x01;
			id_ex.forwardB_reg = 31;
		}
	}
	if ((mem_wb.ctrl.RegWrite == 1) && (mem_wb.dest != 0) && (mem_wb.dest != ex_mem.dest)) {
		if (!((id_ex.ctrl.ALUOp == R_TYPE) && ((id_ex.funct == OP_SRA) || (id_ex.funct == OP_SLL) || (id_ex.funct == OP_SRL)))) {
			if (id_ex.rs == mem_wb.dest) {
				if (mem_wb.ctrl.MemtoReg == 1) {
					id_ex.RD1 = mem_wb.RD;
					id_ex.forwardA = 0x01;
					id_ex.forwardA_reg = mem_wb.dest;
				}
				else {
					id_ex.RD1 = mem_wb.alu_result;
					id_ex.forwardA = 0x01;
					id_ex.forwardA_reg = mem_wb.dest;
				}
			}
		}
		if ((id_ex.rt == mem_wb.dest) && (id_ex.ctrl.ALUSrc == 0)) {
			if (mem_wb.ctrl.MemtoReg == 1) {
				id_ex.RD2 = mem_wb.RD;
				id_ex.forwardB = 0x01;
				id_ex.forwardB_reg = mem_wb.dest;
			}
			else {
				id_ex.RD2 = mem_wb.alu_result;
				id_ex.forwardB = 0x01;
				id_ex.forwardB_reg = mem_wb.dest;
			}
		}
	}
}

//translate name to string
static void op_name_r(void)
{
	switch (id_ex.funct) {
		case OP_SLL:
			id_name = "SLL";
			break;
		case OP_SRL:
			id_name = "SRL";
			break;
		case OP_SRA:
			id_name = "SRA";
			break;
		case OP_JR:
			id_name = "JR";
			break;
		case OP_ADD:
			id_name = "ADD";
			break;
		case OP_ADDU:
			id_name = "ADDU";
			break;
		case OP_SUB:
			id_name = "SUB";
			break;
		case OP_SUBU:
			id_name = "SUBU";
			break;
		case OP_AND:
			id_name = "AND";
			break;
		case OP_OR:
			id_name = "OR";
			break;
		case OP_XOR:
			id_name = "XOR";
			break;
		case OP_NOR:
			id_name = "NOR";
			break;
		case OP_NAND:
			id_name = "NAND";
			break;
		case OP_SLT:
			id_name = "SLT";
			break;
		case OP_SLTU:
			id_name = "SLTU";
			break;
	}
}

void op_name(u32 instr)
{
	u32 op = decoder(instr);
	switch(op) {
		case R_TYPE:
			op_name_r();
			break;
		case OP_J:
			id_name = "J";
			break;
		case OP_JAL:
			id_name = "JAL";
			break;
		case OP_BEQ:
			id_name = "BEQ";
			break;
		case OP_BNE:
			id_name = "BNE";
			break;
		case OP_ADDI:
			id_name = "ADDI";
			break;
		case OP_ADDIU:
			id_name = "ADDIU";
			break;
		case OP_SLTI:
			id_name = "SLTI";
			break;
		case OP_ANDI:
			id_name = "ANDI";
			break;
		case OP_ORI:
			id_name = "ORI";
			break;
		case OP_NORI:
			id_name = "NORI";
			break;
		case OP_LUI:
			id_name = "LUI";
			break;
		case OP_LB:
			id_name = "LB";
			break;
		case OP_LH:
			id_name = "LH";
			break;
		case OP_LW:
			id_name = "LW";
			break;
		case OP_LBU:
			id_name = "LBU";
			break;
		case OP_LHU:
			id_name = "LHU";
			break;
		case OP_SB:
			id_name = "SB";
			break;
		case OP_SH:
			id_name = "SH";
			break;
		case OP_SW:
			id_name = "SW";
			break;
		case OP_HALT:
			id_name = "HALT";
			break;
		default:
			id_name = "NOP";
	}

}
