#include <stdio.h>
#include <stdlib.h>

extern unsigned char avb_1722_1_sec_constant_data[];
extern unsigned int avb_1722_1_sec_data_type_length_t[];
extern unsigned avb_1722_1_sec_dispatch(unsigned func_num, unsigned item_number, char* data);
extern unsigned int avb_1722_1_sec_parse_tree[];

unsigned int* avb_1722_1_walk_tree(unsigned int address)
{
	unsigned int *node = &avb_1722_1_sec_parse_tree[0];
	unsigned int done = 0;

	unsigned int sec = (address & 0x3C000000) >> 26;
	unsigned int sub = (address & 0x03800000) >> 23;
	unsigned int add = (address & 0x003F0000) >> 16;
	unsigned int itm = (address & 0x0000FFFF) >> 0;

	// Hunt until we have a pointer to a child node
	do
	{
		unsigned int match=0;
		unsigned int match_start = (*node & 0x003F0000) >> 16;
		unsigned int match_end   = match_start + ((*node & 0x0FC00000) >> 22);
		unsigned int match_val = 0;

		switch (*node & 0x30000000) {
		case 0x00000000: match_val = sec; break;
		case 0x10000000: match_val = sub; break;
		case 0x20000000: match_val = add; break;
		case 0x30000000: match_val = itm; break;
		}

		if (match_val >= match_start && match_val < match_end) {
			if ((*node & 0x80000000) != 0) done = 1;
			node += (match_val - match_start + 1);
		} else if ((*node & 0x40000000) == 0) {
			node = &avb_1722_1_sec_parse_tree[*node & 0x0000FFFF];
		} else {
			return 0;
		}
	} while  (done == 0);

	return node;
}

