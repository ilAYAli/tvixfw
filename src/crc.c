/*
    Copyright (C) 2008 Petter Wahlman

    Tvixfw is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Tvixfw is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Tvixfw.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include <tvixfw.h>
#include <tvix_crc.h>

u32 *crctable = (u32*) crcbytes;
int calc_crc(unsigned char *buf, int size, u32 *crc)
{
   u32 v1;
   u32 v0;
   unsigned char t;
   int rc = 1;

   printf("calculating crc...\n");
   do {
      v0 = *crc;
      t = *buf;
      v1 = t;
      v1 = v1 ^ v0;
      v1 = v1 & 0xff;
      v0 = v0 >> 8;
      v1 = crctable[v1];
      v0 = v0 ^v1;
      *crc = v0;
      buf++;
   } while(--size);

   if (!*crc)
      rc = 0;

   printf("   crc: %02x %02x %02x %02x\n",
          (*crc & 0x000000ff),
          (*crc & 0x0000ff00) >> 8,
          (*crc & 0x00ff0000) >> 16,
          (*crc & 0xff000000) >> 24);

   return rc;
}
