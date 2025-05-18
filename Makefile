#################################################################
#	   Dolphin Network Base Package SDK Libraries Makefile	   #
#################################################################

# OS Detection and Environment Setup
EXE_EXT :=
WINE_TOOL := # For running Windows executables on non-Windows hosts if needed
HOST_OS_NAME :=
PYTHON_EXECUTABLE := python3 # Default to python3 for Linux/macOS

# Check for Windows first using Make's OS variable
ifneq (,$(findstring Windows,$(OS)))
	HOST_OS_NAME := windows
	EXE_EXT := .exe
	WINE_TOOL := # mwcceppc.exe is a Windows exe, so no wine needed when on Windows
	PYTHON_EXECUTABLE := python # Or py -3. User should ensure it's in PATH.

	# On Windows, it is strongly recommended to run this Makefile from a
	# Unix-like shell environment (e.g., Git Bash, MSYS2 MinGW shell)
	# which provides common utilities (find, mkdir -p, rm -rf, sed, awk, etc.)
	# and handles POSIX paths (/dev/null) correctly.
	# The following findcmd assumes 'where' is available and cmd.exe-like shell logic,
	# but 'type' from sh.exe (Git Bash) would also work with the original Unix findcmd.
	# For simplicity with Git Bash, original findcmd can often be kept.
	# define findcmd_win_shell
	#   @where $(1) >NUL 2>&1 && echo 0 || echo 1
	# endef
	# findcmd = $(shell $(findcmd_win_shell))
	# If using Git Bash, the original findcmd might be preferable:
	findcmd = $(shell type $(1) >/dev/null 2>/dev/null; echo $$?)

else
	# Fallback to uname for Linux/macOS if not Windows
	UNAME_S_DETECTED := $(shell uname -s)
	ifeq ($(UNAME_S_DETECTED),Linux)
		HOST_OS_NAME := linux
		WINE_TOOL := wibo # Command to use Wine for mwcceppc.exe
	else ifeq ($(UNAME_S_DETECTED),Darwin)
		HOST_OS_NAME := macos
		WINE_TOOL := wibo # Command to use Wine for mwcceppc.exe
	else
		$(error Unsupported host/building OS. OS Env: '$(OS)', uname -s: '$(UNAME_S_DETECTED)')
	endif
	findcmd = $(shell type $(1) >/dev/null 2>/dev/null; echo $$?)
endif

# If 0, tells the console to chill out. (Quiets the make process.)
VERBOSE ?= 0
QUIET :=
ifeq ($(VERBOSE),0)
  QUIET := @
endif

# Override HOST_OS based on detection (primarily for rules that use it)
HOST_OS := $(HOST_OS_NAME)

BUILD_DIR := build/build
TOOLS_DIR := $(BUILD_DIR)/tools
BASEROM_DIR := baserom

# Define the output directory for the .a files
OUTPUT_DIR := out

TARGET_LIBS := mdm \
			   eth \
			   netcfg

VERIFY_LIBS := $(TARGET_LIBS)

PYTHON := $(PYTHON_EXECUTABLE)

# Every file has a debug version. Append D to the list.
TARGET_LIBS_DEBUG := $(addsuffix D,$(TARGET_LIBS))

# Define the paths to the output .a files
RELEASE_LIBS := $(addprefix $(OUTPUT_DIR)/,$(addsuffix .a,$(TARGET_LIBS)))
DEBUG_LIBS := $(addprefix $(OUTPUT_DIR)/,$(addsuffix .a,$(TARGET_LIBS_DEBUG)))

# SRC_DIRS, ASM_DIRS, DATA_DIRS need to be defined or populated appropriately.
# Assuming ASM_DIRS and DATA_DIRS are defined elsewhere or intended to be empty if not.
# The $(shell find ...) commands below assume 'find' is available in the PATH.
SRC_DIRS := $(shell find build/src -type d)
# Example definitions if they were missing:
# ASM_DIRS := $(shell find build/asm -type d)
# DATA_DIRS := $(shell find build/data -type d)


###################### Other Tools ######################

C_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES := $(foreach dir,$(SRC_DIRS) $(ASM_DIRS),$(wildcard $(dir)/*.s))
DATA_FILES := $(foreach dir,$(DATA_DIRS),$(wildcard $(dir)/*.bin))
# Corrected BASEROM_FILES assuming BASEROM_DIRS is a list of directory paths
BASEROM_FILES := $(foreach dir,$(BASEROM_DIRS),$(wildcard $(dir)/*.s))


# Object files
O_FILES := $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file:.c=.c.o)) \
		   $(foreach file,$(S_FILES),$(BUILD_DIR)/$(file:.s=.s.o)) \
		   $(foreach file,$(DATA_FILES),$(BUILD_DIR)/$(file:.bin=.bin.o))

DEP_FILES := $(O_FILES:.o=.d) $(DECOMP_C_OBJS:.o=.asmproc.d) # DECOMP_C_OBJS is not defined in snippet

##################### Compiler Options #######################
# findcmd is defined in the OS detection block

# todo, please, better CROSS than this.
CROSS := powerpc-eabi-gcc-#powerpc-linux-gnu-

COMPILER_VERSION ?= 1.2.5n

COMPILER_DIR := mwcc_compiler/GC/$(COMPILER_VERSION)# Use forward slashes; mwcceppc.exe or Wine should handle them.
AS = $(CROSS)as
MWCC	:= $(WINE_TOOL) $(COMPILER_DIR)/mwcceppc$(EXE_EXT) # Add EXE_EXT for the compiler executable if it has one (unlikely for mwcceppc.exe itself, but WINE_TOOL handles the .exe part implicitly)
														# If mwcceppc.exe is indeed the filename, it's covered.
AR = $(CROSS)ar
LD = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy

ifeq ($(HOST_OS),macos)
  CPP := clang -E -P -x c
else ifeq ($(HOST_OS),linux)
  CPP := cpp
else ifeq ($(HOST_OS),windows)
  # MWCC likely handles C preprocessing. If a separate CPP is ever explicitly used
  # by a rule on Windows for .c files, this would need to be a valid command.
  # For now, set to a remark or a safe default if not directly used in C compilation rules.
  CPP := REM_WINDOWS_CPP_IF_NEEDED
endif
DTK	 := $(TOOLS_DIR)/dtk$(EXE_EXT)
DTK_VERSION := 0.9.6

CC		  := $(MWCC)

######################## Flags #############################

CHARFLAGS := -char signed

CFLAGS = $(CHARFLAGS) -lang=c -nodefaults -proc gekko -fp hard -Cpp_exceptions off -enum int -warn pragmas -requireprotos -pragma 'cats off'
INCLUDES := -Ibuild/include -Idolphin/include/libc -Idolphin/include -ir build/src

ASFLAGS = -mgekko -I build/src -I build/include

######################## Targets #############################

# The $(shell mkdir -p ...) command assumes 'mkdir -p' is available.
# This is true in Git Bash / MSYS2 environments.
$(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(DATA_DIRS),$(shell mkdir -p build/build/release/$(dir) build/build/debug/$(dir)))

%/stub.o: CFLAGS += -warn off

######################## Build #############################

A_FILES := $(foreach dir,$(BASEROM_DIR),$(wildcard $(dir)/*.a))

TARGET_LIBS := $(addprefix baserom/,$(addsuffix .a,$(TARGET_LIBS)))
TARGET_LIBS_DEBUG := $(addprefix baserom/,$(addsuffix .a,$(TARGET_LIBS_DEBUG)))

default: all

all: $(DTK) $(RELEASE_LIBS) $(DEBUG_LIBS)

# For verify, sha1sum and sed are expected in PATH.
verify: build/build/release/test.bin build/build/debug/test.bin build/build/verify.sha1
	@sha1sum -c build/build/verify.sha1

# The 'extract' target uses Unix shell features extensively (find, pipes, while read, variable expansion).
# It requires a Unix-like shell (Git Bash, MSYS2) on Windows.
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
# rm -rf is expected from the environment (Git Bash, etc.)
distclean:
	-rm -rf $(BASEROM_DIR)/release
	-rm -rf $(BASEROM_DIR)/debug
	$(MAKE) clean

clean:
	-rm -rf $(BUILD_DIR)
	-rm -rf $(OUTPUT_DIR)

# mkdir -p is expected from the environment.
$(TOOLS_DIR):
	$(QUIET) mkdir -p $(TOOLS_DIR)

.PHONY: check-dtk

# The 'check-dtk' target uses awk and Unix shell 'if' syntax.
# It requires a Unix-like shell and awk in PATH on Windows.
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
$(OUTPUT_DIR)/eth.a : $(eth_obj_files) | $(OUTPUT_DIR)

# For ethD.a
ethD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(eth_c_files))
$(OUTPUT_DIR)/ethD.a : $(ethD_obj_files) | $(OUTPUT_DIR)

# Similarly for mdm.a and mdmD.a
mdm_c_files := $(wildcard build/src/mdm/*.c)
mdm_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/release/build/src/%.o,$(mdm_c_files))
$(OUTPUT_DIR)/mdm.a : $(mdm_obj_files) | $(OUTPUT_DIR)

mdmD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(mdm_c_files))
$(OUTPUT_DIR)/mdmD.a : $(mdmD_obj_files) | $(OUTPUT_DIR)

# And for netcfg.a and netcfgD.a
netcfg_c_files := $(wildcard build/src/netcfg/*.c)
netcfg_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/release/build/src/%.o,$(netcfg_c_files))
$(OUTPUT_DIR)/netcfg.a : $(netcfg_obj_files) | $(OUTPUT_DIR)

netcfgD_obj_files := $(patsubst build/src/%.c,$(BUILD_DIR)/debug/build/src/%.o,$(netcfg_c_files))
$(OUTPUT_DIR)/netcfgD.a : $(netcfgD_obj_files) | $(OUTPUT_DIR)

# Ensure the output directory exists (mkdir -p expected)
$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

build/build/release/baserom.elf: build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),baserom/$(l).a)
build/build/release/test.elf:	 build/build/release/src/stub.o $(addprefix $(OUTPUT_DIR)/,$(addsuffix .a,$(VERIFY_LIBS)))
build/build/debug/baserom.elf:   build/build/release/src/stub.o $(foreach l,$(VERIFY_LIBS),baserom/$(l)D.a)
build/build/debug/test.elf:	   build/build/release/src/stub.o $(addprefix $(OUTPUT_DIR)/,$(addsuffix D.a,$(VERIFY_LIBS)))

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.elf:
	@echo Linking ELF $@
	$(QUIET)$(LD) -T gcn.ld --whole-archive $(filter %.o,$^) $(filter %.a,$^) -o $@ -Map $(@:.elf=.map)

# Update the pattern rule to handle the output directory
$(OUTPUT_DIR)/%.a:
	@ test ! -z '$?' || { echo 'no object files for $@'; exit 1; } # exit 1 instead of return 1 for shell
	@echo 'Creating static library $@'
	$(QUIET)$(AR) -v -r $@ $(filter %.o,$?)

# generate baserom hashes (sha1sum and sed expected from env)
build/build/verify.sha1: build/build/release/baserom.bin build/build/debug/baserom.bin
	sha1sum $^ | sed 's/baserom/test/' > $@

# ------------------------------------------------------------------------------

.PHONY: all clean distclean default split setup extract verify

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true

-include $(DEP_FILES)
