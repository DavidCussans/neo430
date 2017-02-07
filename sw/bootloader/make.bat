@REM #################################################################################################
@REM #  < NEO430 bootloader (!) compilation script - Windows Version >                               #
@REM # ********************************************************************************************* #
@REM # Edit the BIN_PATH variable to point to your MSPGCC\bin folder.                                #
@REM # ********************************************************************************************* #
@REM # This file is part of the NEO430 Processor project: http://opencores.org/project,neo430        #
@REM # Copyright 2015-2016, Stephan Nolting: stnolting@gmail.com                                     #
@REM #                                                                                               #
@REM # This source file may be used and distributed without restriction provided that this copyright #
@REM # statement is not removed from the file and that any derivative work contains the original     #
@REM # copyright notice and the associated disclaimer.                                               #
@REM #                                                                                               #
@REM # This source file is free software; you can redistribute it and/or modify it under the terms   #
@REM # of the GNU Lesser General Public License as published by the Free Software Foundation,        #
@REM # either version 3 of the License, or (at your option) any later version.                       #
@REM #                                                                                               #
@REM # This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;      #
@REM # without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     #
@REM # See the GNU Lesser General Public License for more details.                                   #
@REM #                                                                                               #
@REM # You should have received a copy of the GNU Lesser General Public License along with this      #
@REM # source; if not, download it from http://www.gnu.org/licenses/lgpl-3.0.en.html                 #
@REM # ********************************************************************************************* #
@REM #  Stephan Nolting, Hannover, Germany                                               13.01.2017  #
@REM #################################################################################################


@REM ----------------------USER CONFIGURATION----------------------
@REM Path to MSPGCC binaries:
@set BIN_PATH=C:\msp430-gcc\bin
@REM --------------------------------------------------------------

@REM Tools
@set AS=%BIN_PATH%\msp430-elf-as
@set CC=%BIN_PATH%\msp430-elf-gcc
@set LD=%BIN_PATH%\msp430-elf-ld
@set OBJDUMP=%BIN_PATH%\msp430-elf-objdump
@set OBJCOPY=%BIN_PATH%\msp430-elf-objcopy
@set IMAGE_GEN=..\tools\image_gen\image_gen
@set SIZE=%BIN_PATH%\msp430-elf-size

@echo NEO430 BOOTLOADER compilation script

@REM Compiler flags
@set CC_OPTS=-nostartfiles -pipe -fwhole-program -fdata-sections -ffunction-sections -Xlinker --gc-sections -Wl,-static -s -Wall
@set CC_OPTS=%CC_OPTS% -lm -Os -mmcu=msp430f133 -T boot_linker_script.x -minrt -Xassembler --mY -mhwmult=none


@REM Assemble startup code
@%AS% -mmcu=msp430 boot_crt0.asm -mY -o crt0.elf

@REM Compile app sources
@%CC% %CC_OPTS% bootloader.c -o main.elf

@REM Print memory utilization
@echo Memory utilization:
@%SIZE% main.elf

@REM Generate ASM listing
@%OBJDUMP% -D -S -z main.elf > bootloader.s

@REM Extract final executable
@%OBJCOPY% -I elf32-little main.elf -j .image -O binary bootloader.bin

@REM Converter for VHLD memory init image
@%IMAGE_GEN% -bld_img bootloader.bin neo430_bootloader_image.vhd

@REM Install bootloader image
@echo Installing bootloader image to rtl\core\neo430_bootloader_image.vhd
@copy neo430_bootloader_image.vhd ..\..\rtl\core\. > nul

@REM Display file size
@for %%A in (bootloader.bin) do @set fileSize=%%~zA
@echo Image size (bytes):
@echo %fileSize%


@REM Clean up
@del main.elf crt0.elf main.elf bootloader.bin bootloader.asm neo430_bootloader_image.vhd