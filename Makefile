# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Directories
OUTPUT = output
BUILD = build
SRC = src
INCLUDE = include
LIB = lib

# SDL specific include and library paths
CXXFLAGS += -I$(INCLUDE)
LDFLAGS += -L$(LIB) -lSDL2 -lSDL2_image -lSDL2_ttf

# Executable name
MAIN = main

# File extensions
SRCEXT = cpp
OBJEXT = o

# Source files, object files, and dependency files
SOURCES = $(wildcard $(SRC)/*.$(SRCEXT))
OBJECTS = $(patsubst $(SRC)/%.$(SRCEXT), $(BUILD)/%.$(OBJEXT), $(SOURCES))
DEPS = $(OBJECTS:.$(OBJEXT)=.d)

# Final output executable
OUTPUTMAIN = $(OUTPUT)/$(MAIN)

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    OUTPUTMAIN := $(OUTPUTMAIN).exe
    RM = del /q /f
    MD = mkdir
    COPY = cp
else
    RM = rm -f
    MD = mkdir -p
    COPY = cp
endif

# Default target
all: $(OUTPUT) $(BUILD) $(OUTPUTMAIN) $(OUTPUT)/SDL2.dll
	@echo Build complete!

# Create output and build directories
$(OUTPUT):
	@$(MD) $(OUTPUT) || true

$(BUILD):
	@$(MD) $(BUILD) || true

# Link the final executable
$(OUTPUTMAIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUTMAIN) $(OBJECTS) $(LDFLAGS)

# Compile source files into object files
$(BUILD)/%.$(OBJEXT): $(SRC)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Copy SDL2.dll to the output folder
$(OUTPUT)/SDL2.dll: $(LIB)/SDL2.dll
	$(COPY) $(LIB)/SDL2.dll $(OUTPUT)/SDL2.dll

# Include dependency files
-include $(DEPS)

# Rule to clean up generated files
.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(OBJECTS)
	$(RM) $(DEPS)
	$(RM) $(BUILD)\*
	$(RM) $(OUTPUT)/SDL2.dll
	@echo Cleanup complete!

# Run the program
.PHONY: run
run: all
	./$(OUTPUTMAIN)
	@echo Run complete!
