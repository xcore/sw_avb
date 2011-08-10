#include <string.h>
#include "avb_1722_1_aecp.h"

// Data structures and functions from the generated file
extern unsigned char avb_1722_1_sec_constant_data[];
extern unsigned int avb_1722_1_sec_data_type_length_t[];
extern unsigned avb_1722_1_sec_dispatch(unsigned func_num, unsigned char *address, unsigned set, unsigned char *data);
extern unsigned int avb_1722_1_sec_parse_tree[];

// Callbacks from the generated function table
unsigned int avb_1722_1_getset_actuation_time(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_device_guid(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_boot_id(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_device_name(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_device_wink(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_device_mac_addr(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_device_ptp_gm(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_source_name(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_source_stream_name(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_sink_name(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}

unsigned int avb_1722_1_getset_sink_stream_name(unsigned char *address, unsigned set, unsigned char *data)
{
	return 0;
}


// Function for parsing a 1722.1 SEC address
unsigned int avb_1722_1_walk_tree(unsigned char *address, unsigned set, unsigned char *data)
{
	unsigned int *node = &avb_1722_1_sec_parse_tree[0];
	unsigned int done = 0;

	// Hunt until we have a pointer to a child node
	do
	{
		unsigned int match_start = (*node & 0x003F0000) >> 16;
		unsigned int match_end   = match_start + ((*node & 0x0FC00000) >> 22);
		unsigned int match_val = 0;

		switch (*node & 0x30000000) {
		case 0x00000000: match_val = ((((address[4]<<8) | address[5]) & 0x1C) >> 6); break;
		case 0x10000000: match_val = ((address[5] & 0x38) >> 3); break;
		case 0x20000000: match_val = (address[5] & 0x7); break;
		case 0x30000000: match_val = (((address[10])<<8) | address[11]); break;
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

	if (*node) {
		if ((*node & 0x80000000) != 0) {
			// Functional leaf
			if (set != 0 && (*node & 0x40000000) == 0) {
				return 0;
			} else {
				return avb_1722_1_sec_dispatch((*node & 0x00FFFFFF), address, set, data);
			}
		} else {
		    // Data leaf
			char* param = (char*)((*node & 0x00FFFFFF));
			unsigned int length = avb_1722_1_sec_data_type_length_t[*node & 0x3F000000];

			if (length == 0) {
				length = strlen(param)+1;
			}

			memcpy(data, param, length);
			return length;
		}
	} else {
	    return 0;
	}
}

