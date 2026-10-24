CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -O2 -Iinclude
LDFLAGS_CLI ?= -lm
LDFLAGS_GUI ?= -mwindows -lgdi32 -lcomctl32 -luser32 -lm

SRCDIR = src
INCDIR = include
EXDIR = examples
BUILDDIR = build

CORE_SRCS = $(SRCDIR)/din_crypto.c \
            $(SRCDIR)/din_model.c \
            $(SRCDIR)/din_pds.c \
            $(SRCDIR)/din_ipfs.c \
            $(SRCDIR)/din_blockchain.c \
            $(SRCDIR)/din_secagg.c \
            $(SRCDIR)/din_auditing.c \
            $(SRCDIR)/din_tokenomics.c \
            $(SRCDIR)/din_coordinator.c \
            $(SRCDIR)/din_wallet.c

GUI_SRCS = $(SRCDIR)/din_gui.c $(SRCDIR)/din_wallet_gui.c

CORE_OBJS = $(CORE_SRCS:%.c=$(BUILDDIR)/%.o)
GUI_OBJS = $(GUI_SRCS:%.c=$(BUILDDIR)/%.o)

TARGET_CLI = $(BUILDDIR)/din_simulation
TARGET_GUI = $(BUILDDIR)/din_gui.exe

.PHONY: all clean run run-gui

all: $(TARGET_CLI) $(TARGET_GUI)

$(TARGET_CLI): $(CORE_OBJS) $(BUILDDIR)/$(EXDIR)/main_simulation.o
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS_CLI)

$(TARGET_GUI): $(CORE_OBJS) $(GUI_OBJS) $(BUILDDIR)/$(EXDIR)/main_gui.o
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS_GUI)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET_CLI)
	./$(TARGET_CLI)

run-gui: $(TARGET_GUI)
	./$(TARGET_GUI)

clean:
	rm -rf $(BUILDDIR)
