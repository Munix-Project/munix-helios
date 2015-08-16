################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../helios-src/bin/init.c \
../helios-src/bin/sh.c 

OBJS += \
./helios-src/bin/init.o \
./helios-src/bin/sh.o 

C_DEPS += \
./helios-src/bin/init.d \
./helios-src/bin/sh.d 


# Each subdirectory must supply rules for building sources it contributes
helios-src/bin/%.o: ../helios-src/bin/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	i686-pc-toaru-gcc -std=c99 -I"/home/miguel/git/munix-helios/helios-hdd/usr/include" -O3 -g3 -Wall -m32 -Wa,--32 -g -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


