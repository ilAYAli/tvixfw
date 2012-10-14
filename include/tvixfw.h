#ifndef TVIXFW_H
#define TVIXFW_H

#define TVIXFW_VERSION     "0.4"
#define MAX_FILESYSTEMS    9
#define EEPROM_SIZE        64

#define REVERSE_INT(i) (((i&0xff)<<24)+(((i>>8)&0xff)<<16)+(((i>>16)&0xff)<<8)+((i>>24)&0xff))
#define ALIGN_DATA(size) ((size + 1023) & ~(1024-1))

#define u32 unsigned int

extern int decrypt(unsigned char *, int);
extern int encrypt(unsigned char *, int );
extern int calc_crc(unsigned char *, int , u32 *);
extern int zlib_compress(unsigned char **, u32 *, unsigned char *, u32);
extern int zlib_uncompress(unsigned char **, u32 *, unsigned char *, u32);

#endif
