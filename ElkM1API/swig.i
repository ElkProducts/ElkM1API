/* File: swig.i
*  Description: Interface file for automatically generating SWIG modules.
*  More info: http://www.swig.org/tutorial.html
*/
%module(directors=1) ElkM1API
%{
#include "ElkM1Definition.h"
#include "SwigCallbacks.h"
#include "ElkM1API.h"
#include "ElkM1Monitor.h"
#include "ElkM1AsciiAPI.h"
#include "ElkM1Connection.h"
#include "ElkM1SirenWords.h"
#include "ElkC1M1Tunnel.h"
%}
%include "std_pair.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_shared_ptr.i"
%include "stdint.i"
%include "exception.i"

// Set up things to have shared pointer passes
%shared_ptr(Elk::ElkTCP)
%shared_ptr(Elk::M1Connection)
%shared_ptr(BoolCallback)
%shared_ptr(ArmStatusVectorCallback)

// This allows the C++ code to run calls that wind all the way up into the target platform code.
%feature ("director") BoolCallback;
%feature ("director") IntCallback;
%feature ("director") ArmStatusVectorCallback;

// This allows us to make the M1Connection calls against a derived target-language class, so we can 
// implement the connection on the target platform (such as SSL encrypted on Android)
%feature ("director") M1Connection;
%feature ("director") C1M1Tunnel;

// Set up things passed by vector
%template(BoolVector) std::vector<bool>;
%template(CharVector) std::vector<char>;
%template(IntVector) std::vector<int>;
%template(UShortVector) std::vector<uint16_t>;

%template(LogEntryVector) std::vector<Elk::LogEntry>;
%template(ArmStatusVector) std::vector<Elk::ArmStatus>;
%template(ZoneStateVector) std::vector<Elk::ZoneState>;

%template(TempDevicePair) std::pair<int, Elk::TemperatureDevice>;
%template(TempDevicePairVector) std::vector<std::pair<int, Elk::TemperatureDevice>>;

// Vector of enums needs special handling, use a single-element struct instead
%template(ChimeModeVector) std::vector<Elk::SChimeMode>;
%template(ZoneDefinitionVector) std::vector<Elk::SZoneDefinition>;

%include "ElkM1Definition.h"
%include "SwigCallbacks.h"
%include "ElkM1API.h"
%include "ElkM1Monitor.h"
%include "ElkM1AsciiAPI.h"
%include "ElkM1Connection.h"
%include "ElkM1SirenWords.h"
%include "ElkC1M1Tunnel.h"

// Using this, STL exceptions bubble up to end-user code
%exception {
  try {
    $action
  } catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}