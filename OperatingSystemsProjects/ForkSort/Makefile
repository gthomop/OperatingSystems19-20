#Makefile
#Ioannis Thomopoulos
#OS Project 2
SHELL = /bin/bash
CC = gcc

BUILD 	=	build
INCLUDE =	include
BIN 	=	bin
SRC 	=	src

%.o: %.c
	$(CC) $(CFLAGS) -c -o $(BUILD)/$@ $<

%: %.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(addprefix $(BUILD)/,$^)

CFLAGS 		=	-g -Wall -Werror -Wextra -pedantic -I$(INCLUDE)/coach -I$(INCLUDE)/mysort -I$(INCLUDE)/sorter -I$(INCLUDE)/common
VPATH 		=	$(wildcard $(SRC)/*)

coach.o: coach.c
num_to_string.o: num_to_string.c
arguments.o: arguments.c
mysort.o: mysort.c
heapsort.o: heapsort.c
quicksort.o: quicksort.c
sorter.o: sorter.c

sorter: sorter.o heapsort.o quicksort.o num_to_string.o
coach: coach.o num_to_string.o
mysort: mysort.o arguments.o num_to_string.o

all: $(BUILD) $(BIN) sorter coach mysort

$(BUILD):
	@mkdir -p $@

$(BIN):
	@mkdir -p $@

PHONY: .clean
clean:
	rm -fr $(BIN) $(BUILD)
