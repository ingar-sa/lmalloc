CC = gcc
SRC = $(filter-out src/foo.c src/sdhs/DevUtils/TestDataGenerator.c poc glibc linux gcc, $(shell find src -name "*.c"))
#SRC = src/foo.c
INCLUDES = -I. -I/usr/include/postgresql
LIBS =  -lpthread -lpq -lm

LINTER = clang-tidy
LINTER_FLAGS = -quiet

DEBUG_OPT_LEVEL ?= 0
LOG_LEVEL ?= 4
REL_LOG_LEVEL ?= 2
MEM_TRACE ?= 0
DISABLE_DEBUG_WARNINGS ?= 1

WARNING_FLAGS ?= -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -Wundef -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline -Wfloat-equal -Wpointer-arith -Wwrite-strings -Wold-style-definition 

ifeq ($(DISABLE_DEBUG_WARNINGS), 1)
DISABLED_DEBUG_WARNING_FLAGS ?= -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-discarded-qualifiers
else
DISABLED_DEBUG_WARNING_FLAGS = 
endif

DISABLED_WARNING_FLAGS = -Wno-gnu-zero-variadic-macro-arguments -Wno-cpp -Wno-aggregate-return

LM_DEBUG_FLAGS = -DLM_MEM_TRACE=$(MEM_TRACE) -DLM_LOG_GLOBAL=1 -DLM_LOG_LEVEL=$(LOG_LEVEL) -DLM_ASSERT=1
LM_RELEASE_FLAGS = -DLM_MEM_TRACE=0 -DLM_PRINTF_DEBUG_ENABLE=0 -DLM_LOG_LEVEL=$(REL_LOG_LEVEL)-DLM_ASSERT=0 

SDHS_LOG_LEVEL ?= -DSDHS_LOG_LEVEL=3
SDHS_REL_LOG_LEVEL ?= -DSDHS_LOG_LEVEL=2

SDHS_DEBUG_FLAGS = -DSDHS_MEM_TRACE=1 -DSDHS_PRINTF_DEBUG_ENABLE=1 -DSDHS_ASSERT=1 $(SDHS_LOG_LEVEL)
RELEASE_SDHS_FLAGS = -DSDHS_MEM_TRACE=0 -DSDHS_PRINTF_DEBUG_ENABLE=0 -DSDHS_ASSERT=0 $(SDHS_REL_LOG_LEVEL)

DEBUG_FLAGS = -std=gnu11 -g -O$(DEBUG_OPT_LEVEL) $(WARNING_FLAGS) $(DISABLED_DEBUG_WARNING_FLAGS) $(DISABLED_WARNING_FLAGS) -DDEBUG
RELWDB_FLAGS = -std=gnu11 -g -O2 $(WARNING_FLAGS) $(DISABLED_DEBUG_WARNING_FLAGS) $(DISABLED_WARNING_FLAGS) -DNDEBUG
RELEASE_FLAGS = -std=gnu11 -O3 -march=native $(WARNING_FLAGS) $(DISABLED_DEBUG_WARNING_FLAGS) $(DISABLED_WARNING_FLAGS) -DNDEBUG

PROGRAM_NAME = validation

TASKSET_C = 0
PROGRAM_ARGS = -core=$(TASKSET_C)

.PHONY: all debug relwdb release run docs lint static_analysis format compile_commands.json clean

all: debug

debug: CFLAGS = $(DEBUG_FLAGS) $(LM_DEBUG_FLAGS) $(SDHS_DEBUG_FLAGS)
debug: build_suite

relwdb: CFLAGS = $(RELWDB_FLAGS) $(LM_DEBUG_FLAGS) $(SDHS_DEBUG_FLAGS)
relwdb: build_suite

release: CFLAGS = $(RELEASE_FLAGS) $(LM_RELEASE_FLAGS) $(SDHS_RELEASE_FLAGS)
release: build_suite

run:
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
