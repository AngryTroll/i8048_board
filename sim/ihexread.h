#ifndef IHEXREAD_H
#define IHEXREAD_H


int ihex_read( FILE * file, uint8_t * buffer, uint16_t start, uint16_t size );

int get_hexbyte( char * ptr );



#endif // IHEXREAD_H

