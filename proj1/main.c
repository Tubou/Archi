#include <stdio.h>
#include "main.h"
#include "r_type.h"
#include "i_type.h"
#include "j_type.h"

u32 i_memory[0x100] = {0};
u32 d_memory[0x100] = {0};
u32 instr_max;
u32 data_max;
struct VCPU vcpu;
int cycle = 0;
int check_PC;
int real_PC;

static void print_snapshot(void);
void simu_halt(void)
{
	vcpu.halt_flag = 1;
}

static u32 decoder(u32 instr)
{
	return instr >> 26;
}

void check_data_mis_instr(u32 instr, u32 index_in)
{
	u32 op_code = decoder(instr);
	u32 check = 0;
	int index = (int) index_in;
	check = index % 4;
	if (check != 0) {
		vcpu.err_flag = DATA_MIS;
		cycle++;
		real_PC = index_in;
		err_handler(instr);
		print_snapshot();
	}
	else
		return;
}

void check_data_mis(u32 instr, u32 index_in, int offset)
{
	u32 op_code = decoder(instr);
	u32 check = 0;
	int index = (int) index_in;
	if (op_code == OP_SH || op_code == OP_LH || op_code == OP_LHU) {
		check = (index + offset) % 2;
		if (check != 0) {
			vcpu.err_flag = DATA_MIS;
			err_handler(instr);
		}
		else
			return;
	}
	else if (op_code == OP_SW || op_code == OP_LW || op_code == R_TYPE) {
		check = (index + offset) % 4;
		if (check != 0) {
			vcpu.err_flag = DATA_MIS;
			err_handler(instr);
		}
		else
			return;
	}
}

void check_mem_OF(u32 instr, u32 index_in, int offset)
{
	u32 carry = 0;
	u32 opcode = decoder(instr);
	int index = (int) index_in;
	int check;
	check = index + offset;
	if (check < 0) {
		vcpu.err_flag = MEM_OF;
		err_handler(instr);
		return;
	}
	if (opcode == OP_LW || opcode == OP_SW)
		check = index + offset + 3;
	else if (opcode == OP_LH || opcode == OP_LHU || opcode == OP_SH)
		check = index + offset + 1;
	else if (opcode == OP_LB || opcode == OP_LBU || opcode == OP_SB)
		check = index + offset;
	if (check > 1023 || check < 0) {
		vcpu.err_flag = MEM_OF;
		err_handler(instr);
		return;
	}

}

void check_num_OF_sub(u32 instr, u32 a, u32 b)
{
	u32 carry = 1;
	int i;
	int op_a = (signed int) a;
	int op_b = (signed int) b;
	if (op_a >= 0 && op_b < 0) {
		for (i = 0; i < 31; i++)
			carry = (carry & (((op_a >> i) & 0x1) ^ (~((op_b >> i) & 0x1)))) | (((op_a >> i) & 0x1) & (~((op_b >> i) & 0x1)));
		if (carry == 1) {
			vcpu.err_flag = NUM_OF;
			err_handler(instr);
		}
	}
	else if (op_a < 0 && op_b >= 0) {
		for (i = 0; i < 31; i++)
			carry = (carry & (((op_a >> i) & 0x1) ^ (~((op_b >> i) & 0x1)))) | (((op_a >> i) & 0x1) & (~((op_b >> i) & 0x1)));
		if (carry == 0) {
			vcpu.err_flag = NUM_OF;
			err_handler(instr);
		}
	}
}

void check_num_OF_add(u32 instr, u32 a, u32 b)
{
	u32 carry = 0;
	int op_a = (signed int) a;
	int op_b = (signed int) b;
	int i;
	if (op_a >= 0 && op_b >= 0) {
		for (i = 0; i < 31; i++)
			carry = (carry & (((op_a >> i) & 0x1) ^ ((op_b >> i) & 0x1))) | (((op_a >> i) & 0x1) & ((op_b >> i) & 0x1));
		if (carry == 1) {
			vcpu.err_flag = NUM_OF;
			err_handler(instr);
		}
	}
	else if (op_a < 0 && op_b < 0) {
		for (i = 0; i < 31; i++)
			carry = (carry & (((op_a >> i) & 0x1) ^ ((op_b >> i) & 0x1))) | (((op_a >> i) & 0x1) & ((op_b >> i) & 0x1));
		if (carry == 0) {
			vcpu.err_flag = NUM_OF;
			err_handler(instr);
		}
	}
}

void check_num_OF_subu(u32 instr, u32 op_a, u32 op_b)
{
	int i;
	u32 carry = 0;
	for (i = 0; i < 32; i++)
		carry = (carry & (((op_a >> i) & 0x1) ^  (~((op_b >> i) & 0x1)))) | (((op_a >> i) & 0x1) & (~((op_b >> i) & 0x1)));
	if ((op_a < op_b) && carry == 0) {
		vcpu.err_flag = NUM_OF;
		err_handler(instr);
	}
}

void check_num_OF_addu(u32 instr, u32 op_a, u32 op_b)
{
	int i;
	u32 carry = 0;
	for (i = 0; i < 32; i++)
		carry = (carry & (((op_a >> i) & 0x1) ^ ((op_b >> i) & 0x1))) | (((op_a >> i) & 0x1) & ((op_b >> i) & 0x1));
	if (carry == 1) {
		vcpu.err_flag = NUM_OF;
		err_handler(instr);
	}
}


static void conti(void)
{
}

static void conti_halt(void)
{
	switch (vcpu.err_flag) {
		case WT_REG_0:
			conti();
			break;
		case NUM_OF:
			conti();
			break;
		case MEM_OF:
			simu_halt();
			break;
		case DATA_MIS:
			simu_halt();
			break;
	}
}

static void print_errlog(u32 instr)
{
	FILE *fp;
	fp = fopen("Error_dump.rpt", "a");
	switch (vcpu.err_flag) {
		case WT_REG_0:
			fprintf(fp,"Write $0 error in cycle: %d\n",cycle);	
			break;
		case NUM_OF:
			fprintf(fp,"Number overflow in cycle: %d\n",cycle);
			break;
		case MEM_OF:
			fprintf(fp,"Address overflow in cycle: %d\n",cycle);
			break;
		case DATA_MIS:
			fprintf(fp,"Misalignment error in cycle: %d\n",cycle);
			break;
	}
	fclose(fp);
}

void err_handler(u32 instr)
{
	print_errlog(instr);
	conti_halt();
}

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
	u32 index = vcpu.PC;
	u32 instr = 0;
	int i;
	FILE *fp;
	fp = fopen("Error_dump.rpt", "w+");
	fclose(fp);
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
		if (i >= 0x100) {		//check I-memory access error
			printf("i-memory OF\n");
			vcpu.err_flag = MEM_OF;
			err_handler(instr);
			return;
		}
	}
	instr = combine_instr(fp_d);
	vcpu.reg[29] = instr;
	instr = combine_instr(fp_d);
	data_max = instr;
	for (i = 0; i < data_max; i++) {
		instr = combine_instr(fp_d);
		d_memory[i] = instr;
		if (i >= 0x100) {		//check D-memory access error
			printf("d-memory OF\n");
			vcpu.err_flag = MEM_OF;
			err_handler(instr);
			return;
		}
	}
	fclose(fp_i);
	fclose(fp_d);
}

static void print_snapshot(void)
{
	int i;
	FILE *fp;
	fp = fopen("snapshot.rpt", "a");
	fprintf(fp, "cycle %d\n", cycle);
	for (i = 0; i < 32; i++) {
		fprintf(fp, "$%02d: 0x%08X\n", i, vcpu.reg[i]);
	}
	fprintf(fp, "PC: 0x%08X\n\n\n", real_PC);
	fclose(fp);
}

int main (void)
{
	int i;
	u32 instr;
	u32 op = 0;
	FILE *fp;
	for (i = 0; i < 32; i++) {
		vcpu.reg[i] = 0;
	}
	loader();
	fp = fopen("snapshot.rpt", "w+");
	fclose(fp);
	while (vcpu.halt_flag == 0) {
		print_snapshot();
		if (vcpu.PC > 255 || vcpu.PC < 0) {
			vcpu.err_flag = MEM_OF;
			err_handler(instr);
			break;
		}
		if (check_PC != 0) {
			vcpu.err_flag = DATA_MIS;
			err_handler(instr);
			break;
		}
		instr = i_memory[vcpu.PC];
		vcpu.PC++;
		op = decoder(instr);
		switch(op) {
			case R_TYPE:
				simu_r(instr);
				break;
			case OP_J:
				simu_j(instr);
				break;
			case OP_JAL:
				simu_jal(instr);
				break;
			case OP_BEQ:
				simu_beq(instr);
				break;
			case OP_BNE:
				simu_bne(instr);
				break;
			case OP_ADDI:
				simu_addi(instr);
				break;
			case OP_ADDIU:
				simu_addiu(instr);
				break;
			case OP_SLTI:
				simu_slti(instr);
				break;
			case OP_ANDI:
				simu_andi(instr);
				break;
			case OP_ORI:
				simu_ori(instr);
				break;
			case OP_NORI:
				simu_nori(instr);
				break;
			case OP_LUI:
				simu_lui(instr);
				break;
			case OP_LB:
				simu_lb(instr);
				break;
			case OP_LH:
				simu_lh(instr);
				break;
			case OP_LW:
				simu_lw(instr);
				break;
			case OP_LBU:
				simu_lbu(instr);
				break;
			case OP_LHU:
				simu_lhu(instr);
				break;
			case OP_SB:
				simu_sb(instr);
				break;
			case OP_SH:
				simu_sh(instr);
				break;
			case OP_SW:
				simu_sw(instr);
				break;
			case OP_HALT:
				simu_halt();
				break;
		}
		real_PC = vcpu.PC * 4;
		cycle++;
	}
	return 0;
}
