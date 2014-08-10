#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "r_type.h"
#include "i_type.h"
#include "j_type.h"
#include "helper.h"
#include "pipe_stages.h"

u32 i_memory[0x100] = {0};
u32 d_memory[0x100] = {0};
u32 instr_max;
u32 data_max;
struct VCPU vcpu;
int cycle = 0;
int check_PC;
int real_PC;
int if_nop;
int id_stall;
u32 if_instr;
u32 forwardA;
u32 forwardB;
int forwardA_reg;
int forwardB_reg;
u32 branch_forwardA;
u32 branch_forwardB;
int branch_forwardA_reg;
int branch_forwardB_reg;
char *id_name;
char *wb_name;
char *ex_name;
char *mem_name;
int correct;
u32 if_op;
struct CTRL control;
int flush;
u32 BHT[16];
int taken_flag;

struct IF_ID if_id;
struct ID_EX id_ex;
struct EX_MEM ex_mem;
struct MEM_WB mem_wb;

static char *flush_str;
static char *correct_str;
static char *taken_str;

//initial register
static void init_register(void)
{
	int i;
	id_ex.name = "NOP";
	ex_mem.name = "NOP";
	mem_wb.name = "NOP";
	wb_name = "NOP";
	if_id.nop = 1;
	id_ex.nop = 1;
	ex_mem.nop = 1;
	mem_wb.nop = 1;
	for (i = 0; i < 16; i++) {
		BHT[i] = 1;
	}
}

//decoder
u32 decoder(u32 instr)
{
	return instr >> 26;
}

void decode_halt(u32 instr)
{
	control.ALUOp = decoder(instr);
	control.jump = 0;
	control.branch = 0;
	control.MemRead = 0;
	control.MemWrite = 0;
	control.RegWrite = 0;	
}

//branch IF
void branch_jr_if(u32 instr)
{
	u32 op = decoder(instr);
	u32 rs = (instr >> 21) & 0x1F;
	u32 rt = (instr >> 16) & 0x1F;
	if (op == OP_BEQ || op == OP_BNE) {
		if_id.branch_RD1 = vcpu.reg[rs];
		if_id.branch_RD2 = vcpu.reg[rt];
	}
	else
		id_ex.jump_PC = vcpu.reg[rs] >> 2;
}

//loader
static u32 combine_instr(FILE *fp)
{
	u32 input;
	u32 instr = 0;
	int i;
	for (i = 1; i <= 4; i++) {
		fread(&input, sizeof(char),1,fp);
		instr = instr + (input << (8 * (4 - i)));
	}
	return instr;
}

static void loader(void)
{
	FILE *fp_i,*fp_d;
	u32 instr = 0;
	int i;
	fp_i=fopen("iimage.bin", "rb");
	fp_d=fopen("dimage.bin", "rb");
	instr = combine_instr(fp_i);
	vcpu.PC = instr >> 2;
	check_PC = instr % 4;
	real_PC = instr;
	instr = combine_instr(fp_i);
	instr_max = instr;
	for (i = vcpu.PC; i < (instr_max + vcpu.PC); i++) {
		instr = combine_instr(fp_i);
		i_memory[i]=instr;	
	}
	instr = combine_instr(fp_d);
	vcpu.reg[29] = instr;
	instr = combine_instr(fp_d);
	data_max = instr;
	for (i = 0; i < data_max; i++) {
		instr = combine_instr(fp_d);
		d_memory[i] = instr;
	}	
	fclose(fp_i);
	fclose(fp_d);
}

//snapshot printer
static void print_reg(void)
{
	int i;
	FILE *fp;
	fp = fopen("snapshot.rpt", "a");
	fprintf(fp, "cycle %d\n", cycle);
	for (i = 0; i < 32; i++) {
		fprintf(fp, "$%02d: 0x%08X\n", i, vcpu.reg[i]);
	}
	fprintf(fp, "PC: 0x%08X\n", real_PC);
	fclose(fp);
}

static void print_stage(void)
{	
	FILE *fp;
	char *stall_str;
	fp = fopen("snapshot.rpt", "a");
	if (id_stall == 1)
		stall_str = " to_be_stalled";
	else 
		stall_str = "";
	if (flush == 1)
		flush_str = " to_be_flushed";
	else
		flush_str = "";
	if ((if_op == OP_BEQ) || (if_op == OP_BNE)) {
		if (taken_flag == 1)
			taken_str = "taken";
		else
			taken_str = "not_taken";
	}
	else
		taken_str = "n/a";
	if ((id_ex.ctrl.ALUOp == OP_BEQ) || (id_ex.ctrl.ALUOp == OP_BNE)) {
		if (correct == 1) 
			correct_str = "correct";
		else
			correct_str = "incorrect";
	}
	else
		correct_str = "n/a";
	fprintf(fp, "IF: 0x%08X%s%s\n", if_instr, stall_str, flush_str);
	if (branch_forwardA == 0x10) {
		if (branch_forwardB == 0x10)
			fprintf(fp, "ID: %s fwd_EX-DM_rs_$%d fwd_EX-DM_rt_$%d\n", id_name,branch_forwardA_reg,branch_forwardB_reg);
		else
			fprintf(fp, "ID: %s fwd_EX-DM_rs_$%d\n", id_name,branch_forwardA_reg);
	}
	else {
		if (branch_forwardB == 0x10)
			fprintf(fp, "ID: %s fwd_EX-DM_rt_$%d\n", id_name,branch_forwardB_reg);
		else
			fprintf(fp, "ID: %s%s\n", id_name, stall_str);
	}
	if (forwardA == 0x10) {
		if (forwardB == 0x10)
			fprintf(fp, "EX: %s fwd_EX-DM_rs_$%d fwd_EX-DM_rt_$%d\n", ex_name,forwardA_reg,forwardB_reg);
		else if (forwardB == 0x01)
			fprintf(fp, "EX: %s fwd_EX-DM_rs_$%d fwd_DM-WB_rt_$%d\n", ex_name,forwardA_reg,forwardB_reg);
		else
			fprintf(fp, "EX: %s fwd_EX-DM_rs_$%d\n", ex_name,forwardA_reg);
	}
	else if (forwardA == 0x01) {
		if (forwardB == 0x10)
			fprintf(fp, "EX: %s fwd_DM-WB_rs_$%d fwd_EX-DM_rt_$%d\n", ex_name,forwardA_reg,forwardB_reg);
		else if (forwardB == 0x01)
			fprintf(fp, "EX: %s fwd_DM-WB_rs_$%d fwd_DM-WB_rt_$%d\n", ex_name,forwardA_reg,forwardB_reg);
		else
			fprintf(fp, "EX: %s fwd_DM-WB_rs_$%d\n", ex_name,forwardA_reg);
	}
	else {
		if (forwardB == 0x10)
			fprintf(fp, "EX: %s fwd_EX-DM_rt_$%d\n", ex_name,forwardB_reg);
		else if (forwardB == 0x01)
			fprintf(fp, "EX: %s fwd_DM-WB_rt_$%d\n", ex_name,forwardB_reg);
		else
			fprintf(fp, "EX: %s\n", ex_name);
	}
	fprintf(fp, "DM: %s\n", mem_name);
	fprintf(fp, "WB: %s\n", wb_name);
	fprintf(fp, "PREDICTION: %s\n",taken_str);
	fprintf(fp, "CORRECTNESS: %s\n\n\n",correct_str);
	fclose(fp);
}

//main function
int main (void)
{
	FILE *fp;
	int i;
	for (i = 0; i < 32; i++) {
		vcpu.reg[i] = 0;
	}
	loader();
	fp = fopen("snapshot.rpt", "w+");
	fclose(fp);
	init_register();
	while (vcpu.halt_flag == 0) {
		print_reg();	
		id_stall = 0;
		forwardA = 0x00;
		forwardB = 0x00;
		branch_forwardA = 0x00;
		branch_forwardB = 0x00;
		WB();	
		DM();
		EX();
		ID();
		IF();
		branch_state();
		//printf("cycle:%d A:%02X B:%02X\n",cycle,forwardA,forwardB);
		print_stage();
		if ((id_ex.ctrl.jump == 1) && (id_stall == 0)) {
			vcpu.PC = id_ex.jump_PC;
			if_id.nop = 1;
			flush = 1;
		}
		else {
			if ((id_ex.ctrl.branch == 1) && (correct == 0) && (id_stall == 0)) {
				vcpu.PC = id_ex.branch_no_PC;
				flush = 0;	
			}
			else {
				vcpu.PC = if_id.PC;
				flush = 0;
			}
		}	
		real_PC = vcpu.PC * 4;
		cycle++;
	}
	return 0;
}
