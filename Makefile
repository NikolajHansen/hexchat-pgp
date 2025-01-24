# Define the project name
PROJECT = hexchat-pgp

# Source directory
SRC_DIR = src

# Where the binary will be placed
BUILD_DIR = .

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -shared -fPIC -I/usr/include/hexchat

# Libraries to link
#LDLIBS = -lgpgme -lhexchat
LDLIBS = -lgpgme 

# Source files
SRC = $(SRC_DIR)/pgp-plugin.c

# Output file name
OUTPUT = $(BUILD_DIR)/pgp-plugin.so

# Build target
.PHONY: all clean

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(OUTPUT)
	
install:
	cp $(OUTPUT) ~/.config/hexchat/addons/pgp.plugin.so
