#!/bin/bash

EMU=qemu-system-i386
EMUAPPEND="logtoserial=1 start=--vga root=/dev/hda"
MEMSIZE=1024
KERNELPATH=/home/miguel/git/munix/munix_toolchain
KERNEL=munix-kernel
DISKFILE=munix_helios_toolchain/helios.img

# Sort out dependencies in the proper order:
PR=/home/miguel/git/munix/munix_toolchain/modules
MODS="serial,procfs,tmpfs,ata,ext2,debug_shell,mouse,kbd,lfbvideo,packetfs,snd,pcspkr,ac97,net,rtl8139,irc"
MODS=$(echo $MODS | sed "s@ @@g") # Remove empty spaces
MODS=$(echo $MODS | sed "s@,@.ko,$PR/@g") # Replace commas with the prefix and sufixes
MODS="$PR/$MODS.ko" # Fix it up by adding the last two pieces

printf "==========\nLaunching QEMU i386 . . .\n==========\n"
$EMU -initrd $MODS -hda $DISKFILE -sdl -kernel $KERNELPATH/$KERNEL -serial stdio -vga std -rtc base=localtime -net nic,model=rtl8139 -net user -soundhw pcspk,ac97 -net dump -no-kvm-irqchip -m $MEMSIZE -append "$EMUAPPEND"
