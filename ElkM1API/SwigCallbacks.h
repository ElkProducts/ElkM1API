/*
	SwigCallbacks.h: Provides some simple (static, untemplated) callback classes to allow wrapping of function callbacks 
	for SWIG supported languages.
*/
#pragma once
#include "ElkM1Definition.h"
#include <iostream>

class BoolCallback {
public:
	virtual void run(bool arg1) = 0;
};

class IntCallback {
public:
	virtual void run(int arg1) = 0;
};

class TempDeviceCallback {
public:
	virtual void run( Elk::TemperatureDevice arg1, int arg2) = 0;
};