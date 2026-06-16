PROJECT = stm32f446_rtos
BUILD_DIR = bin

CFILES = Src/main.c

# FreeRTOS configuration
FREERTOS_DIR = FreeRTOS-Kernel
FREERTOS_PORT = $(FREERTOS_DIR)/portable/GCC/ARM_CM4F
CFILES += \
	$(FREERTOS_DIR)/tasks.c \
	$(FREERTOS_DIR)/list.c \
	$(FREERTOS_DIR)/queue.c \
	$(FREERTOS_DIR)/timers.c \
	$(FREERTOS_PORT)/port.c \
	$(FREERTOS_DIR)/portable/MemMang/heap_4.c

# Generate OBJS from CFILES (.c -> .o)
OBJS = $(CFILES:.c=.o)

DEVICE=stm32f446re
OOCD_FILE = board/st_nucleo_f4.cfg

V ?= 0
OPENCM3_DIR = libopencm3

CPPFLAGS += -IInc -I$(FREERTOS_DIR)/include -I$(FREERTOS_PORT)
LDFLAGS += --specs=nosys.specs -nostartfiles -Wl,--print-memory-usage

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all libopencm3

all: libopencm3 $(BUILD_DIR) $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

libopencm3:
	$(MAKE) -C $(OPENCM3_DIR) TARGETS=stm32/f4

clean:
	$(RM) -rf $(BUILD_DIR) generated.* $(OBJS)
	$(MAKE) -C $(OPENCM3_DIR) clean

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
