#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ihexread.h"

#define STR_SIZE 1024



int ihex_read( FILE * file, uint8_t * buffer, uint16_t start, uint16_t size)
{
	// start: start of buffer in the address space


	// :CC AAAA TT DD..DD SS
	// CC   -- byte count
	// AAAA -- start address
	// TT   -- type
	// DD..DD -- data
	// SS   -- checksum (complete sum is zero)

	// types: 00 -- data
	//        01 -- end of file
	//    others -- error


	char str[STR_SIZE];
	char * ptr;

	int num_bytes;
	int address;
	int type;
	int byte;
	int checksum;

	int i;

	int error = 0;
	int eof = 0;


	while( !eof && !error && fgets(str,STR_SIZE,file) )
	{
		ptr = str;

		// check for ':'
		if( (*ptr)!=':' )
		{
			error=1;
			break;
		}
		ptr++;

		num_bytes = get_hexbyte(ptr);
		if( num_bytes<0 )
		{
			error=1;
			break;
		}
		ptr+=2;
//printf("num_bytes = %x\n",num_bytes);

		address = get_hexbyte(ptr)<<8;
		if( address<0 )
		{
			error=1;
			break;
		}
		ptr+=2;
		address |= get_hexbyte(ptr);
		if( address<0 )
		{
			error=1;
			break;
		}
		ptr+=2;

		type = get_hexbyte(ptr);
		if( type<0 )
		{
			error=1;
			break;
		}
		ptr+=2;

		checksum = num_bytes + (address>>8) + (address&255) + type;
		
		if( type==0 )
		{
			for(i=0;(i<num_bytes && !error);i++)
			{
				byte = get_hexbyte(ptr);
				ptr+=2;
				if( byte<0 )
				{
					error=1;
					break;
				}

				if( start<=address && address<(start+size) )
				{
					buffer[address-start] = (uint8_t)byte;
					address++;
					checksum += byte;
				}
				else
				{
					error=3;
					break;
				}
			}

			if( error )
				break;

			// last byte is checksum
			byte = get_hexbyte(ptr);
			if( byte<0 )
			{
				error=1;
				break;
			}

			checksum += byte;

			if( checksum&255 )
			{
				error = 4;
				break;
			}
		}
		else if( type==1 )
		{
			eof=1;
			break;
		}
		else
		{
			error=2;
			break;
		}
	}


	// errors: 1 - ihex parse error, 2 - unsupported type, 3 - range error, 4 - checksum error
	if( error==1 )
	{
		printf("%s(): intelhex parse error! %s\n",__func__,str);
	}
	else if( error==2 )
	{
		printf("%s(): intelhex unsupported type! %s\n",__func__,str);
	}
	else if( error==3 )
	{
		printf("%s(): intelhex range error! %s\n",__func__,str);
	}
	else if( error==4 )
	{
		printf("%s(): intelhex checksum error! %s\n",__func__,str);
	}


	return error;
}

int get_hexbyte( char * ptr )
{ // returns -1 on error, otherwise byte as positive value

	char c1=0,c2=0;
	int val;


	if( !ptr )
		return -1;

	if( *ptr )
		c1=toupper(*ptr);
	else
		return -1;

	ptr++;

	if( *ptr )
		c2=toupper(*ptr);
	else
		return -1;

	if( (('0'<=c1 && c1<='9') || ('A'<=c1 && c1<='F')) &&
	    (('0'<=c2 && c2<='9') || ('A'<=c2 && c2<='F')) )
	{
		val  = (c1>'9') ? (c1-'A'+0x0A) : (c1-'0');
		val <<= 4;
		val += (c2>'9') ? (c2-'A'+0x0A) : (c2-'0');

		return val;
	}

	return -1;
}

