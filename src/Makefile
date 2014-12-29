PROGRAM := nt133
VERSION_MAJOR := 0x00
VERSION_MINOR := 0x01

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS += \
	-O2 \
	-g3 \
	-MD \
	-std=gnu99 \
	-Wall \
	-Wextra \
	-Wno-pointer-sign \
	-Wno-char-subscripts\
	-Wno-sign-compare \
	-DVERSION_MAJOR=$(VERSION_MAJOR) \
	-DVERSION_MINOR=$(VERSION_MINOR) \
	-I../libopencm3/include \
	-I.

LDFLAGS += \
	-lc \
	-lnosys \
	-nostartfiles \
	-Wl,-Map=mapfile \
	-Wl,-gc-sections \
	-l$(LIB_OPENCM32_OBJECT) \
	-L../libopencm3/lib \

# uncomment the next two lines to use the USB_DFU bootloader
CFLAGS += -DBOOTLOADER
LDFLAGS += -Wl,-Ttext=0x8002000 -Wl,--defsym,_stack=0x20005000

# uncomment if debugging code should be included
#CFLAGS += -DDEBUG

Q := @
# Program rules
SUB_DIR = \
	platform/$(PLATFORM) \
	usb \
	i2c_led \
	io \
	nt_timer

include $(addsuffix /Makefile, $(SUB_DIR))

SRC += \
	main.c

OBJ = $(SRC:.c=.o)

# Uncomment the next line to generate assembly from the objects
#DISASM += $(SRC:.c=.dis)

all: $(PROGRAM).bin $(PROGRAM).hex $(DISASM)

gdb: $(PROGRAM).elf
	cgdb -d arm-cortexm3-eabi-gdb $^

$(PROGRAM).bin: $(PROGRAM).elf
	@printf '  %s  -->  %s\n' $^ $@
	$(Q)$(OBJCOPY) -O binary $^ $@

$(PROGRAM).hex: $(PROGRAM).elf
	@printf '  %s  -->  %s\n' $^ $@
	$(Q)$(OBJCOPY) -O ihex $^ $@

$(PROGRAM).elf: $(OBJ)
	@printf '\n'
	@printf '  %s  -->  %s\n' "$^" "$@"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

%.dis: %.o
	@printf '  %s  -->  %s\n' "$^" "$@"
	$(Q)$(OBJDUMP) -l -S $^ > $@

clean:
	-$(Q)$(RM) $(addsuffix /*.o, $(SUB_DIR) .)
	-$(Q)$(RM) $(addsuffix /*.dis, $(SUB_DIR) .)
	-$(Q)$(RM) $(addsuffix /*.d, $(SUB_DIR) .)
	-$(Q)$(RM) $(addsuffix /*.~, $(SUB_DIR) .)
	-$(Q)$(RM) $(addsuffix /mapfile, $(SUB_DIR) .)
	-$(Q)$(RM) $(PROGRAM).elf $(PROGRAM).bin $(PROGRAM).hex

.PHONY:	all clean gdb

-include *.d
