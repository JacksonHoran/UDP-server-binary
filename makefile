
# Simple Makefile for UDP-server-binary
# - builds all C programs found in bin_send_recv/ and init_tester/
# - places resulting executables into bin/

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic
LDFLAGS ?=
BIN_DIR := bin

SRC_DIRS := bin_send_recv init_tester

SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))
OBJS := $(patsubst %.c,%.o,$(SRCS))


# Map each source file to an executable name (strip directory and .c)
EXE_NAMES := $(patsubst %.c,%, $(notdir $(SRCS)))
EXES := $(patsubst %,$(BIN_DIR)/%,$(EXE_NAMES))

.PHONY: all clean help

all: $(BIN_DIR) $(EXES)

help:
	@echo "Makefile for UDP-server-binary"
	@echo "Targets:"
	@echo "  make        - build all binaries into $(BIN_DIR)/"
	@echo "  make clean  - remove built objects and binaries"
	@echo "  make help   - show this message"

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# findsrc(name) -> first matching source file for the executable name
findsrc = $(firstword $(foreach d,$(SRC_DIRS),$(wildcard $(d)/$(1).c)))

# Generate per-executable rules. For each EXE name, create a rule:
# bin/name: <source-path>
#
define make-exe-rule
$(BIN_DIR)/$(1): $(call findsrc,$(1))
	$(CC) $(CFLAGS) -o $$@ $$< $(LDFLAGS)
endef

$(foreach name,$(EXE_NAMES),$(eval $(call make-exe-rule,$(name))))

clean:
	-@rm -rf $(BIN_DIR)

