%module(directors="1") sndsource

%{
    #define SWIG_FILE_WITH_INIT
    #include "SndWriter2.h"
    #include "SndSource2.h"
%}


%include <std_string.i>
%include "numpy.i"
%init %{
import_array();
%}



%include "SndWriter2.h"
%include "SndSource2.h"
%include "enums.h"