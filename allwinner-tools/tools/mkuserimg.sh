#!/bin/bash
#
# To call this script, make sure make_ext4fs is somewhere in PATH

function usage() {
cat<<EOT
Usage:
mkuserimg.sh SRC_DIR OUTPUT_FILE EXT_VARIANT MOUNT_POINT SIZE
EOT
}

echo "in mkuserimg.sh PATH=$PATH"

if [ $# -ne 4 -a $# -ne 5 ]; then
  usage
  exit 1
fi

SRC_DIR=$1
if [ ! -d $SRC_DIR ]; then
  echo "Can not find directory $SRC_DIR!"
  exit 2
fi

OUTPUT_FILE=$2
EXT_VARIANT=$3
MOUNT_POINT=$4
SIZE=$5

case $EXT_VARIANT in
  ext4) ;;
  *) echo "Only ext4 is supported!"; exit 3 ;;
esac

if [ -z $MOUNT_POINT ]; then
  echo "Mount point is required"
  exit 2
fi

if [ -z $SIZE ]; then
    SIZE=128M
fi

echo "make_ext4fs -l $SIZE -a $MOUNT_POINT $OUTPUT_FILE $SRC_DIR"
make_ext4fs -s -l $SIZE -a $MOUNT_POINT $OUTPUT_FILE $SRC_DIR
if [ $? -ne 0 ]; then
  exit 4
fi
