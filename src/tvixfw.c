/*
   Copyright (C) 2008 Petter Wahlman, aka BadEIP

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

   usage:
      tvixfw --dump     /path/to/firmware.fwp
      tvixfw --create   /path/to/fwp.header
   
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <tvixfw.h>

#define DECRYPTED_FILE  "fwp.decrypted"
#define HEADER_FILE     "fwp.header"

#define OPT_DUMP        1
#define OPT_CREATE      2

typedef struct fs_info {
   u32 avail;
   u32 data;
   u32 size;
} __attribute__((packed)) fs_info_t;

typedef struct tvix_header {
   unsigned char unknown[0xc]; // string
   unsigned char version;
   unsigned char version_major;
   unsigned char version_minor;
   unsigned char unkown1[0x9];
   unsigned char model[16];
   u32 unknown2;
   u32 rel_date;
   fs_info_t fs[MAX_FILESYSTEMS];
   u32 file_len;
} __attribute__((packed)) tvix_header_t;

char *fs_name[] = {
   "disabled0",
   "disabled1",
   "disabled2",
   "romfs.rootfs",
   "disabled3",
   "jffs2.apps",
   "jffs2.fonts",
   "disabled4",
   "disabled5",
   "disabled6"
};

static int dump_data(char *filename, void *data, u32 size)
{
   int fd, n;
   int rc = 1;

   fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   if (-1 == fd) {
      perror(filename);
      return rc;
   }

   n = write(fd, data, size);
   if (n == size)
      rc = 0;
   close(fd);

   return rc;
}

static int dump_filesystems(unsigned char *unc, tvix_header_t *th)
{
   int i, fdd[MAX_FILESYSTEMS];
   int rc = 1;

   memset(fdd, 0, sizeof(fdd));
   for (i = 0; i < MAX_FILESYSTEMS; i++) {
      if (!th->fs[i].avail)
         continue;
      //("fs[%d] writing 0x%08lx (%lu) bytes\n", i, th->fs[i].size, th->fs[i].size);
      rc |= fdd[i] = dump_data(fs_name[i], unc + th->fs[i].data, th->fs[i].size);
   }

   return rc;  
 }

static void print_header(tvix_header_t *th)
{
   int i;
   struct tm *t;

   printf("%s\n", th->model);
   t = gmtime((time_t *)&th->rel_date);
   printf("%u.%u.%u (%d/%d-%d %.2d:%.2d)\n", 
         th->version, th->version_major, th->version_minor,
         t->tm_mday, t->tm_mon +1, t->tm_year + 1900,
         t->tm_hour, t->tm_min);/* rather verbose, but I tend to build often */
   printf("firmware size: %u\n", th->file_len);
   
   for (i = 0; i < MAX_FILESYSTEMS; i++) {
      if (th->fs[i].avail) {
         printf("fs[%d] data: 0x%08x size: 0x%08x name: \"%s\"\n", 
            i, th->fs[i].data, th->fs[i].size, fs_name[i]);
      }
   }
   putchar('\n');

   return;
}

void foo(void)
{
   printf("foo\n");
}

// --dump
static int opt_dump(int fd)
{
   unsigned char *decrypted;
   int rc = 1;

   do {
      tvix_header_t th;
      unsigned char *unc;
      u32 unc_size = 0;
      struct stat st;

      if (fstat(fd, &st)) {
         fprintf(stderr, "fstat: could not get size assosiated with fd: %d\n", fd);
         return rc;
      }
   
      decrypted = malloc(st.st_size + 1024);
      if (!decrypted) {
         perror("malloc");
         return rc;
      }

      read(fd, decrypted, st.st_size);
      memcpy(&th, decrypted, sizeof(th));
      print_header(&th);

      if ((rc = dump_data(HEADER_FILE, &th, sizeof(th))))
         break;

      decrypt(decrypted + sizeof(th), st.st_size - sizeof(th));
      if ((rc = zlib_uncompress(&unc, &unc_size, decrypted + sizeof(th), st.st_size - sizeof(th)))) {
         fprintf(stderr, "error, zlib_uncompress: %d\n", rc);
         break;
      }

      if ((rc = dump_data(DECRYPTED_FILE, unc, unc_size)))
         break;

      if ((rc = dump_filesystems(unc, &th)))
         break;
   } while(0);

   free(decrypted);

   return rc;
}

// --create
static int opt_create(int fd)
{
   unsigned char *decrypted;
   unsigned char *encrypted;
   u32 crc = 0;
   char firmware_file[64];
   tvix_header_t th;
   struct dirent *dp;
   struct stat st;
   u32 enc_len;
   int i, n, fdd, fdf;
   DIR *dirp;
   int rc = 1;

   read(fd, &th, sizeof(th));
   print_header(&th);

   fdd = open(DECRYPTED_FILE, O_RDWR);
   if (-1 == fdd) {
      fprintf(stderr, "error, unable to locate %s.\n", DECRYPTED_FILE);
      return rc;
   }

   fstat(fdd, &st);
   decrypted = malloc(st.st_size);
   if (!decrypted) {
      perror("malloc");
      return rc;
   }

   // sigh...
   read(fdd, decrypted, st.st_size);
   close(fdd);

   if ((dirp = opendir(".")) == NULL) {
      perror("error, couldn't open '.'");
      return rc;
   }

   while ((dp = readdir(dirp))) {
      for (i = 0; i < MAX_FILESYSTEMS; i++) {
         if (!th.fs[i].avail)
            continue;
         if (!strcmp(dp->d_name, fs_name[i])) {
            u32 align;
            struct stat tst;
            int tfd;

            printf("injecting: %s\n", fs_name[i]);
            tfd = open(dp->d_name, O_RDONLY);
            if (-1 == tfd) {
               perror(dp->d_name);
               break;
            }

            fstat(tfd, &tst);
            // st.st_size = fucked
            n = read(tfd, &decrypted[th.fs[i].data], tst.st_size);
            close(tfd);

            align = ALIGN_DATA(tst.st_size);
            printf("- original size:     0x%08x\n"
                   "- modified size:     0x%08x %s\n"
                   "- new size (aligned) 0x%08x\n",
               th.fs[i].size, 
               (u32)tst.st_size, 
               th.fs[i].size != tst.st_size ? "[size changed]" : "", align); 

            if (align != th.fs[i].size) 
               th.fs[i].size = align;
            //printf("- %d bytes written\n\n", n);
         }
      }
   }
   closedir(dirp);

   rc = zlib_compress(&encrypted, &enc_len, decrypted, st.st_size);
   if (rc) {
      fprintf(stderr, "error, zlib_compress: %d\n", rc);
      return rc;
   }

   rc = encrypt(encrypted, enc_len);
   if (rc) {
      fprintf(stderr, "error, encrypt: %d\n", rc);
      return rc;
   }

   th.rel_date = time(NULL);
   th.file_len = sizeof(th) + enc_len + 4;
   printf("new firmware size: %u\n", th.file_len);

   calc_crc((unsigned char *)&th, sizeof(th), &crc);
   calc_crc(encrypted, enc_len, &crc);

   snprintf(firmware_file, sizeof(firmware_file), "BadEIP_M-6x00_%d.%d.%d.fwp", th.version, th.version_major, th.version_minor);
   fdf = open(firmware_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   if (-1 == fdf) {
      perror(firmware_file);
      return rc;
   }
   
   write(fdf, &th, sizeof(th));
   write(fdf, encrypted, enc_len);
   write(fdf, &crc, sizeof(crc));
   close(fdf);

   if (!rc)
      printf("\n\"firmware created: %s\n", firmware_file);

   return rc;
}

static void print_usage(void)
{
   printf("usage: tvixfw [option]\n"
                  "--dump   firmware.fwp   dump contents of \"firmware.fwp\"\n"
                  "--create firmware.fwp   create firmware from dumped \"firmware.fwp\"\n"
                  "\n"
   );
   exit(1);
}

int main(int argc, char **argv)
{
   char *source;
   char dir[256];
   int opt, fd;
   int rc;

   printf("tvixfw v.%s. (c) Petter Wahlman, aka BadEIP\n\n", TVIXFW_VERSION);
   opt = 0;
   while (1) {
      int c;
      int option_ndex = 0;
      static struct option long_ptions[] = {
         {"dump",   0, 0, 'd' },
         {"create", 0, 0, 'c' },
         {0,        0, 0,  0  }
      };

      c = getopt_long(argc,  argv, "dc", long_ptions, &option_ndex);
      if (-1 == c)
         break;
      switch (c) {
         case 'd':
            opt = OPT_DUMP;
            break;
         case 'c':
            opt = OPT_CREATE;
            break;
         default:
            print_usage();
      }
   }
   if (!opt)
      print_usage();

   source = argv[optind++];
   if (!source) {
      fprintf(stderr, "error, no source file specified\n");
      return 1;
   }

   if (opt == OPT_DUMP) {
      sprintf(dir, "%s.dump", basename(source));
      mkdir(dir, 0755);
   } else {
      if (!strstr(source, "fwp.header")) {
         fprintf(stderr, "error, source must refer to fwp.header\n");
         return 1;
      }
      strcpy(dir, source);
      *(char *)(strrchr(dir, '/')) = '\0';
   }

   fd = open(source, O_RDONLY);
   if (-1 == fd) {
      perror(source);
      return 1;
   }

   chdir(dir);
   if (opt == OPT_DUMP)
      rc = opt_dump(fd);
   else
      rc = opt_create(fd);
   close(fd);

   printf("data saved in: \"./%s/\"\n", dir);

   return rc;
}
