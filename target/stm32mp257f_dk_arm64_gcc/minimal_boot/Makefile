#
#  minimal_boot Makefile（STM32MP257F-DK / SWD デバッグ用ランディングパッド）
#
#  build/main.bin を生成する．これを TF-A の fiptool で FIP の bm-fw として
#  書き込むと，起動後 USART2 に "Connect using OpenOCD" を表示して停止する．
#

CROSS   ?= aarch64-none-elf-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy

#  FSBL が bm-fw をロードし entry するアドレス（既定 0x88000000）
TEXT    ?= 0x88000000

CFLAGS   = -mcpu=cortex-a35 -ffreestanding -nostdlib -nostartfiles
LDFLAGS  = -Wl,-Ttext,$(TEXT) -Wl,-T,linkscript.ld -Wl,--build-id=none

BUILD    = build

all: $(BUILD)/main.bin

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.elf: start.S linkscript.ld | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ start.S

$(BUILD)/main.bin: $(BUILD)/main.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean
