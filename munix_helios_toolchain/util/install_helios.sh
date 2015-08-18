#!/bin/bash

TOOLCHAIN=./munix_helios_toolchain
SRCPATH=helios-src
DSTPATH=./helios-hdd

DISKFILE=munix_helios_toolchain/helios.img
DEVTABLE=munix_helios_toolchain/util/devtable
SECTOR_COUNT=131072
SECTOR_SIZE=4096
HDDDIR=helios-hdd
GENEXT=genext2fs
OSNAME="Helios"

# Strip the extension from all object files and copy the files to the HDD
FILELIST=$(find $TOOLCHAIN -name *.o | rename -v -f 's@\.o@@g' | grep -o -E " \./.+?")

# Loop through both FILELIST and FILELSTDST and use their index like an array
for ARG in $(echo $FILELIST); do
	FILELISTDST=$(echo $ARG | sed "s@$TOOLCHAIN/$SRCPATH@$DSTPATH@g")
	cp -f $ARG $FILELISTDST
	#TODO: Also copy to mounted disk IF CHOICE SAYS SO
done

# TODO: Allow choice between: creating entire disk from scratch from the folders 
# OR: mount disk already created and copy the files THERE (and also the HDD folder)

# Now make the disk (HDD) image:
printf "Generating $OSNAME disk image... (sector: size = $SECTOR_SIZE, count = $SECTOR_COUNT, total = )"
rm -f $DISKFILE
$GENEXT -B $SECTOR_SIZE -d $HDDDIR -D $DEVTABLE -U -b $SECTOR_COUNT -N $SECTOR_SIZE $DISKFILE

