################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../helios-src/bin/cat.c \
../helios-src/bin/init.c \
../helios-src/bin/login.c \
../helios-src/bin/sh.c \
../helios-src/bin/terminal.c \
../helios-src/bin/uname.c 

OBJS += \
./helios-src/bin/cat.o \
./helios-src/bin/init.o \
./helios-src/bin/login.o \
./helios-src/bin/sh.o \
./helios-src/bin/terminal.o \
./helios-src/bin/uname.o 

C_DEPS += \
./helios-src/bin/cat.d \
./helios-src/bin/init.d \
./helios-src/bin/login.d \
./helios-src/bin/sh.d \
./helios-src/bin/terminal.d \
./helios-src/bin/uname.d 


# Each subdirectory must supply rules for building sources it contributes
helios-src/bin/%.o: ../helios-src/bin/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/init.o: ../helios-src/bin/init.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/init.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/login.o: ../helios-src/bin/login.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/security/helios_auth.o -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/login.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/sh.o: ../helios-src/bin/sh.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/kbd.o /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/rline.o /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/list.o -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/sh.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

helios-src/bin/terminal.o: ../helios-src/bin/terminal.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -I"/home/miguel/git/munix-helios/helios-src/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/kbd.o /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/graphics/graphics.o /home/miguel/git/munix-helios/munix_helios_toolchain/helios-src/usr/lib/pthread.o -MMD -MP -MF"$(@:%.o=%.d)" -MT"helios-src/bin/terminal.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


