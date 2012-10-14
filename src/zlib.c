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
#include <string.h>
#include <errno.h>
#include <zlib.h>
#include <tvixfw.h>

int zlib_compress(unsigned char **new_buf, u32 *new_len, unsigned char *buf, u32 offset)
{
   int rc = 1;

   printf("compressing..\n");
   *new_len = (1024 * 1024) * EEPROM_SIZE;
   *new_buf = malloc(*new_len);
   if (!*new_buf) {
      perror("malloc()");
      return rc;
   }

   rc = compress((Byte *)*new_buf, (uLong *)new_len, buf, offset);
   return rc;
}

int zlib_uncompress(unsigned char **unc, u32 *new_len, unsigned char *buf, u32 offset)
{
   int rc = 1;

   printf("uncompressing..\n");
   *new_len = (1024 * 1024) * EEPROM_SIZE;
   *unc = malloc(*new_len);
   if (!*unc) {
      perror("malloc()");
      return rc;
   }


   rc = uncompress((Byte *)*unc, (uLong *)new_len, buf, offset);
   return rc;
}
