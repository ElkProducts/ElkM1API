/* File: swig.i
*  Description: Interface file for automatically generating SWIG modules.
*  More info: http://www.swig.org/tutorial.html
*/
%module ElkM1API
%{
#include "ElkM1API.h"
#include "ElkM1Monitor.h"
#include "ElkM1AsciiAPI.h"
#include "ElkM1Connection.h"
#include "ElkM1SirenWords.h"
%}
%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"
%include "std_shared_ptr.i"
%shared_ptr(Elk::M1Connection)
%shared_ptr(Elk::ElkTCP)

%include "ElkM1API.h"
%include "ElkM1Monitor.h"
%include "ElkM1AsciiAPI.h"
%include "ElkM1Connection.h"
%include "ElkM1SirenWords.h"