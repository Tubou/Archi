#include "main.h"
#include "j_type.h"


void decode_j(u32 instr)
{
	u32 op = decoder(instr);
	id_ex.jump_PC = (((4 * if_id.PC) & 0xF0000000) | ((4 * instr) & 0x3FFFFFF)) >> 2;
	control.jump = 1;
	control.MemRead = 0;
	control.MemWrite = 0;
	control.RegWrite = 0;
	control.ALUOp = op;
}


