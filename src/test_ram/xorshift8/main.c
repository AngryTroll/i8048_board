#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const uint64_t SZ8G=0x200000000ULL;


#include "xorshift.c"





int main(int argc, char ** argv)
{
	uint8_t * mem = (uint8_t *)malloc(SZ8G);
	if( !mem )
	{
		printf("cant allocate 8G!\n");
		exit(1);
	}

	size_t i,ii;

	size_t n=40;


	//xorshift8(0x0100);
	xorshift8(0x01);

	printf("generating...\n");

	ii=0;
	for(i=0;i<SZ8G;i++)
	{
		mem[i] = xorshift8(0);

		//if( (i-ii)>99999999 )
		//{
		//	printf("%f%%\n",100.0*((double)i)/((double)SZ8G));
		//	ii=i;
		//}

		if( argc>2 )
		{
			printf("%02X",mem[i]);
			ii++;
			if( ii==n )
			{
				ii=0;
				printf("\n");
			}
		}
	}


	printf("checking period #1...\n");

	// find period by comparing 4 bytes
	uint32_t initial = * (uint32_t *) mem;
	size_t pos=1;
	while( initial != *(uint32_t *)(&mem[pos]) )
		pos++;

	if( pos!=0xFFFFFFFFULL )
	{
			printf("period is not 2^32-1!\n");
			exit(1);
	}


	printf("checking period #2...\n");

	for(i=0;i<0xFFFFFFFFULL;i++)
	{
		if( mem[i]!=mem[i+0xFFFFFFFFULL] )
		{
			printf("period is not 2^32-1!\n");
			exit(1);
		}
	}

	printf("Period is 2^32-1!\n");


	
	if( argc<=1 )
	{
		FILE * dump;

		dump=fopen("dump","wb");
		if( dump )
		{
			if( 1!=fwrite(mem,SZ8G,1,dump) )
			{
				printf("cant write to file!\n");
				exit(1);
			}

			fclose(dump);
		}
		else
		{
			printf("cant open file!\n");
			exit(1);
		}
	}

	free(mem);
	
	
	return 0;
}

