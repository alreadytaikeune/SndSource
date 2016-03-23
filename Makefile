CC=g++
LIBS_FFMPEG=-lavutil -lavformat -lavcodec -lswscale -lswresample

LIBS_MISC= -lstdc++ -lm -lz -lpthread \
-lfftw3 -lboost_program_options 

LIBS=$(LIBS_MISC) $(LIBS_FFMPEG)
INC=-Ilib
CFLAGS=-std=c++11 -g -O0


LINKER   = gcc -o
# linking flags here
LFLAGS   = -Wall 

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

TARGET=sndsource
SRC=$(wildcard src/**/*.cc src/*.cc)
OBJ=$(SRC:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)


$(BINDIR)/$(TARGET): $(OBJ)
	@echo $(LINKER) $@ $(LFLAGS) $(OBJ) $(LIBPATH) $(LIBS)
	@$(LINKER) $@ $(LFLAGS) $(OBJ) $(LIBPATH) $(LIBS)
	@echo "Linking complete!"

$(OBJ): $(OBJDIR)/%.o : $(SRCDIR)/%.cc
	@$(CC) $(CFLAGS) -c $< -o $@ $(INC)
	@echo "Compiled "$<" successfully!"

build:
	@- if [ ! -d "$(OBJDIR)" ]; then mkdir "$(OBJDIR)"; fi
	@- if [ ! -d "$(BINDIR)" ]; then mkdir "$(BINDIR)"; fi

.PHONY: lib


.PHONY: clean
clean:
	@- $(RM) $(TARGET)
	@- $(RM) $(OBJ)
