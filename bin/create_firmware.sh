#! /bin/bash
# create_firmware.sh by BadEIP.
#
# The script will create a directory in the current working directory named mnt, where one of the
# JFFS2 filesystems contained in the firmware will be mounted.
# This directory can then be manually- or automatically patched. 
# To perform an automatic creation of the firmware, put a directory named root_6500 in your $PATH
# 
# usage:
# create_firmware.sh /path/to/firmware.fwp
#
# This script only supports the Tvix M-6500 platform, as firmware for all other Tvix-architectures
# can easily be generated manually with TvixFW

FIRMWARE=$1
TVIXFW=`which tvixfw`
OPENTVIX_SRC="./root_6500"
SUDO=sudo
MTD="/dev/mtdblock0"
MNT_DIR="./mnt"
JFFS2_MNT="./mnt/jffs2"
ROMFS_MNT="./mnt/romfs"
DUMP_DIR=`basename $FIRMWARE.dump`
JFFS2_FILE="$DUMP_DIR/jffs2.apps"
ROMFS_FILE="$DUMP_DIR/romfs.rootfs"


about()
{
   clear
   echo "================================================================================="
   echo " This is as a supplement to TvixFW (http://wiki.opentvix.com/Tvixfw) to ease the"
   echo " creation of Tvix M-6500 firmware."
   echo "================================================================================="
   echo
}

check_args()
{
   if [ -z $FIRMWARE ]; then
      echo "usage: $0 /path/to/firmware.fwp"
      exit 1
   fi
   echo "Source firmware: $FIRMWARE"
}

locate_tvixfw()
{
   if [ -z $TVIXFW ]; then
      if [ -f ./bin/tvixfw ]; then
         TVIXFW=./bin/tvixfw
      else
         echo "Error, unable to locate the tvixfw binary in your \$PATH"
         exit 1
      fi
   fi
}

init()
{
   if [ "$(id -u)" == "0" ]; then
      unset SUDO
   else
      echo -n "Sudo "
   fi

   $SUDO rm -rvf $MNT_DIR 2>&1 >/dev/null
   mkdir -p $JFFS2_MNT
   mkdir -p $ROMFS_MNT
   $SUDO umount $MNT_DIR 2>/dev/null
   $SUDO umount $JFFS2_MNT 2>/dev/null
   $SUDO umount $ROMFS_MNT 2>/dev/null
   echo
}

load_modules()
{
   $SUDO modprobe mtdcore
   $SUDO modprobe jffs2
   $SUDO modprobe mtdram total_size=65536 erase_size=512
   $SUDO modprobe mtdchar
   $SUDO modprobe mtdblock
}

dump_filesystems()
{
   $TVIXFW --dump $FIRMWARE 2>/dev/null
}

# not needed on the 65k.
extract_romfs_image()
{
   $SUDO mount -o loop -t romfs $ROMFS_FILE $ROMFS_MNT
   cp -vp $ROMFS_MNT/xrpc_xload_vmlinux_ES4_prod.bin .
   $SUDO umount $ROMFS_MNT
   mv ./xrpc_xload_vmlinux_ES4_prod.bin $ROMFS_MNT
   echo "romfs image extracted"
}

extract_jffs2_apps()
{
   $SUDO dd if=/dev/zero bs=10240 count=6536 of=$MTD 2>/dev/null &&
   $SUDO dd if=$JFFS2_FILE of=$MTD 2>/dev/null
   $SUDO mount -t jffs2 $MTD $JFFS2_MNT || exit 1
   echo "jffs2 image mounted"

}

modify_jffs2_apps()
{
   echo "you can now modify the contents of the $MNT_DIR directory."
   echo "-- press enter to continue --"
   read foo

   echo "generating jffs2 image.."
   $SUDO mkfs.jffs2 --eraseblock=$((0x2000)) --pagesize=4096 --root=$JFFS2_MNT -o $JFFS2_FILE
   $SUDO umount $JFFS2_MNT
}

create_firmware()
{
   $TVIXFW --create $DUMP_DIR/fwp.header
}

main()
{
   about
   check_args
   locate_tvixfw
   init
   load_modules
   dump_filesystems
   extract_jffs2_apps
   modify_jffs2_apps
   create_firmware
}

main
