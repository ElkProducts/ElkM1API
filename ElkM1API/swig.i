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
%include "std_shared_ptr.i"
%include "stdint.i"

// Set up things to have shared pointer passes
%shared_ptr(Elk::M1Connection)
%shared_ptr(Elk::ElkTCP)

// Set up things passed by vector
%template(BoolVector) std::vector<bool>;
%template(CharVector) std::vector<char>;
%template(IntVector) std::vector<int>;
%template(UShortVector) std::vector<uint16_t>;
%template(LogEntryVector) std::vector<Elk::M1API::LogEntry>;
%template(ArmStatusVector) std::vector<Elk::M1API::ArmStatus>;
%template(ZoneStateVector) std::vector<Elk::M1API::ZoneState>;

// Vector of enums needs special handling, use a single-element struct instead
%template(ChimeModeVector) std::vector<Elk::M1API::SChimeMode>;
%template(ZoneDefinitionVector) std::vector<Elk::M1API::SZoneDefinition>;


%include "ElkM1API.h"
%include "ElkM1Monitor.h"
%include "ElkM1AsciiAPI.h"
%include "ElkM1Connection.h"
%include "ElkM1SirenWords.h"