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

int encrypt(unsigned char *buf, int size)
{
   unsigned char *ptr = buf;
   u32 offset = 0;
   u32 val;

   if (size < 1)
      return 1;

   printf("encrypting..\n");
   for (offset = 0; offset < size; offset++) {
      *ptr = *buf;
      val = (u32)(*ptr) + 0xffff;
      val &= 0xff;
      val = val < 0xfe ? 1 : 0;

      if ( !(!val || (*ptr == 0x5a) || (*ptr == 0xa5)) ) {
         val = (*ptr) ^ 0x5a;
         *buf = val;
      }

      val = *ptr;
      val ^= 0x88;
      *ptr = val;

      buf++;
      ptr++;
   }

   return 0;
}

int decrypt(unsigned char *buf, int size)
{
   unsigned char *ptr = buf;
   u32 offset = 0;
   u32 val;

   if (size < 1)
      return 1;

   printf("decrypting..\n");
   for (offset = 0; offset < size; offset++) {
      val = *ptr;
      val ^= 0x88;
      *ptr = val;

      val = (u32)(*ptr) + 0xffff;
      val &= 0xff;
      val = val < 0xfe ? 1 : 0;

      if ( !(!val || (*ptr == 0x5a) || (*ptr == 0xa5)) ) {
         val = (*ptr) ^ 0x5a;
         *buf = val;
      }

      buf++;
      ptr++;
   }

   return 0;
}
