TOPNAME := top

NXDC_FILES := constr/top.nxdc

INC_PATH ?=

VERILATOR := verilator

# 环境变量继承
VERILATOR_CFLAGS += -MMD --build -cc -O3 \
		    --x-assign fast \
		    --x-initial fast \
		    --noassert

BUILD_DIR := ./build
OBJ_DIR := $(BUILD_DIR)/obj_dir
BIN := $(BUILD_DIR)/$(TOPNAME)

# 默认生成目标，提前声明
default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))

SRC_AUTO_BIND := $(abspath $(BUILD_DIR)/auto_bind.cpp)

# 如果 top.nxdc 更新，则重新生成 auto_bind.cpp，否则跳过
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

VSRCS := $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS := $(shell find $(abspath ./csrc) -name "*.c" -or \
	 -name "*.cc" -or \
	 -name "*.cpp")

CSRCS += $(SRC_AUTO_BIND)

# 引入NVBoard的构建规则（Makefile）
include $(NVBOARD_HOME)/scripts/nvboard.mk

INCFLAGS = $(addprefix -I, $(INC_PATH))

CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) \
		$(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) \
		--exe \
		-o $(abspath $(BIN))

all: default

run: $(BIN)
	@$(BIN)

# make sim：记录开发踪迹并运行 NVBoard。
sim: $(BIN)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@$(BIN)

# 使用 Verilator 进行静态代码检查。
lint:
	$(VERILATOR) --lint-only -Wall \
		--top-module $(TOPNAME) \
		$(VSRCS)

# 删除构建产物。
clean:
	rm -rf $(BUILD_DIR) obj_dir wave.fst

.PHONY: default all run sim lint clean

# 引入 ysyx-workbench 顶层 Makefile，
# 从中获得 git_commit 函数。
include ../Makefile
