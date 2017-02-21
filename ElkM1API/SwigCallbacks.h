/*
	SwigCallbacks.h: Provides some simple (static, untemplated) callback classes to allow wrapping of function callbacks 
	for SWIG supported languages.
*/
#pragma once
#include "ElkM1Definition.h"
#include <vector>

class BoolCallback {
public:
	virtual ELKM1API void run(bool arg1) = 0;
	virtual ELKM1API ~BoolCallback() = 0;
};

class IntCallback {
public:
	virtual ELKM1API void run(int arg1) = 0;
	virtual ELKM1API ~IntCallback() = 0;
};

class ArmStatusVectorCallback {
public:
	virtual ELKM1API void run(std::vector<Elk::ArmStatus> status) = 0;
	virtual ELKM1API ~ArmStatusVectorCallback() = 0;
};

class StringCallback {
public:
	virtual ELKM1API void run(std::string arg1) = 0;
	virtual ELKM1API ~StringCallback() = 0;
};