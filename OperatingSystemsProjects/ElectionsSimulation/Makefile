#Makefile
#Ioannis Thomopoulos
#Os Project 1
SHELL=/bin/bash
CC=gcc
FLAGS=-g -Wall -pedantic

BUILD=build
INCLUDE=include
BIN=bin
SRC=src

CFILES:=$(shell ls $(SRC))
OFILES:=$(patsubst %.c, %.o, $(CFILES))
OFILES:=$(addprefix $(BUILD)/, $(OFILES))
CFILES:=$(addprefix $(SRC)/, $(CFILES))
HFILES:=$(shell ls $(INCLUDE))
HFILES:=$(addprefix $(INCLUDE)/, $(HFILES))

OUT=runelection

all: $(BIN) $(BUILD) $(BIN)/$(OUT)

$(BIN)/$(OUT): $(OFILES)
	$(CC) $(FLAGS) -o $@ $(OFILES)

$(OFILES): 	$(CFILES) $(HFILES)
	$(CC) $(FLAGS) -I$(INCLUDE) -c $(patsubst $(BUILD)/%.o, $(SRC)/%.c, $@) -o $@

$(BIN):
	@mkdir $(BIN)

$(BUILD):
	@mkdir $(BUILD)

.PHONY: clean
clean:
	rm -fr $(BUILD) $(BIN)
