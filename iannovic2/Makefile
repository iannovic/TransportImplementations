SRC_DIR = ./src
BINS = abt gbn sr

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)

CC = /usr/bin/g++

all: $(BINS)

%: $(SRC_DIR)/%.cpp
	$(CC) -o $@ $<

clean:
	rm -f abt gbn sr
