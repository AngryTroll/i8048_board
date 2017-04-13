#include <stdint.h>


// xorshift8(0) -- no seed init
// non-zero -- init seed
inline uint8_t xorshift8(uint32_t seed_init)
#if 0
{
	static uint32_t seed;

	uint32_t a;
	uint32_t hl,de;

	if( seed_init )
		seed = seed_init;

	
	hl = seed>>16;
	de = seed&65535;


	seed >>= 16;


	hl ^= 255 & (hl<<3);

	hl = (hl&255) | ((de^((de<<1)&0xFE00UL))&0xFF00ULL);

	hl = ((hl^(hl>>8)^(hl>>9))&255) | (de<<8);


	seed |= (hl<<16)&0xFFFF0000UL;

}
#endif
#if 0
{
	static uint32_t seed;
	
	uint8_t h,l,d,e;

	if( seed_init )
	{
		seed=seed_init;
	}


	h=seed>>24;
	l=seed>>16;
	d=seed>>8;
	e=seed;

	l = l ^ (l<<3);
	h = d ^ (d<<1);
	l = l ^ h ^ (h>>1);
	h = e;

	seed >>= 16;
	seed |= (h<<24) | (l<<16);

	return l;
}
#endif
#if 1
{
	static uint8_t h,l,d,e;
	uint8_t hh,ll;

	if( seed_init )
	{
		h=seed_init>>24;
		l=seed_init>>16;
		d=seed_init>>8;
		e=seed_init;
	}

	ll = l ^ (l<<3);
	hh = d ^ (d<<1);
	ll = ll ^ hh ^ (hh>>1);
	hh = e;

	//seed >>= 16;
	//seed |= (h<<24) | (l<<16);

	d=h;e=l;
	h=hh;l=ll;

	return l;
}
#endif


