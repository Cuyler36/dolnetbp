#################################################################
#      Dolphin Network Base Package SDK Libraries Makefile      #
#################################################################

ifneq (,$(findstring Windows,$(OS)))
  EXE := .exe
else
  WINE := wibo
endif

# If 0, tells the console to chill out. (Quiets the make process.)
VERBOSE ?= 0

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  HOST_OS := linux
else ifeq ($(UNAME_S),Darwin)
  HOST_OS := macos
else
  $(error Unsupported host/building OS <$(UNAME_S)>)
endif

BUILD_DIR := build/build
TOOLS_DIR := $(BUILD_DIR)/tools
BASEROM_DIR := baserom
TARGET_LIBS := mdm              \
               eth              \
               netcfg

VERIFY_LIBS := $(TARGET_LIBS)

ifeq ($(VERBOSE),0)
  QUIET := @
endif

PYTHON := python3

# Every file has a debug version. Append D to the list.
TARGET_LIBS_DEBUG := $(addsuffix D,$(TARGET_LIBS))

# TODO, decompile
SRC_DIRS := $(shell find build/src -type d)

###################### Other Tools ######################

C_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES := $(foreach dir,$(SRC_DIRS) $(ASM_DIRS),$(wildcard $(dir)/*.s))
DATA_FILES := $(foreach dir,$(DATA_DIRS),$(wildcard $(dir)/*.bin))
BASEROM_FILES := $(foreach dir,$(BASEROM_DIRS)\,$(wildcard $(dir)/*.s))

# Object files
O_FILES := $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file:.c=.c.o)) \
           $(foreach file,$(S_FILES),$(BUILD_DIR)/$(file:.s=.s.o)) \
           $(foreach file,$(DATA_FILES),$(BUILD_DIR)/$(file:.bin=.bin.o)) \

DEP_FILES := $(O_FILES:.o=.d) $(DECOMP_C_OBJS:.o=.asmproc.d)

##################### Compiler Options #######################
findcmd = $(shell type $(1) >/dev/null 2>/dev/null; echo $$?)

# todo, please, better CROSS than this.
CROSS := powerpc-linux-gnu-

COMPILER_VERSION ?= 1.2.5n

COMPILER_DIR := mwcc_compiler/GC/$(COMPILER_VERSION)
AS = $(CROSS)as
MWCC    := $(WINE) $(COMPILER_DIR)/mwcceppc.exe
AR = $(CROSS)ar
LD = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy
ifeq ($(HOST_OS),macos)
  CPP := clang -E -P -x c
else
  CPP := cpp
endif
DTK     := $(TOOLS_DIR)/dtk
DTK_VERSION := 0.9.6

CC        = $(MWCC)

######################## Flags #############################

CHARFLAGS := -char signed

CFLAGS = $(CHARFLAGS) -lang=c++ -nodefaults -proc gekko -fp hard -Cpp_exceptions off -enum int -warn pragmas -requireprotos -pragma 'cats off'
INCLUDES := -Ibuild/include -Idolphin/include/libc -Idolphin/include -ir build/src

ASFLAGS = -mgekko -I build/src -I build/include

######################## Targets #############################

$(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(DATA_DIRS),$(shell mkdir -p build/build/release/$(dir) build/build/debug/$(dir)))

%/stub.o: CFLAGS += -warn off

######################## Build #############################

A_FILES := $(foreach dir,$(BASEROM_DIR),$(wildcard $(dir)/*.a))

TARGET_LIBS := $(addprefix baserom/,$(addsuffix .a,$(TARGET_LIBS)))
TARGET_LIBS_DEBUG := $(addprefix baserom/,$(addsuffix .a,$(TARGET_LIBS_DEBUG)))

default: all

all: $(DTK) eth.a ethD.a mdm.a mdmD.a netcfg.a netcfgD.a stub.o

verify: build/build/release/test.bin build/build/debug/test.bin build/build/verify.sha1
	@sha1sum -c build/build/verify.sha1

extract: $(DTK)
	$(info Extracting files...)
	@$(DTK) ar extract $(TARGET_LIBS) --out baserom/release/src
	@$(DTK) ar extract $(TARGET_LIBS_DEBUG) --out baserom/debug/src
    # Thank you GPT, very cool. Temporary hack to remove D off of inner src folders to let objdiff work.
	@for dir in $$(find baserom/debug/src -type d -name 'src'); do \
		find "$$dir" -mindepth 1 -maxdepth 1 -type d | while read subdir; do \
			mv "$$subdir" "$${subdir%?}"; \
		done \
	done
	# Disassemble the objects and extract their dwarf info.
	find baserom -name '*.o' | while read i; do \
		$(DTK) elf disasm $$i $${i%.o}.s ; \
		$(DTK) dwarf dump $$i -o $${i%.o}_DWARF.c ; \
	done

# clean extraction so extraction can be done again.
distclean:
	rm -rf $(BASEROM_DIR)/release
	rm -rf $(BASEROM_DIR)/debug
	make clean

clean:
	rm -rf $(BUILD_DIR)
	rm -rf *.a

$(TOOLS_DIR):
	$(QUIET) mkdir -p $(TOOLS_DIR)

.PHONY: check-dtk

check-dtk: $(TOOLS_DIR)
	@version=$$($(DTK) --version | awk '{print $$2}'); \
	if [ "$(DTK_VERSION)" != "$$version" ]; then \
		$(PYTHON) tools/download_dtk.py dtk $(DTK) --tag "v$(DTK_VERSION)"; \
	fi

$(DTK): check-dtk

build/build/debug/build/src/%.o: build/src/%.c
	@echo 'Compiling $< (debug)'
	$(QUIET)$(CC) -c -opt level=0 -inline off -schedule off -sym on $(CFLAGS) -I- $(INCLUDES) -DDEBUG $< -o $@

build/build/release/build/src/%.o: build/src/%.c
	@echo 'Compiling $< (release)'
	$(QUIET)$(CC) -c -O4,p -inline auto -sym on $(CFLAGS) -I- $(INCLUDES) -DRELEASE $< -o $@

################################ Build Files ###############################

# For eth.a
eth_c_files := $(wildcard build/src/eth/*.c)
eth_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/release/build/src/%.o,$(eth_c_files))
eth.a : $(eth_obj_files)

# For ethD.a
ethD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(eth_c_files))
ethD.a : $(ethD_obj_files)

# Similarly for mdm.a and mdmD.a
mdm_c_files := $(wildcard build/src/mdm/*.c)
mdm_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/release/build/src/%.o,$(mdm_c_files))
mdm.a : $(mdm_obj_files)

mdmD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(mdm_c_files))
mdmD.a : $(mdmD_obj_files)

# And for netcfg.a and netcfgD.a
netcfg_c_files := $(wildcard build/src/netcfg/*.c)
netcfg_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/release/build/src/%.o,$(netcfg_c_files))
netcfg.a : $(netcfg_obj_files)

netcfgD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(netcfg_c_files))
netcfgD.a : $(netcfgD_obj_files)

build/build/release/baserom.elf: build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),baserom/$(l).a)
build/build/release/test.elf:    build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),$(l).a)
build/build/debug/baserom.elf:   build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),baserom/$(l)D.a)
build/build/debug/test.elf:      build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),$(l)D.a)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.elf:
	@echo Linking ELF $@
	$(QUIET)$(LD) -T gcn.ld --whole-archive $(filter %.o,$^) $(filter %.a,$^) -o $@ -Map $(@:.elf=.map)

%.a:
	@ test ! -z '$?' || { echo 'no object files for $@'; return 1; }
	@echo 'Creating static library $@'
	$(QUIET)$(AR) -v -r $@ $(filter %.o,$?)

# generate baserom hashes
build/build/verify.sha1: build/build/release/baserom.bin build/build/debug/baserom.bin
	sha1sum $^ | sed 's/baserom/test/' > $@

# ------------------------------------------------------------------------------

.PHONY: all clean distclean default split setup extract

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true

-include $(DEP_FILES)
