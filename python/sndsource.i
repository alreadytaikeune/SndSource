%module(directors="1") sndsource

%{
    #define SWIG_FILE_WITH_INIT
    #include "../src/SndWriter.h"
    #include "SndSource2.h"
%}


%include <std_string.i>
%include "numpy.i"
%init %{
import_array();
%}



%include "../src/SndWriter.h"
%include "SndSource2.h"