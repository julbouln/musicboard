# A name common to all output files (elf, map, hex, bin, lst)
TARGET = musicboard

BOARD = STM32746G_DISCOVERY

#BOARD = MUSICBOARD

ifeq ($(BOARD), MUSICBOARD)
	BSP_BASE   = musicboard
else
	BSP_BASE   = stm32746g_discovery
endif

OCDFLAGS   = -f stm32f7discovery.cfg
GDBFLAGS   =

# MCU family and type in various capitalizations o_O
MCU_FAMILY = stm32f7xx
MCU_LC     = stm32f746xx
MCU_MC     = STM32F746xx
MCU_UC     = STM32F746NGHx

# Your C files from the /src directory
SRCS       = main.c
SRCS      += system_$(MCU_FAMILY).c
SRCS      += stm32f7xx_it.c

# Basic HAL libraries
SRCS      += stm32f7xx_hal_rcc.c stm32f7xx_hal_rcc_ex.c stm32f7xx_hal.c stm32f7xx_hal_cortex.c stm32f7xx_hal_gpio.c \
			 stm32f7xx_hal_pwr_ex.c stm32f7xx_hal_tim.c stm32f7xx_hal_tim_ex.c stm32f7xx_hal_pcd.c stm32f7xx_hal_pcd_ex.c \
			 stm32f7xx_hal_hcd.c stm32f7xx_hal_i2s.c stm32f7xx_hal_qspi.c stm32f7xx_hal_sai.c stm32f7xx_hal_sai_ex.c stm32f7xx_hal_i2c.c \
			 stm32f7xx_ll_usb.c stm32f7xx_hal_dma.c \
			 $(BSP_BASE).c $(BSP_BASE)_audio.c $(BSP_BASE)_qspi.c $(BSP_BASE)_codec.c \
			 qspi_wrapper.c \
			 usbd_core.c usbd_ctlreq.c usbd_ioreq.c \
			 usbd_msc_bot.c usbd_msc_scsi.c usbd_msc.c usbd_msc_data.c \
			 usbd_composite.c \
			 audio_buffer.c \
			 midi.c synth.c \
			 usbd_audio.c usbd_audio_if.c \
			 usbd_midi.c usbd_midi_if.c \
			 usbd_storage.c \
			 usbd_conf.c usbd_desc.c

ifeq ($(SYNTH), FLUIDSYNTH)
	SRCS      += fluid_altsfont.c fluid_conv.c fluid_mod.c fluid_sys.c riff.c \
			 fluid_chan.c fluid_gen.c fluid_rev.c fluid_tuning.c \
			 fluid_chorus.c fluid_list.c fluid_synth.c fluid_voice.c
endif

# Directories
OCD_DIR    = openocd-stm32f7/tcl

HAL_DIR    = src/hal
BSP_DIR    = src/bsp/$(BSP_BASE)
CMSIS_DIR  = src/cmsis
AUDIO_DIR  = src/audio
USB_DIR	   = src/usb

# that's it, no need to change anything below this line!

###############################################################################
# Toolchain

PREFIX     = arm-none-eabi
CC         = $(PREFIX)-gcc
AR         = $(PREFIX)-ar
OBJCOPY    = $(PREFIX)-objcopy
OBJDUMP    = $(PREFIX)-objdump
SIZE       = $(PREFIX)-size
GDB        = $(PREFIX)-gdb

OCD        = openocd-stm32f7/src/openocd

###############################################################################
# Options

# Defines
DEFS       = -D$(MCU_MC) -DUSE_HAL_DRIVER -DUSE_USB_FS -D__FPU_PRESENT -DUSE_$(BOARD)

DEFS      += -DSTM32 -DSTM32F7 -DSTM32F746xx -DSTM32F746NGHx -DSTM32F746G_DISCO

#DEFS      += -DAUDIO_FORMAT_32BITS # 32bits audio
DEFS      += -DFREQ_216 # 216mhz clocking

DEFS      += -DARM_MATH_CM7

# Debug specific definitions for semihosting
DEFS       += -DUSE_DBPRINTF


ifeq ($(SYNTH), FLUIDSYNTH)
DEFS 	  += -DFLUID_NO_NAMES -DFLUID_SAMPLE_MMAP -DFLUID_NO_NRPN_EXT -DFLUID_CALC_FORMAT_FLOAT -DFLUID_BUFFER_S16 -DFLUID_FIXED_POINT 
else
DEFS      += -DTSF_SYNTH
endif

# Include search paths (-I)
INCS       = -Isrc
INCS      += -I$(HAL_DIR)
INCS      += -I$(BSP_DIR)
INCS      += -I$(CMSIS_DIR)
INCS      += -I$(AUDIO_DIR)
INCS      += -I$(USB_DIR)
INCS      += -Isrc/efluidsynth

# Library search paths
LIBS       = -L$(CMSIS_DIR)

# Compiler flags
CFLAGS     = -Wall -g -std=c99 -O3

CFLAGS += -mcpu=cortex-m7 -mlittle-endian
CFLAGS += -mfloat-abi=hard -mfpu=fpv5-sp-d16
CFLAGS += -mthumb 
CFLAGS += -ftree-vectorize -ffast-math -fsingle-precision-constant
CFLAGS += -fno-schedule-insns -fno-schedule-insns2 # gcc do something wrong with inline assembly
#CFLAGS += -fconserve-stack -mno-sched-prolog

CFLAGS    += -ffunction-sections -fdata-sections
CFLAGS    += $(INCS) $(DEFS)

LDFLAGS    = -Wl,--gc-sections -Wl,-Map=$(TARGET).map $(LIBS) -T$(MCU_LC).ld

# Enable Semihosting
LDFLAGS   += --specs=rdimon.specs -lc -lrdimon 
#-ldsplib_lm7f

# Source search paths
VPATH      = ./src
VPATH     += $(BSP_DIR)
VPATH     += $(HAL_DIR)
VPATH     += $(AUDIO_DIR)
VPATH     += $(USB_DIR)
VPATH     += src/efluidsynth
OBJS       = $(addprefix obj/,$(SRCS:.c=.o))
DEPS       = $(addprefix dep/,$(SRCS:.c=.d))

# Prettify output
V = 0
ifeq ($V, 0)
	Q = @
	P = > /dev/null
endif

###################################################

.PHONY: all dirs program debug template clean

all: $(TARGET).elf

-include $(DEPS)

dirs: dep obj
dep obj src:
	@echo "[MKDIR]   $@"
	$Qmkdir -p $@

obj/%.o : %.c | dirs
	$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF dep/$(*F).d

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) src/startup_$(MCU_LC).s $^ -o $@
	$(OBJCOPY) $(TARGET).elf -O ihex $(TARGET).hex 
	$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin
	$(OBJDUMP) -M reg-names-std -sSt $(TARGET).elf >$(TARGET).lst
	@echo "[SIZE]    $(TARGET).elf"
	$(SIZE) $(TARGET).elf

build_openocd:
	tools/install_openocd.sh

openocd:
	$(OCD) -s $(OCD_DIR) $(OCDFLAGS)

program: all
	$(OCD) -s $(OCD_DIR) $(OCDFLAGS) -c "program $(TARGET).elf verify reset exit"

flash: all
	sudo dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D musicboard.bin

debug:
	@if ! nc -z localhost 3333; then \
		echo "\n\t[Error] OpenOCD is not running! Start it with: 'make openocd'\n"; exit 1; \
	else \
		$(GDB)  -ex "target extended localhost:3333" \
			-ex "monitor arm semihosting enable" \
			-ex "monitor reset halt" \
			-ex "load" \
			-ex "monitor reset init" \
			$(GDBFLAGS) $(TARGET).elf; \
	fi

clean:
	@echo "[RM]      $(TARGET).elf"; rm -f $(TARGET).elf
	@echo "[RM]      $(TARGET).bin"; rm -f $(TARGET).bin
	@echo "[RM]      $(TARGET).hex"; rm -f $(TARGET).hex
	@echo "[RM]      $(TARGET).map"; rm -f $(TARGET).map
	@echo "[RM]      $(TARGET).lst"; rm -f $(TARGET).lst
	@echo "[RMDIR]   dep"          ; rm -fr dep
	@echo "[RMDIR]   obj"          ; rm -fr obj

