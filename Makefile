NAME = blinky

SOURCE_DIR = src
INCLUDE_DIRS = $(SOURCE_DIR)
LIBRARIES = 

CC = xtensa-lx106-elf-gcc

CPPFLAGS +=

CFLAGS += -std=c11
CFLAGS += -Wall -Werror
CFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-function
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -mlongcalls -DICACHE_FLASH

LDLIBS += -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lcirom -Wl,--end-group -lgcc
LDFLAGS += -Teagle.app.v6.ld

ESPTOOL_BAUD ?= 115200
ESPTOOL_PORT ?= /dev/ttyACM0
ESPTOOL_FLAGS ?= --before no_reset --after soft_reset

ESPTOOL_FLAGS += --port $(ESPTOOL_PORT) --baud $(ESPTOOL_BAUD)

# ---

SOURCES = $(wildcard $(SOURCE_DIR)/*.c) $(wildcard $(SOURCE_DIR)/driver/*.c)
OBJECTS = ${SOURCES:.c=.o}

CPPFLAGS += $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(LIBRARIES),-l$(library))

# ---

.PHONY: all flash clean 

all: $(NAME)

# --

$(NAME): $(OBJECTS)
	$(LINK.c) $(OBJECTS) $(LDLIBS) -o $(NAME)

# --

$(NAME)-0x00000.bin: $(NAME)
	esptool.py elf2image $<
	
flash: $(NAME)-0x00000.bin
	esptool.py $(ESPTOOL_FLAGS) \
		write_flash \
		0x00000 $(NAME)-0x00000.bin 0x10000 $(NAME)-0x10000.bin

flash-reset: $(NAME)-0x00000.bin
	esptool.py $(ESPTOOL_FLAGS) \
		write_flash \
		0x00000 $(NAME)-0x00000.bin 0x10000 $(NAME)-0x10000.bin \
		0xFC000 esp_init_data_default.bin 0xFE000 blank.bin
		
run:
	esptool.py $(ESPTOOL_FLAGS) run

term:
	miniterm.py -p $(ESPTOOL_PORT) -b 115200
	
# --

clean:
	$(RM) $(SOURCE_DIR)/*.o
	$(RM) $(SOURCE_DIR)/driver/*.o
	$(RM) $(NAME)
	$(RM) $(NAME)-0x00000.bin $(NAME)-0x10000.bin
