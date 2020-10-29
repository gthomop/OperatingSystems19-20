#Makefile
#Ioannis Thomopoulos
#OS Project 3
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

CFLAGS 		=	-pthread -g -Wall -Werror -Wextra -pedantic -I$(INCLUDE)/bus -I$(INCLUDE)/comptroller -I$(INCLUDE)/mystation -I$(INCLUDE)/stationManager -I$(INCLUDE)
VPATH 		=	$(wildcard $(SRC)/*):$(wildcard $(SRC))

#Object files
bus.o: bus.c
busArguments.o: busArguments.c
busUtilities.o: busUtilities.c
comptroller.o: comptroller.c
comptrollerArguments.o: comptrollerArguments.c
comptrollerUtilities.o: comptrollerUtilities.c
configParser.o: configParser.c
linkedList.o: linkedList.c
mystation.o: mystation.c
simHelpers.o: simHelpers.c
simUtilities.o: simUtilities.c
stationManager.o: stationManager.c
stationManagerUtilities.o: stationManagerUtilities.c

#Executable files
bus: bus.o busArguments.o busUtilities.o
comptroller: comptroller.o comptrollerArguments.o comptrollerUtilities.o
mystation: configParser.o linkedList.o mystation.o simUtilities.o simHelpers.o
stationManager: stationManager.o stationManagerUtilities.o

all: $(BUILD) $(BIN) bus comptroller mystation stationManager

$(BUILD):
	@mkdir -p $@

$(BIN):
	@mkdir -p $@

PHONY: .clean
clean:
	rm -fr $(BIN) $(BUILD)
