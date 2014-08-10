#ifndef H_MAIN
#define H_MAIN


#define DISK_SIZE 0x400


typedef unsigned int u32;

typedef struct {
	u32 data;
	u32 VA_page;
	u32 page;
	u32 tag;
	u32 valid;
	u32 ref;
	u32 dirty;
	u32 PAddr;
}BITS;


extern u32 i_mem_size;
extern u32 d_mem_size;

extern u32 i_page_size;
extern u32 d_page_size;
extern u32 i_cache_size;
extern u32 d_cache_size;

extern u32 i_disk[DISK_SIZE>>2];
extern u32 d_disk[DISK_SIZE>>2];
extern BITS *i_mem;
extern BITS *d_mem;
extern u32 *i_memQ;
extern u32 *d_memQ;

extern int i_cache_way;
extern int d_cache_way;
extern u32 i_block_size;
extern u32 d_block_size;
extern u32 i_cache_entry;
extern u32 d_cache_entry;
extern u32 i_PT_entry;
extern u32 d_PT_entry;
extern u32 i_TLB_entry;
extern u32 d_TLB_entry;

extern BITS *i_cache;
extern BITS *d_cache;
extern BITS *i_PT;
extern BITS *d_PT;
extern BITS *i_TLB;
extern BITS *d_TLB;

extern u32 instr_max;
extern u32 data_max;

extern int iTLB_miss;
extern int dTLB_miss;
extern int iTLB_hit;
extern int dTLB_hit;
extern int iPT_miss;
extern int dPT_miss;
extern int iPT_hit;
extern int dPT_hit;
extern int icache_miss;
extern int dcache_miss;
extern int icache_hit;
extern int dcache_hit;

struct VCPU {
	u32 PC;
	u32 reg[32];
	u32 halt_flag;
};

extern struct VCPU vcpu;

extern int cycle;


#define R_TYPE 0x00
#define OP_J 0x02
#define OP_JAL 0x03
#define OP_BEQ 0x04
#define OP_BNE 0x05
#define OP_ADDI 0x08
#define OP_ADDIU 0x09
#define OP_SLTI 0x0A
#define OP_ANDI 0x0C
#define OP_ORI 0x0D
#define OP_NORI 0x0E
#define OP_LUI 0x0F
#define OP_LB 0x20
#define OP_LH 0x21
#define OP_LW 0x23
#define OP_LBU 0x24
#define OP_LHU 0x25
#define OP_SB 0x28
#define OP_SH 0x29
#define OP_SW 0x2B
#define OP_HALT 0x3F




void simu_halt(void);
void err_handler(u32 instr);
void check_num_OF_subu(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_addu(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_sub(u32 instr, u32 op_a, u32 op_b);
void check_num_OF_add(u32 instr, u32 op_a, u32 op_b);
void check_mem_OF(u32 instr, u32 index, int offset);
void check_data_mis(u32 instr, u32 index, int offset);
void check_data_mis_instr(u32 instr, u32 index);

#endif
