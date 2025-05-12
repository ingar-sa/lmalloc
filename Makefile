CC = gcc
SRC = $(filter-out src/foo.c src/godbolt/wrappers.c src/sdhs/DevUtils/TestDataGenerator.c poc glibc linux gcc, $(shell find src -name "*.c"))
#SRC = src/foo.c
INCLUDES = -I. -I/usr/include/postgresql
LIBS =  -lpthread -lpq -lm

LINTER = clang-tidy
LINTER_FLAGS = -quiet

OPT_LEVEL ?= 2
LOG_LEVEL ?= 4
MEM_TRACE ?= 0

PROGRAM_NAME = validation
TASKSET_C = 0
PROGRAM_ARGS = 

WARNING_FLAGS ?= -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -Wundef -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline -Wfloat-equal -Wpointer-arith -Wwrite-strings -Wold-style-definition 

DISABLED_WARNING_FLAGS = -Wno-cpp -Wno-aggregate-return -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-discarded-qualifiers -Wno-unused-but-set-variable
#-Wno-gnu-zero-variadic-macro-arguments

SDHS_LOG_LEVEL ?= -DSDHS_LOG_LEVEL=3
SDHS_FLAGS = -DSDHS_MEM_TRACE=0 -DSDHS_PRINTF_DEBUG_ENABLE=1 -DSDHS_ASSERT=1 $(SDHS_LOG_LEVEL)
LM_FLAGS = -DLM_MEM_TRACE=$(MEM_TRACE) -DLM_LOG_GLOBAL=1 -DLM_LOG_LEVEL=$(LOG_LEVEL) -DLM_ASSERT=1

.PHONY: all validation run docs lint static_analysis format compile_commands.json clean

all: validation
validation: CFLAGS = -std=gnu11 -g -O$(OPT_LEVEL) $(WARNING_FLAGS) $(DISABLED_WARNING_FLAGS) $(LM_FLAGS) $(SDHS_FLAGS) -DNDEBUG
validation: build_suite

run:
	@echo "Setting cpu frequency governor to performance"
	sudo cpupower frequency-set -g performance
	taskset -c $(TASKSET_C) ./build/$(PROGRAM_NAME) $(PROGRAM_ARGS)

docs:
	@echo "Generating documentation..."
	doxygen Doxyfile
	@echo "Documentation generated in docs/html"

lint: compile_commands.json
	@echo "Running clang-tidy..."
	$(LINTER) $(SRC) $(LINTER_FLAGS)
	@echo "Linting completed."

static_analysis: 
	@echo "Running cppcheck..."
	cppcheck --enable=all --inconclusive --error-exitcode=1 src
	@echo "Static analysis completed."

format:
	@echo "Formatting code..."
	clang-format -i $(SRC)
	@echo "Code formatting completed."

compile_commands.json: 
	@echo "Generating compile_commands.json..."
	rm -f compile_commands.json
	bear -- make

build_suite:
	@mkdir -p build
	@mkdir -p logs
	@printf "\033[0;32m\nBuilding validation suite\n\033[0m"
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o build/$(PROGRAM_NAME) $(LIBS)
	@printf "\033[0;32mFinished building validation suite\n\033[0m"

clean:
	rm -rf build
