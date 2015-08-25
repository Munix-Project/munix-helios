################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../helios-src/bin/cat.c \
../helios-src/bin/chmod.c \
../helios-src/bin/chown.c \
../helios-src/bin/clear.c \
../helios-src/bin/cp.c \
../helios-src/bin/cpudet.c \
../helios-src/bin/echo.c \
../helios-src/bin/env.c \
../helios-src/bin/find.c \
../helios-src/bin/free.c \
../helios-src/bin/hostname.c \
../helios-src/bin/init.c \
../helios-src/bin/kill.c \
../helios-src/bin/ln.c \
../helios-src/bin/login.c \
../helios-src/bin/ls.c \
../helios-src/bin/mkdir.c \
../helios-src/bin/mount.c \
../helios-src/bin/mv.c \
../helios-src/bin/nohup.c \
../helios-src/bin/pkill.c \
../helios-src/bin/ps.c \
../helios-src/bin/pstree.c \
../helios-src/bin/readlink.c \
../helios-src/bin/reboot.c \
../helios-src/bin/rename.c \
../helios-src/bin/rm.c \
../helios-src/bin/sayhello.c \
../helios-src/bin/sh.c \
../helios-src/bin/shutdown.c \
../helios-src/bin/sleep.c \
../helios-src/bin/spawn.c \
../helios-src/bin/stat.c \
../helios-src/bin/sudo.c \
../helios-src/bin/syscall.c \
../helios-src/bin/terminal.c \
../helios-src/bin/touch.c \
../helios-src/bin/uname.c \
../helios-src/bin/uptime.c \
../helios-src/bin/user.c \
../helios-src/bin/which.c \
../helios-src/bin/whoami.c 

OBJS += \
./helios-src/bin/cat.o \
./helios-src/bin/chmod.o \
./helios-src/bin/chown.o \
./helios-src/bin/clear.o \
./helios-src/bin/cp.o \
./helios-src/bin/cpudet.o \
./helios-src/bin/echo.o \
./helios-src/bin/env.o \
./helios-src/bin/find.o \
./helios-src/bin/free.o \
./helios-src/bin/hostname.o \
./helios-src/bin/init.o \
./helios-src/bin/kill.o \
./helios-src/bin/ln.o \
./helios-src/bin/login.o \
./helios-src/bin/ls.o \
./helios-src/bin/mkdir.o \
./helios-src/bin/mount.o \
./helios-src/bin/mv.o \
./helios-src/bin/nohup.o \
./helios-src/bin/pkill.o \
./helios-src/bin/ps.o \
./helios-src/bin/pstree.o \
./helios-src/bin/readlink.o \
./helios-src/bin/reboot.o \
./helios-src/bin/rename.o \
./helios-src/bin/rm.o \
./helios-src/bin/sayhello.o \
./helios-src/bin/sh.o \
./helios-src/bin/shutdown.o \
./helios-src/bin/sleep.o \
./helios-src/bin/spawn.o \
./helios-src/bin/stat.o \
./helios-src/bin/sudo.o \
./helios-src/bin/syscall.o \
./helios-src/bin/terminal.o \
./helios-src/bin/touch.o \
./helios-src/bin/uname.o \
./helios-src/bin/uptime.o \
./helios-src/bin/user.o \
./helios-src/bin/which.o \
./helios-src/bin/whoami.o 

C_DEPS += \
./helios-src/bin/cat.d \
./helios-src/bin/chmod.d \
./helios-src/bin/chown.d \
./helios-src/bin/clear.d \
./helios-src/bin/cp.d \
./helios-src/bin/cpudet.d \
./helios-src/bin/echo.d \
./helios-src/bin/env.d \
./helios-src/bin/find.d \
./helios-src/bin/free.d \
./helios-src/bin/hostname.d \
./helios-src/bin/init.d \
./helios-src/bin/kill.d \
./helios-src/bin/ln.d \
./helios-src/bin/login.d \
./helios-src/bin/ls.d \
./helios-src/bin/mkdir.d \
./helios-src/bin/mount.d \
./helios-src/bin/mv.d \
./helios-src/bin/nohup.d \
./helios-src/bin/pkill.d \
./helios-src/bin/ps.d \
./helios-src/bin/pstree.d \
./helios-src/bin/readlink.d \
./helios-src/bin/reboot.d \
./helios-src/bin/rename.d \
./helios-src/bin/rm.d \
./helios-src/bin/sayhello.d \
./helios-src/bin/sh.d \
./helios-src/bin/shutdown.d \
./helios-src/bin/sleep.d \
./helios-src/bin/spawn.d \
./helios-src/bin/stat.d \
./helios-src/bin/sudo.d \
./helios-src/bin/syscall.d \
./helios-src/bin/terminal.d \
./helios-src/bin/touch.d \
./helios-src/bin/uname.d \
./helios-src/bin/uptime.d \
./helios-src/bin/user.d \
./helios-src/bin/which.d \
./helios-src/bin/whoami.d 


# Each subdirectory must supply rules for building sources it contributes
helios-src/bin/%.o: ../helios-src/bin/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -U__STRICT_ANSI__ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/init.o: ../helios-src/bin/init.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -U__STRICT_ANSI__ -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/init.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/login.o: ../helios-src/bin/login.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/crypt/sha2 /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/helios_auth -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/login.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/ls.o: ../helios-src/bin/ls.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g "/home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/list" -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/ls.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/ps.o: ../helios-src/bin/ps.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g "/home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/list" -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/ps.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/pstree.o: ../helios-src/bin/pstree.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/tree /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/list -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/pstree.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/sh.o: ../helios-src/bin/sh.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O0 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/kbd /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/rline /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/list -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/sh.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/sudo.o: ../helios-src/bin/sudo.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/crypt/sha2 /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/helios_auth -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/sudo.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/terminal.o: ../helios-src/bin/terminal.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/kbd /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/graphics/graphics /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/terminal.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/user.o: ../helios-src/bin/user.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -U__STRICT_ANSI__ /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/helios_auth /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/crypt/sha2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/user.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


