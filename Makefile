CC=g++
LIBS_FFMPEG=-lavutil -lavformat -lavcodec -lswscale -lswresample

LIBS_MISC= -lstdc++ -lm -ldl -lz -lpthread -lboost_program_options 

LIBS=$(LIBS_MISC) $(LIBS_FFMPEG)

FLIBS=libavformat libavutil libswscale libswresample libavcodec

ifdef FFMPEG
	INC = -I$(FFMPEG)
	LIBPATH = $(addprefix -L$(FFMPEG), $(FLIBS))
	# LIBS_FFMPEG=-l:$(FFMPEG)/lib/libavformat.a -l:$(FFMPEG)/lib/libavutil.a  \
	# -l:$(FFMPEG)/lib/libswscale.a -l:$(FFMPEG)/lib/libswresample.a -l:$(FFMPEG)/lib/libavcodec.a
	noop=
	space = $(noop) $(noop)
	coma =,
	PFLIBS = $(addprefix $(FFMPEG), $(FLIBS))
	LDPATH=-Wl,$(subst $(space),$(coma),$(addprefix -rpath$(coma), $(PFLIBS)))
endif

ifdef BOOST
	INC := $(INC) -I$(BOOST)
	LIBPATH := $(LIBPATH) -L$(BOOST)/stage/lib
endif

CFLAGS=-std=c++11 -g -O0 -fPIC


LINKER   = g++
# linking flags here
LFLAGS   = -Wall

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
LIBDIR   = lib

TARGET=sndsource
SRC=$(wildcard src/**/*.cc src/*.cc)
OBJ=$(SRC:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)

all: clean $(BINDIR)/$(TARGET) lib
	$(MAKE) -C python


$(BINDIR)/$(TARGET): build $(OBJ)
	@echo $(LINKER) $(LFLAGS) $(OBJ) $(LIBPATH) $(LIBS) $(LDPATH) -o $@
	@$(LINKER) $(LFLAGS) $(OBJ) $(LIBPATH) $(LIBS) $(LDPATH) -Wl,--no-as-needed -o $@
	@echo "Linking complete!"

$(OBJ): $(OBJDIR)/%.o : $(SRCDIR)/%.cc
	@echo $(CC) $(CFLAGS) -c $< -o $@ $(INC)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INC)
	@echo "Compiled "$<" successfully!"

build: 
	@- if [ ! -d "$(OBJDIR)" ]; then mkdir "$(OBJDIR)"; fi
	@- if [ ! -d "$(BINDIR)" ]; then mkdir "$(BINDIR)"; fi

.PHONY: lib

lib: libsndsource.so
	@- if [ ! -d "$(LIBDIR)" ]; then mkdir "$(LIBDIR)"; fi
	@- mv libsndsource.so $(LIBDIR)


libsndsource.so: obj/SndSource.o obj/SndWriter.o obj/ringbuffer.o
	$(LINKER) $^ $(LFLAGS) -shared $(LIBPATH) $(LIBS) $(LDPATH) -Wl,-soname,$@ -o $@

.PHONY: clean
clean:
	@- $(RM) $(TARGET)
	@- $(RM) $(OBJ)
