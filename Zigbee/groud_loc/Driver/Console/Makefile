###############################################################################
# Makefile for the project libConsole
###############################################################################

## General Flags
MCU = atmega644
TARGET = ../libConsole.a
CC = avr-gcc
AR = avr-ar

## Directories
SRC_PATH = ./src
OBJ_PATH = ./obj
DEP_PATH = ./dep
INC_PATH = ..

INCLUDES = $(addprefix -I,  $(subst :, ,$(INC_PATH)))
C_SUFFIX = *.c
C_SUF    = $(suffix $(C_SUFFIX))
CSRC_WITH_PATH  = $(wildcard $(C_SUFFIX) $(addsuffix /$(C_SUFFIX), $(subst :, ,$(SRC_PATH))))
CSRC     = $(notdir $(CSRC_WITH_PATH))

## Compile options common for all C compilation units.
CFLAGS  = -mmcu=$(MCU)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(OBJ_PATH)/$(*F).o -MF $(DEP_PATH)/$(@F).d 

## Objects that must be built in order to link
OBJECTS  = $(addprefix $(OBJ_PATH)/,$(CSRC:$(C_SUF)=.o))

## Build
make: $(TARGET)

all: clean $(TARGET)

## Compile
$(OBJ_PATH)/%.o: $(SRC_PATH)/%$(suffix $(C_SUFFIX)) ./makefile
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $@
	@echo

## Link
$(TARGET): $(OBJECTS)
	 $(AR) -r $(TARGET) $(OBJECTS)

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJ_PATH)/* $(DEP_PATH)/*


## Other dependencies
-include $(shell mkdir $(DEP_PATH) $(OBJ_PATH) 2>/dev/null) $(wildcard $(DEP_PATH)/*)
