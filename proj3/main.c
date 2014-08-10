#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "r_type.h"
#include "i_type.h"
#include "j_type.h"




u32 i_mem_size = 64;
u32 d_mem_size = 32;

u32 i_disk[DISK_SIZE>>2];
u32 d_disk[DISK_SIZE>>2];
BITS *i_mem;
BITS *d_mem;
u32 *i_memQ;
u32 *d_memQ;

u32 i_page_size = 8;
u32 d_page_size = 16;
u32 i_cache_size = 16;
u32 d_cache_size = 16;
int i_cache_way = 4;
int d_cache_way = 1;
u32 i_block_size = 4;
u32 d_block_size = 4;
u32 i_cache_entry;
u32 d_cache_entry;
u32 i_PT_entry;
u32 d_PT_entry;
u32 i_TLB_entry;
u32 d_TLB_entry;

BITS *i_cache;
BITS *d_cache;
BITS *i_PT;
BITS *d_PT;
BITS *i_TLB;
BITS *d_TLB;

int iTLB_miss = 0;
int dTLB_miss = 0;
int iTLB_hit = 0;
int dTLB_hit = 0;
int iPT_miss = 0;
int dPT_miss = 0;
int iPT_hit = 0;
int dPT_hit = 0;
int icache_miss = 0;
int dcache_miss = 0;
int icache_hit = 0;
int dcache_hit = 0;

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
		i_disk[i]=instr;
		
	}
	instr = combine_instr(fp_d);
	vcpu.reg[29] = instr;
	instr = combine_instr(fp_d);
	data_max = instr;
	for (i = 0; i < data_max; i++) {
		instr = combine_instr(fp_d);
		d_disk[i] = instr;
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

static void print_report(void)
{
	FILE *fp_rep;
	fp_rep = fopen("report.rpt","w+");
	fprintf(fp_rep,"ICache :\n");
	fprintf(fp_rep,"# hits: %u\n",icache_hit);
	fprintf(fp_rep,"# misses: %u\n\n",icache_miss);

	fprintf(fp_rep,"DCache :\n");
	fprintf(fp_rep,"# hits: %u\n",dcache_hit);
	fprintf(fp_rep,"# misses: %u\n\n",dcache_miss);

	fprintf(fp_rep,"ITLB :\n");
	fprintf(fp_rep,"# hits: %u\n",iTLB_hit);
	fprintf(fp_rep,"# misses: %u\n\n",iTLB_miss);

	fprintf(fp_rep,"DTLB :\n");
	fprintf(fp_rep,"# hits: %u\n",dTLB_hit);
	fprintf(fp_rep,"# misses: %u\n\n",dTLB_miss);

	fprintf(fp_rep,"IPageTable :\n");
	fprintf(fp_rep,"# hits: %u\n",iPT_hit);
	fprintf(fp_rep,"# misses: %u\n\n",iPT_miss);

	fprintf(fp_rep,"DPageTable :\n");
	fprintf(fp_rep,"# hits: %u\n",dPT_hit);
	fprintf(fp_rep,"# misses: %u\n\n",dPT_miss);

	fclose(fp_rep);
}

void init_reg(int argc, char *argv[])
{
	int i;
	if (argc > 1) {
		i_mem_size = atoi(argv[1]);
		d_mem_size = atoi(argv[2]);
		i_page_size = atoi(argv[3]);
		d_page_size = atoi(argv[4]);
		i_cache_size = atoi(argv[5]);
		i_block_size = atoi(argv[6]);
		i_cache_way = atoi(argv[7]);
		d_cache_size = atoi(argv[8]);
		d_block_size = atoi(argv[9]);
		d_cache_way = atoi(argv[10]);
	}
	i_cache_entry = i_cache_size >> 2;
	d_cache_entry = d_cache_size >> 2;
	i_PT_entry = DISK_SIZE / i_page_size;
	d_PT_entry = DISK_SIZE / d_page_size;
	i_TLB_entry = i_PT_entry >> 2;
	d_TLB_entry = d_PT_entry >> 2;
	
	for (i = 0; i < 32; i++) {
		vcpu.reg[i] = 0;
	}
	i_mem = (BITS*)malloc(sizeof(BITS) * (i_mem_size >> 2));
	d_mem = (BITS*)malloc(sizeof(BITS) * (d_mem_size >> 2));
	i_memQ = (u32*)malloc(sizeof(u32) * (i_mem_size/i_page_size));
	d_memQ = (u32*)malloc(sizeof(u32) * (d_mem_size/d_page_size));
	i_cache = (BITS*)malloc(sizeof(BITS) * i_cache_entry);
	d_cache = (BITS*)malloc(sizeof(BITS) * d_cache_entry);
	i_PT = (BITS*)malloc(sizeof(BITS) * i_PT_entry);
	d_PT = (BITS*)malloc(sizeof(BITS) * d_PT_entry);
	i_TLB = (BITS*)malloc(sizeof(BITS) * i_TLB_entry);
	d_TLB = (BITS*)malloc(sizeof(BITS) * d_TLB_entry);
	for (i = 0; i < (i_mem_size/i_page_size); i++) {
		i_memQ[i] = i;	
	}
	for (i = 0; i < (d_mem_size/d_page_size); i++) {
		d_memQ[i] = i;
	}
}

void print_buf(void)
{
	int i;
	printf("cycle:%d\n",cycle);
	for (i = 0; i < (i_mem_size>>2); i++) {
		printf("i_mem[%d]:%08X\n",i,i_mem[i].data);
	}
	for (i = 0; i < i_cache_entry; i++) {
		printf("i_cache[%d]:%08X\n",i,i_cache[i].data);
	}
	//for (i = 0; i < (d_mem_size>>2); i++) {
	//	printf("d_mem[%d]:%08X\n",i,d_mem[i].data);
	//}
	//for (i = 0; i < d_cache_entry; i++) {
	//	printf("d_cache[%d]:%08X\n",i,d_cache[i].data);
	//}
	//for (i = 0; i < i_TLB_entry; i++) {
	//	printf("i_TLB[%d]:%08X\n",i,i_TLB[i].VA_page);
	//}
	//for (i = 0; i < d_TLB_entry; i++) {
	//	printf("d_TLB[%d]:%08X\n",i,d_TLB[i].VA_page);
	//}
	//for (i = 0; i < DISK_SIZE>>2; i++) {
	//	printf("d_disk[%d]:%08X\n",i,d_disk[i]);
	//}
}

int main (int argc, char *argv[])
{

	u32 instr;
	u32 op = 0;
	FILE *fp;

	init_reg(argc,argv);
	
	loader();
	fp = fopen("snapshot.rpt", "w+");
	fclose(fp);
	while (vcpu.halt_flag == 0) {
		print_snapshot();
	
		instr = read_Imemory(vcpu.PC<<2);
		printf("PC:%d instr:%08X\n",vcpu.PC,instr);
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
		/*debug print*/
		print_buf();
	}
	print_report();
	return 0;
}
