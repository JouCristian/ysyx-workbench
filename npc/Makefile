TOPNAME := top

VSRCS := vsrc/top.v

CSRCS := csrc/main.cpp

OBJ_DIR := obj_dir

BIN := $(OBJ_DIR)/V$(TOPNAME)

all: sim

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	verilator --cc --exe --build -j 0 -Wall --trace-fst \
		--top-module $(TOPNAME) \
		--Mdir $(OBJ_DIR) \
		$(VSRCS) $(CSRCS) 
		./$(BIN)

wave:
	gtkwave wave.fst

clean:
	rm -rf $(OBJ_DIR) wave.fst

.PHONY: all sim wave clean


include ../Makefile
