CC=gcc -std=c++11 -fPIC
LINKER = g++ -shared

SWIG=swig -c++ -python
FLAGS=-fPIC -g
INC=-I/usr/include/python2.7

ALL_INC= $(INC) -I../src
ALL_LIB_PATH=-L../lib
ALL_LIB=-lsndsource -lstdc++ -lm -ldl -lz -lpthread -lboost_program_options 

FLIBS=libavformat libavutil libswscale libswresample libavcodec


ifdef FFMPEG
	ALL_INC := $(ALL_INC) -I$(FFMPEG)
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
	ALL_INC := $(ALL_INC) -I$(BOOST)
	LIBPATH := $(LIBPATH) -L$(BOOST)/stage/lib
endif

ALL_LIB_PATH := $(ALL_LIB_PATH) $(LIBPATH)

all: clean _sndsource.so

_sndsource.so: sndsource_wrap.o
	$(LINKER) $< $(ALL_LIB_PATH) $(ALL_LIB) -Wl,-rpath,../lib $(LDPATH) -o $@

sndsource_wrap.o: sndsource_wrap.cxx
	$(CC) -c $^ $(ALL_INC) -o $@

sndsource_wrap.cxx: sndsource.i
	$(SWIG) $(ALL_INC) sndsource.i

.PHONY: clean

clean:
	@- $(RM) _sndsource.so sndsource_wrap.cxx sndsource_wrap.o