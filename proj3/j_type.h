#ifndef H_J_TYPE
#define H_J_TYPE

struct op_j {
	u32 C;
};


void simu_j(u32 instr);
void simu_jal(u32 instr);

#endif 
