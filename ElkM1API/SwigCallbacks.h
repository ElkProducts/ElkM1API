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

class BoolVectorCallback {
public:
	virtual ELKM1API void run(std::vector<bool> status) = 0;
	virtual ELKM1API ~BoolVectorCallback() = 0;
};

class KeypadFkeyStatusCallback {
public:
	virtual ELKM1API void run(Elk::KeypadFkeyStatus status) = 0;
	virtual ELKM1API ~KeypadFkeyStatusCallback() = 0;
};

class EntryExitTimeDataCallback {
public:
	virtual ELKM1API void run(Elk::EntryExitTimeData data) = 0;
	virtual ELKM1API ~EntryExitTimeDataCallback() = 0;
};

class LogDataUpdateCallback {
public:
	virtual ELKM1API void run(Elk::LogEntry) = 0;
	virtual ELKM1API ~LogDataUpdateCallback() = 0;
};

class InvalidUserCodeDataCallback {
public:
	virtual ELKM1API void run(Elk::InvalidUserCodeData) = 0;
	virtual ELKM1API ~InvalidUserCodeDataCallback() = 0;
};

class ValidUserCodeDataCallback {
public:
	virtual ELKM1API void run(Elk::ValidUserCodeData) = 0;
	virtual ELKM1API ~ValidUserCodeDataCallback() = 0;
};

class ZoneStateCallback {
public:
	virtual ELKM1API void run(Elk::ZoneState) = 0;
	virtual ELKM1API ~ZoneStateCallback() = 0;
};

class StringCallback {
public:
	virtual ELKM1API void run(std::string arg1) = 0;
	virtual ELKM1API ~StringCallback() = 0;
};