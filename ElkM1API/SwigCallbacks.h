/*
	SwigCallbacks.h: Provides some simple (static, untemplated) callback classes to allow wrapping of function callbacks 
	for SWIG supported languages.
*/
#pragma once

class BoolCallback {
public:
	virtual void run(bool arg1) = 0;
	virtual ~BoolCallback() = 0;
};

class IntCallback {
public:
	virtual void run(int arg1) = 0;
	virtual ~IntCallback() = 0;
};