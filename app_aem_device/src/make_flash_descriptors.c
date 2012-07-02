#ifdef MAKE_FLASH_DESCRIPTOR_FILE
#include <stdio.h>
#include <xs1.h>
#include "make_flash_descriptors.h"

int make_descriptor_bin_file(void)
{
	#include "aem_descriptors.h" // Can move this global when removed from actual fw
	
	FILE *fp;
	int desc_size_bytes = 0, i = 0;
	unsigned char *descriptor;

	fp = fopen("descriptors.bin", "wb");

	while (1)
	{
		int num_descriptors = aem_descriptor_list[i+1];
		printf("Found %d descriptors\n", num_descriptors);

		for (int j=0, k=2; j < num_descriptors; j++, k += 2)
		{
			printf("%d %d\n", i, k);
			desc_size_bytes = aem_descriptor_list[i+k];
			descriptor = (unsigned char *)aem_descriptor_list[i+k+1];

			fwrite(descriptor, sizeof(unsigned char), desc_size_bytes, fp);
			printf("Wrote %d bytes to output\n", desc_size_bytes);
		}

		i += ((num_descriptors*2)+2);
		if (i >= (sizeof(aem_descriptor_list)/4)) break;
	}

	fclose(fp);

	return 0;
}

int main(void)
{
	printf("Generating flash data file for descriptors...\n");
	make_descriptor_bin_file();

	return 0;
}
#endif