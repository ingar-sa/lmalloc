CC = gcc
SRC = $(filter-out odin sdhs, $(shell find src -name "*.c"))
INCLUDES = -I.
LIBS =  -lpthread

LINTER = clang-tidy
LINTER_FLAGS = -quiet

EIT_LOG_LEVEL ?= 4
EIT_REL_LOG_LEVEL ?= 2

EIT_FLAGS = -DEIT_MEM_TRACE=1 -DEIT_LOG_GLOBAL=1 -DEIT_LOG_LEVEL=$(EIT_LOG_LEVEL) -DEIT_ASSERT=1
RELEASE_EIT_FLAGS = -DEIT_MEM_TRACE=0 -DEIT_PRINTF_DEBUG_ENABLE=0 -DEIT_LOG_LEVEL=$(EIT_REL_LOG_LEVEL)-DEIT_ASSERT=0 

DEBUG_FLAGS = -g -O0 -Wextra -pedantic  -Wno-unused-function -Wno-cpp -DDEBUG
RELWDB_FLAGS = -g -O2 -Wextra -pedantic -Wno-unused-function -Wno-cpp -DNDEBUG 
RELEASE_FLAGS = -O3 -march=native -Wextra -pedantic -Wno-unused-function -Wno-cpp -DNDEBUG

PROGRAM_NAME = validation

.PHONY: all debug relwdb release run docs lint static_analysis format compile_commands.json clean

all: debug

debug: CFLAGS = $(DEBUG_FLAGS) $(EIT_FLAGS)
debug: build_suite

relwdb: CFLAGS = $(RELWDB_FLAGS) $(EIT_FLAGS)
relwdb: build_suite

release: CFLAGS = $(RELEASE_FLAGS) $(RELEASE_EIT_FLAGS)
release: build_suite

run:
	./build/$(PROGRAM_NAME)

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
	@printf "\033[0;32m\nBuilding validation suite\n\033[0m"
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o build/$(PROGRAM_NAME) $(LIBS)
	@printf "\033[0;32mFinished building validation suite\n\033[0m"

clean:
	rm -rf build
