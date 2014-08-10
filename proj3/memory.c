#include "main.h"
#include <math.h>

static enum{MISS, HIT}CHECK;


static u32 combine_data(u32 reserve_data, u32 shift, u32 top_bit, u32 bit_save, u32 op)
{
	if (op == OP_SB) {
		u32 bit_reserve = reserve_data & ((0x00FFFFFF >> shift) + top_bit);
		return bit_save + bit_reserve;
	}
	else if (op == OP_SH) {
		u32 bit_reserve = reserve_data & (0x0000FFFF << shift);
		return bit_save + bit_reserve;
	}
	else if (op == OP_SW) {
		return bit_save;
	}

}

static void check_Idirty_cache2mem(u32 PAddr)
{
	int i,j,k;
	for (i = 0; i < i_page_size>>2; i++) {
		u32 dirty_PAddr = PAddr + (i<<2);
		u32 tag = (dirty_PAddr / i_block_size) / (((i_cache_entry << 2) / i_block_size) / i_cache_way);
		u32 index = ((dirty_PAddr / i_block_size) % (((i_cache_entry << 2) / i_block_size) / i_cache_way));
		int end = ((index * i_cache_way * (i_block_size >> 2)) + (i_cache_way * (i_block_size >> 2)) - 1);
		int start = ((index * i_cache_way * (i_block_size >> 2)) - 1);

	

		for (j = (start+1); j < (end+1); j = j + (i_block_size/4)) {
			if ((i_cache[j].valid == 1) && (i_cache[j].tag == tag)) {
				if (i_cache[j].dirty == 1) {
					for (k = 0; k < i_block_size>>2; k++) {
						i_mem[(dirty_PAddr>>2)+k].data = i_cache[j+k].data;
						i_mem[(dirty_PAddr>>2)+k].dirty = 1;
						i_cache[j+k].dirty = 0;
						i_cache[j+k].valid = 0;
						i_cache[j+k].data = 0x00000000;
					}
				}
				else {
					for (k = 0; k < i_block_size>>2; k++) {
						i_cache[j+k].data = 0x00000000;
						i_cache[j+k].valid = 0;
					}
				}
			}
		}
	}

}

static void check_Idirty_mem2disk(u32 PAddr, u32 offset_bit)
{
	int i,j;
	for (i = 0; i < i_page_size>>2; i++) {
		if (i_mem[(PAddr>>2)+i].dirty == 1) {
			u32 dirty_VA = (i_mem[PAddr>>2].VA_page << offset_bit);
			for (j = 0; j < i_page_size>>2; j++) 
				i_disk[(dirty_VA>>2)+j] = i_mem[(PAddr>>2)+j].data;
			i_mem[(PAddr>>2)+j].dirty = 0;
			break;
		}
	}
}


u32 read_Imemory(u32 VAddr)
{
	int i,j,k;
	int TLB_check = MISS;
	int cache_check = MISS;
	u32 PAddr;
	u32 tag;
	u32 offset;
	u32 offset_bit;
	u32 index;
	//check TLB
	offset = VAddr % i_page_size;
	offset_bit = log2(i_page_size);
	u32 VA_page = VAddr / i_page_size;
	

	for (i = 0; i < i_TLB_entry; i++) {
		if ((VA_page == i_TLB[i].VA_page) && (i_TLB[i].valid == 1)) {
			TLB_check = HIT;
			PAddr = (i_TLB[i].page << offset_bit) + offset;
		
			break;
		}
	}
	if (TLB_check == MISS) {
		printf("TLB miss\n");
		iTLB_miss++;
		//check page table
		if (i_PT[VA_page].valid == 0) {
			iPT_miss++;
			u32 PA_page = i_memQ[0];
			printf("PA_page:%d\n",i_memQ[0]);

			for (i = 0; i < ((i_mem_size/i_page_size)-1); i++) {
				i_memQ[i] = i_memQ[i+1];
			}
			i_memQ[(i_mem_size/i_page_size)-1] = PA_page;
			PAddr = (PA_page << offset_bit);
			if (i_mem[PAddr>>2].valid != 0) {
				check_Idirty_cache2mem(PAddr);
			}
			check_Idirty_mem2disk(PAddr,offset_bit);
	
			if (i_mem[PAddr>>2].valid != 0) {
				u32 last_VApage = i_mem[PAddr>>2].VA_page;
				i_PT[last_VApage].valid = 0;
				for (i = 0; i < i_TLB_entry; i++) {
					if ((last_VApage == i_TLB[i].VA_page) && (i_TLB[i].valid == 1)) {
						i_TLB[i].valid = 0;
						break;
					}
				}
			}

			for (i = 0; i < i_page_size>>2; i++) {
				u32 VA_start = VA_page << offset_bit;
				u32 PA_start = PA_page << offset_bit;
				i_mem[(PA_start>>2)+i].data = i_disk[(VA_start>>2)+i];
				i_mem[(PA_start>>2)+i].VA_page = VA_page;
				i_mem[(PA_start>>2)+i].valid = 1;
			}
			i_PT[VA_page].page = PA_page;
			i_PT[VA_page].valid = 1;
		}
		else {
			iPT_hit++;
		}
		PAddr = (i_PT[VA_page].page << offset_bit) + offset;
		int max = 0;
		u32 change_index;
		for (i = (i_TLB_entry-1); i >= 0; i--) {
			if (i_TLB[i].ref >= max) {
				max = i_TLB[i].ref;
				change_index = i;
			}
		}
		for (i = 0; i < i_TLB_entry; i++) {
			i_TLB[i].ref++;
		}
		i_TLB[change_index].ref = 0;
		i_TLB[change_index].valid = 1;
		i_TLB[change_index].page = i_PT[VA_page].page;
		i_TLB[change_index].VA_page = VA_page;
	}
	else {
		iTLB_hit++;
	}
	//check cache

	tag = (PAddr / i_block_size) / (((i_cache_entry << 2) / i_block_size) / i_cache_way);
	index = ((PAddr / i_block_size) % (((i_cache_entry << 2) / i_block_size) / i_cache_way));
	u32 cache_offset = (PAddr>>2) % (i_block_size / 4);
	u32 cache_shift = log2(i_block_size>>2);
	int end = ((index * i_cache_way * (i_block_size >> 2)) + (i_cache_way * (i_block_size >> 2)) - 1);
	int start = ((index * i_cache_way * (i_block_size >> 2)) - 1);
	for (i = (start+1); i < (end+1); i = i+(i_block_size/4)) {
		if ((i_cache[i].valid == 1) && (i_cache[i].tag == tag)) {
			cache_check = HIT;
			icache_hit++;
			u32 PA_page = i_cache[i].PAddr / i_page_size;
			for (j = 0; j < (i_mem_size/i_page_size); j++) {
				if (i_memQ[j] == PA_page) {
					for (k = j; k < ((i_mem_size/i_page_size)-1); k++) {
						i_memQ[k] = i_memQ[k+1];
				}
					i_memQ[(i_mem_size/i_page_size)-1] = PA_page;
					break;
				}
			}
			return i_cache[i+cache_offset].data;
		}
	}

	if (cache_check == MISS) {
		icache_miss++;

		int max = 0;
		u32 change_index;

		for (i = (end-((i_block_size>>2)-1)); i > start; i = i-(i_block_size/4)) {
			if (i_cache[i].ref >= max) {
				max = i_cache[i].ref;
				change_index = i;	
			}
		}
	
		for (i = (start+1); i < (end+1); i++) {
			i_cache[i].ref++;
		}
		u32 start_PA;
		u32 page_offset;
		if (i_cache[change_index].dirty == 1) {
			u32 dirty_VApage = i_cache[change_index].VA_page;
			u32 PA_page = i_PT[dirty_VApage].page;
			PAddr = i_cache[change_index].PAddr;
			page_offset = (PAddr>>2)%(i_block_size>>2);
			if (page_offset == 0) {
				start_PA = PAddr;
			}
			else {
				start_PA = PAddr - page_offset;
			}
			
			for (i = 0; i < (i_block_size>>2); i++) {
				i_mem[(start_PA>>2)+i].dirty = 1;
				i_mem[(start_PA>>2)+i].data = i_cache[change_index+i].data;
				i_mem[(start_PA>>2)+i].VA_page = i_cache[change_index+i].VA_page;
				i_cache[change_index+i].dirty = 0;
			}
			for (i = 0; i < (i_mem_size/i_page_size); i++) {
				if (i_memQ[i] == PA_page) {
					for (j = i; j < ((i_mem_size/i_page_size)-1); j++) {
						i_memQ[j] = i_memQ[j+1];
					}
					i_memQ[(i_mem_size/i_page_size)-1] = PA_page;
					break;
				}
			}
		}
		u32 PA_page = i_PT[VA_page].page;
		PAddr = (PA_page << offset_bit) + offset;
		page_offset = (PAddr>>2)%(i_block_size>>2);
		if (page_offset == 0) {
			start_PA = PAddr;
		}
		else {
			start_PA = PAddr - page_offset;
		}
		

		for (i = 0; i < (i_block_size>>2); i++) {
			i_cache[change_index+i].ref = 0;
			i_cache[change_index+i].valid = 1;
			i_cache[change_index+i].tag = tag;
			i_cache[change_index+i].VA_page = VA_page;
			i_cache[change_index+i].data = i_mem[(start_PA>>2)+i].data;
			i_cache[change_index+i].PAddr = start_PA + (i<<2);
	
		}
		for (i = 0; i < i_mem_size/i_page_size; i++) {
			if (i_memQ[i] == PA_page) {
				for (j = i; j < ((i_mem_size/i_page_size)-1); j++) {
					i_memQ[j] = i_memQ[j+1];
				}
				i_memQ[(i_mem_size/i_page_size)-1] = PA_page;
				break;
			}
		}

		return i_mem[(start_PA>>2)+cache_offset].data;
	}

}

u32 read_Dmemory(u32 VAddr)
{
	int i,j,k;
	int TLB_check = MISS;
	int cache_check = MISS;
	u32 PAddr;
	u32 tag;
	u32 offset;
	u32 offset_bit;
	u32 index;
	int start,end;
	//check TLB
	offset = VAddr % d_page_size;
	offset_bit = log2(d_page_size);
	u32 VA_page = VAddr / d_page_size;

	for (i = 0; i < d_TLB_entry; i++) {
		if ((VA_page == d_TLB[i].VA_page) && (d_TLB[i].valid == 1)) {
			TLB_check = HIT;
			PAddr = (d_TLB[i].page << offset_bit) + offset;

			break;
		}
	}
	if (TLB_check == MISS) {
		dTLB_miss++;
		//check page table
		if (d_PT[VA_page].valid == 0) {
			dPT_miss++;
			u32 PA_page = d_memQ[0];
		

			for (i = 0; i < ((d_mem_size/d_page_size)-1); i++) {
				d_memQ[i] = d_memQ[i+1];
			}
			d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
	
			PAddr = (PA_page << offset_bit);
			if (d_mem[PAddr>>2].valid != 0) {
	
			for (i = 0; i < d_page_size>>2; i++) {
				u32 dirty_PAddr = PAddr + (i<<2);
				tag = (dirty_PAddr / d_block_size) / (((d_cache_entry << 2) / d_block_size) / d_cache_way);
				index = ((dirty_PAddr / d_block_size) % (((d_cache_entry << 2) / d_block_size) / d_cache_way));
				end = ((index * d_cache_way * (d_block_size >> 2)) + (d_cache_way * (d_block_size >> 2)) - 1);
				start = ((index * d_cache_way * (d_block_size >> 2)) - 1);
		
				for (j = (start+1); j < (end+1); j = j + (d_block_size/4)) {
					if ((d_cache[j].valid == 1) && (d_cache[j].tag == tag)) {
						if (d_cache[j].dirty == 1) {
							for (k = 0; k < (d_block_size>>2); k++) {
								d_mem[(dirty_PAddr>>2)+k].data = d_cache[j+k].data;
								d_mem[(dirty_PAddr>>2)+k].dirty = 1;
								d_cache[j+k].dirty = 0;
								d_cache[j+k].valid = 0;
								d_cache[j+k].data = 0x00000000;
							}
						}
						else {
							for (k = 0; k < (d_block_size>>2); k++) {
								d_cache[j+k].data = 0x00000000;
								d_cache[j+k].valid = 0;
							}
						}
					}
				}

			}
			}
			for (i = 0; i < d_page_size>>2; i++) {
				if (d_mem[(PAddr>>2)+i].dirty == 1) {
					u32 dirty_VA = (d_mem[PAddr>>2].VA_page << offset_bit);
					for (j = 0; j < d_page_size>>2; j++) { 
						d_disk[(dirty_VA>>2)+j] = d_mem[(PAddr>>2)+j].data;
					}
					break;
				}
			}
			if (d_mem[PAddr>>2].valid != 0) {
				u32 last_VApage = d_mem[PAddr>>2].VA_page;
				d_PT[last_VApage].valid = 0;
				for (i = 0; i < d_TLB_entry; i++) {
					if ((last_VApage == d_TLB[i].VA_page) && (d_TLB[i].valid == 1)) {
						d_TLB[i].valid = 0;
						break;
					}
				}
			}
			for (i = 0; i < d_page_size>>2; i++) {
				u32 VA_start = VA_page << offset_bit;
				u32 PA_start = PA_page << offset_bit;
				d_mem[(PA_start>>2)+i].data = d_disk[(VA_start>>2)+i];
				d_mem[(PA_start>>2)+i].VA_page = VA_page;
				d_mem[(PA_start>>2)+i].valid = 1;

			}
			d_PT[VA_page].page = PA_page;
			d_PT[VA_page].valid = 1;
		}
		else {
			dPT_hit++;
		}
		PAddr = (d_PT[VA_page].page << offset_bit) + offset;
		int max = 0;
		u32 change_index;
		for (i = (d_TLB_entry-1); i >= 0; i--) {
			if (d_TLB[i].ref >= max) {
				max = d_TLB[i].ref;
				change_index = i;
			}
		}
		for (i = 0; i < d_TLB_entry; i++) {
			d_TLB[i].ref++;
		}
		d_TLB[change_index].ref = 0;
		d_TLB[change_index].valid = 1;
		d_TLB[change_index].page = d_PT[VA_page].page;
		d_TLB[change_index].VA_page = VA_page;
	}
	else {
		dTLB_hit++;
	}
	//check cache
	tag = (PAddr / d_block_size) / (((d_cache_entry << 2) / d_block_size) / d_cache_way);
	index = ((PAddr / d_block_size) % (((d_cache_entry << 2) / d_block_size) / d_cache_way));
	u32 cache_offset = (PAddr>>2) % (d_block_size / 4);
	u32 cache_shift = log2(d_block_size>>2);
	end = ((index * d_cache_way * (d_block_size >> 2)) + (d_cache_way * (d_block_size >> 2)) - 1);
	start = ((index * d_cache_way * (d_block_size >> 2)) - 1);

	for (i = (start+1); i < (end+1); i = i + (d_block_size/4)) {
		if ((d_cache[i].valid == 1) && (d_cache[i].tag == tag)) {
			cache_check = HIT;
			dcache_hit++;
			u32 PA_page = d_cache[i].PAddr / d_page_size;
			for (j = 0; j < (d_mem_size/d_page_size); j++) {
				if (d_memQ[j] == PA_page) {
					for (k = j; k < ((d_mem_size/d_page_size)-1); k++) {
						d_memQ[k] = d_memQ[k+1];
				}
					d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
					break;
				}
			}
			return d_cache[i+cache_offset].data;
		}
	}

	if (cache_check == MISS) {
		dcache_miss++;
		int max = 0;
		u32 change_index;
	

		for (i = (end-((d_block_size>>2)-1)); i > start; i = i - (d_block_size >> 2)) {
			if (d_cache[i].ref >= max) {
				max = d_cache[i].ref;
				change_index = i;
			}
		}

		for (i = (start+1); i < (end+1); i++) {
		
			d_cache[i].ref++;
		}
		
		u32 start_PA;
		u32 page_offset;
		if (d_cache[change_index].dirty == 1) {
			u32 dirty_VApage = d_cache[change_index].VA_page;
			u32 PA_page = d_PT[dirty_VApage].page;
			PAddr = d_cache[change_index].PAddr;
			page_offset = (PAddr>>2)%(d_block_size>>2);
			if (page_offset == 0) {
				start_PA = PAddr;
			}
			else {
				start_PA = PAddr - page_offset;
			}
	
			for (i = 0; i < (d_block_size>>2); i++) {
				d_mem[(start_PA>>2)+i].dirty = 1;
				d_mem[(start_PA>>2)+i].data = d_cache[change_index+i].data;
				d_mem[(start_PA>>2)+i].VA_page = d_cache[change_index+i].VA_page;
				d_cache[change_index+i].dirty = 0;
			}
			/*for (i = 0; i < (d_mem_size/d_page_size); i++) {
				if (d_memQ[i] == PA_page) {
					for (j = i; j < ((d_mem_size/d_page_size)-1); j++) {
						d_memQ[j] = d_memQ[j+1];
					}
					d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
					break;
				}
			}*/
		}
		u32 PA_page = d_PT[VA_page].page;
		PAddr = (PA_page << offset_bit) + offset;
		page_offset = (PAddr>>2)%(d_block_size>>2);
		if (page_offset == 0) {
			start_PA = PAddr;
		}
		else {
			start_PA = PAddr - page_offset;
		}

		for (i = 0; i < d_block_size>>2; i++) {
			d_cache[change_index+i].ref = 0;
			d_cache[change_index+i].valid = 1;
			d_cache[change_index+i].tag = tag;
			d_cache[change_index+i].VA_page = VA_page;
			d_cache[change_index+i].data = d_mem[(start_PA>>2)+i].data;
			d_cache[change_index+i].PAddr = start_PA + (i<<2);
		}
	
		for (i = 0; i < (d_mem_size/d_page_size); i++) {
			if (d_memQ[i] == PA_page) {
				for (j = i; j < ((d_mem_size/d_page_size)-1); j++) {
					d_memQ[j] = d_memQ[j+1];
				}
				d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
				break;
			}
		}

		return d_mem[(start_PA>>2)+cache_offset].data;
	}

}

void write_Imemory(u32 VAddr, u32 shift, u32 top_bit, u32 bit_save, u32 op)
{

}

void write_Dmemory(u32 VAddr, u32 shift, u32 top_bit, u32 bit_save, u32 op)
{

	int i,j,k;
	int TLB_check = MISS;
	int cache_check = MISS;
	u32 PAddr;
	u32 tag;
	u32 offset;
	u32 offset_bit;
	u32 index;
	int start,end;
	//check TLB
	offset = VAddr % d_page_size;
	
	offset_bit = log2(d_page_size);
	u32 VA_page = VAddr / d_page_size;

	for (i = 0; i < d_TLB_entry; i++) {
		if ((VA_page == d_TLB[i].VA_page) && (d_TLB[i].valid == 1)) {
			TLB_check = HIT;
			PAddr = (d_TLB[i].page << offset_bit) + offset;
			break;
		}
	}
	if (TLB_check == MISS) {

		dTLB_miss++;
		//check page table
		if (d_PT[VA_page].valid == 0) {
			dPT_miss++;
			u32 PA_page = d_memQ[0];
			

			for (i = 0; i < ((d_mem_size/d_page_size)-1); i++) {
				d_memQ[i] = d_memQ[i+1];
			}
			d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
		
			PAddr = (PA_page << offset_bit);
			if (d_mem[PAddr>>2].valid != 0) {
	
			for (i = 0; i < d_page_size>>2; i++) {
				u32 dirty_PAddr = PAddr + (i<<2);
				tag = (dirty_PAddr / d_block_size) / (((d_cache_entry << 2) / d_block_size) / d_cache_way);
				index = ((dirty_PAddr / d_block_size) % (((d_cache_entry << 2) / d_block_size) / d_cache_way));
				end = ((index * d_cache_way * (d_block_size >> 2)) + (d_cache_way * (d_block_size >> 2)) - 1);
				start = ((index * d_cache_way * (d_block_size >> 2)) - 1);
		
				for (j = (start+1); j < (end+1); j = j + (d_block_size/4)) {
					if ((d_cache[j].valid == 1) && (d_cache[j].tag == tag)) {
						if (d_cache[j].dirty == 1) {
							for (k = 0; k < (d_block_size>>2); k++) {
								d_mem[(dirty_PAddr>>2)+k].data = d_cache[j+k].data;
								d_mem[(dirty_PAddr>>2)+k].dirty = 1;
								d_cache[j+k].dirty = 0;
								d_cache[j+k].valid = 0;
								d_cache[j+k].data = 0x00000000;
							}
						}
						else {
							for (k = 0; k < (d_block_size>>2); k++) {
								d_cache[j+k].data = 0x00000000;
								d_cache[j+k].valid = 0;
							}
						}
					}
				}

			}
			}
			for (i = 0; i < d_page_size>>2; i++) {
				if (d_mem[(PAddr>>2)+i].dirty == 1) {
					u32 dirty_VA = (d_mem[PAddr>>2].VA_page << offset_bit);
					for (j = 0; j < d_page_size>>2; j++) { 
						d_disk[(dirty_VA>>2)+j] = d_mem[(PAddr>>2)+j].data;
					}
					break;
				}
			}
		
		
			if (d_mem[PAddr>>2].valid != 0) {
				u32 last_VApage = d_mem[PAddr>>2].VA_page;
				d_PT[last_VApage].valid = 0;
				for (i = 0; i < d_TLB_entry; i++) {
					if ((last_VApage == d_TLB[i].VA_page) && (d_TLB[i].valid == 1)) {
						d_TLB[i].valid = 0;
						break;
					}
				}
			}
			for (i = 0; i < d_page_size>>2; i++) {
				u32 VA_start = VA_page << offset_bit;
				u32 PA_start = PA_page << offset_bit;
				d_mem[(PA_start>>2)+i].data = d_disk[(VA_start>>2)+i];
				d_mem[(PA_start>>2)+i].VA_page = VA_page;
				d_mem[(PA_start>>2)+i].valid = 1;
		
			}
			d_PT[VA_page].page = PA_page;
			d_PT[VA_page].valid = 1;
		}
		else {
			dPT_hit++;
		}
		PAddr = (d_PT[VA_page].page << offset_bit) + offset;
		int max = 0;
		u32 change_index;
		for (i = (d_TLB_entry-1); i >= 0; i--) {
			if (d_TLB[i].ref >= max) {
				max = d_TLB[i].ref;
				change_index = i;
			}
		}

		for (i = 0; i < d_TLB_entry; i++) {
			d_TLB[i].ref++;
		}
		d_TLB[change_index].ref = 0;
		d_TLB[change_index].valid = 1;
		d_TLB[change_index].page = d_PT[VA_page].page;
		d_TLB[change_index].VA_page = VA_page;
	}
	else {
		dTLB_hit++;
	}
	//check cache
	tag = (PAddr / d_block_size) / (((d_cache_entry << 2) / d_block_size) / d_cache_way);
	index = ((PAddr / d_block_size) % (((d_cache_entry << 2) / d_block_size) / d_cache_way));
	u32 cache_offset = (PAddr>>2) % (d_block_size / 4);
	u32 cache_shift = log2(d_block_size);
	end = ((index * d_cache_way * (d_block_size >> 2)) + (d_cache_way * (d_block_size >> 2)) - 1);
	start = ((index * d_cache_way * (d_block_size >> 2)) - 1);


	for (i = (start+1); i < (end+1); i = i + (d_block_size>>2)) {
		if ((d_cache[i].valid == 1) && (d_cache[i].tag == tag)) {
			cache_check = HIT;
			dcache_hit++;
			u32 reserve_data = d_cache[i+cache_offset].data;
			d_cache[i+cache_offset].data = combine_data(reserve_data,shift,top_bit,bit_save,op);
			u32 PA_page = d_cache[i].PAddr / d_page_size;
			for (j = 0; j < (d_mem_size/d_page_size); j++) {
				if (d_memQ[j] == PA_page) {
					for (k = j; k < ((d_mem_size/d_page_size)-1); k++) {
						d_memQ[k] = d_memQ[k+1];
				}
					d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
					break;
				}
			}
			for (j = 0; j < (d_block_size>>2); j++)
				d_cache[i+j].dirty = 1;
			return;
		}
	}

	if (cache_check == MISS) {
		dcache_miss++;
		int max = 0;
		u32 change_index;
	

		for (i = (end-((d_block_size>>2)-1)); i > start; i = i - (d_block_size>>2)) {
			if (d_cache[i].ref >= max) {
				max = d_cache[i].ref;
				change_index = i;
			}
		}
	
		for (i = (start+1); i < (end+1); i++) {
			d_cache[i].ref++;
		}
		u32 start_PA;
		u32 page_offset;
		if (d_cache[change_index].dirty == 1) {
	
			u32 dirty_VApage = d_cache[change_index].VA_page;
			u32 PA_page = d_PT[dirty_VApage].page;
			PAddr = d_cache[change_index].PAddr;
			page_offset = (PAddr>>2)%(d_block_size>>2);
			if (page_offset == 0) {
				start_PA = PAddr;
			}
			else {
				start_PA = PAddr - page_offset;
			}
		
			for (i = 0; i < (d_block_size>>2); i++) {
				d_mem[(start_PA>>2)+i].dirty = 1;
				d_mem[(start_PA>>2)+i].data = d_cache[change_index+i].data;
				d_mem[(start_PA>>2)+i].VA_page = d_cache[change_index+i].VA_page;
				d_cache[change_index+i].dirty = 0;
			}
			/*for (i = 0; i < (d_mem_size/d_page_size); i++) {
				if (d_memQ[i] == PA_page) {
					for (j = i; j < ((d_mem_size/d_page_size)-1); j++) {
						d_memQ[j] = d_memQ[j+1];
					}
					d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
					break;
				}
			}*/
		}
		u32 PA_page = d_PT[VA_page].page;
		PAddr = (PA_page << offset_bit) + offset;
		page_offset = (PAddr>>2)%(d_block_size>>2);
		if (page_offset == 0) {
			start_PA = PAddr;
		}
		else {
			start_PA = PAddr - page_offset;
		}

		for (i = 0; i < d_block_size>>2; i++) {
			d_cache[change_index+i].ref = 0;
			d_cache[change_index+i].valid = 1;
			d_cache[change_index+i].tag = tag;
			d_cache[change_index+i].VA_page = VA_page;
			d_cache[change_index+i].PAddr = start_PA + (i<<2);
		}
		
	
		u32 reserve_data = d_mem[PAddr>>2].data;
		d_mem[PAddr>>2].data = combine_data(reserve_data,shift,top_bit,bit_save,op);
		d_mem[PAddr>>2].dirty = 1;
		for (i = 0; i < d_block_size>>2; i++) {
			d_cache[change_index+i].data = d_mem[(start_PA>>2)+i].data;
		}	
		for (i = 0; i < (d_mem_size/d_page_size); i++) {
			if (d_memQ[i] == PA_page) {
				for (j = i; j < ((d_mem_size/d_page_size)-1); j++) {
					d_memQ[j] = d_memQ[j+1];
				}
				d_memQ[(d_mem_size/d_page_size)-1] = PA_page;
				break;
			}
		}
			
	}
}
