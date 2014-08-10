#ifndef H_MEMORY
#define H_MEMORY

u32 read_Imemory(u32 VAddr);
u32 read_Dmemory(u32 VAddr);
void write_Imemory(u32 VAddr, u32 shift, u32 top_bit, u32 bit_save, u32 op);
void write_Dmemory(u32 VAddr, u32 shift, u32 top_bit, u32 bit_save, u32 op);

#endif 
