#!/bin/bash

TOOLCHAIN=./munix_helios_toolchain
SRCPATH=helios-src
DSTPATH=/home/miguel/git/toolchain/munix-hdd
LIBSPATH=$TOOLCHAIN/$SRCPATH/usr/lib

DISKFILE=munix_helios_toolchain/helios.img
DEVTABLE=munix_helios_toolchain/util/devtable
MOUNTLOC=/mnt/helios
SECTOR_COUNT=131072
SECTOR_SIZE=4096
HDDDIR=$DSTPATH
GENEXT=genext2fs
OSNAME="Helios"

# Allow choice between: creating entire disk from scratch from the folders 
# OR: mount disk already created and copy the files THERE (and also the HDD folder)

# Mount the disk now if any parameter was passed on
if [[ $# -ne 0 ]]; then
	if [[ -f $DISKFILE ]]; then
		printf "Mounting and updating disk...\n"
		sudo rm -rf $MOUNTLOC
		sudo mkdir $MOUNTLOC
		sudo mount -o loop $DISKFILE $MOUNTLOC
	else
		echo "Disk not found"
	fi
fi

# Strip the extension from all object files and copy the files to the HDD
FILELIST=$(find $TOOLCHAIN -path $LIBSPATH -prune -o -name *.o | rename -v -f 's@\.o@@g' | grep -o -E " \./.+?")

# Loop through both FILELIST and FILELSTDST and use their index like an array
for ARG in $(echo $FILELIST); do
	ISEXECUTABLE=$(expr "$(i686-munix-nm `echo $ARG` -g | grep " main")" : '.*')
	
	if [ "$ISEXECUTABLE" -gt 0 ]; then
		echo $ARG - Installed
		FILELISTDST=$(echo $ARG | sed "s@$TOOLCHAIN/$SRCPATH@$DSTPATH@g")
		cp -f $ARG $FILELISTDST
		# Also copy to mounted disk IF CHOICE SAYS SO:
		if [[ $# -ne 0 && -f $DISKFILE ]]; then
			MOUNTFILEDST=$(echo $ARG | sed "s@$TOOLCHAIN/$SRCPATH@$MOUNTLOC@g")
			sudo cp -f $ARG $MOUNTFILEDST
		fi
	fi
done

# Now make the disk (HDD) image:
if [[ $# -eq 0 || ! -f $DISKFILE ]]; then
	BYTES_IN_GB=1073741824.0
	TOTAL=$(expr $SECTOR_SIZE \* $SECTOR_COUNT)
	TOTAL_GB=$(echo "scale=3;$TOTAL/$BYTES_IN_GB" | bc)

	printf "Generating $OSNAME disk image... (sector: size = $SECTOR_SIZE, count = $SECTOR_COUNT, total = $TOTAL bytes, ~$TOTAL_GB GB)\n"
	rm -f $DISKFILE
	$GENEXT -B $SECTOR_SIZE -d $HDDDIR -D $DEVTABLE -U -b $SECTOR_COUNT -N $SECTOR_SIZE $DISKFILE
else
	# The disk wasn't created, just mounted and updated the changed files
	sudo umount $MOUNTLOC
	sudo rm -rf $MOUNTLOC
fi

