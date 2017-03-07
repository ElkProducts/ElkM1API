/*
	ElkM1API.h: Provides an interface to allow you to talk to the M1 using whatever underlying protocol you choose,
	  using whatever transmission method you need.
	@author Zach Jaggi
*/

#pragma once
#include "ElkM1Definition.h"
#include "ElkM1Connection.h"
#include "ElkM1SirenWords.h"
#include "SwigCallbacks.h"

// TODO: Cleanup, ensure only included at level they are needed
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <queue>
#include <stdint.h>
#include <ctime>
#include <vector>
#include <memory>

namespace Elk
{

	/*
		Abstract class, inherited by M1Monitor.
		M1Monitor implements these functions as calls to its internal cache, which is filled as you request more information.
		That way, messages incidentally recieved because of other M1API instances or otherwise will still be used.
	*/
	class M1API
	{
	public:
		// Developer using this API implements this as a callback for when the M1 goes into programming mode. 
		std::shared_ptr<BoolCallback> onRPConnection;
		std::shared_ptr<ArmStatusVectorCallback> onArmStatusChange;
		std::shared_ptr<StringCallback> onDebugOutput;
		std::shared_ptr<KeypadFkeyStatusCallback> onKeypadFkeyStatusChange;
		std::shared_ptr<BoolVectorCallback> onOutputStatusChange;
		std::shared_ptr<EntryExitTimeDataCallback> onEntryExitTimerChange;
		std::shared_ptr<LogDataUpdateCallback> onLogDataUpdate;
		std::shared_ptr<InvalidUserCodeDataCallback> onInvalidUserCodeEntered;
		std::shared_ptr<ValidUserCodeDataCallback> onValidUserCodeEntered;
		std::shared_ptr<LightingDataCallback> onLightingDataUpdate;
		std::shared_ptr<X10DataCallback> onX10DataUpdate;

		virtual ELKM1API std::vector<int> getConfiguredZones() = 0;
		virtual ELKM1API std::vector<int> getConfiguredKeypads() = 0;
		virtual ELKM1API std::vector<std::pair<int, Elk::TemperatureDevice>> getConfiguredTempDevices() = 0;

		// Arm or disarm a partition using the specified user code.
		virtual ELKM1API void armDisarm(int partition, ArmMode mode, std::string userCode) = 0;
		// Retrieve arm status for all partitions.
		virtual ELKM1API std::vector<ArmStatus> getArmStatus() = 0;
		// Get zone alarms. If not 'ZONEDEF_DISABLED' then the area is in alarm.
		virtual ELKM1API std::vector<Elk::SZoneDefinition> getZoneAlarms() = 0;
		// Retrieve the audio data for a device in audioZone 0-15
		virtual ELKM1API AudioData getAudioData(int audioZone) = 0;
		// Enable or disable a control output.
		virtual ELKM1API void enableControlOutput(int output, uint16_t seconds) = 0;
		virtual ELKM1API void disableControlOutput(int output) = 0;
		virtual ELKM1API void toggleControlOutput(int output) = 0;
		// Retrieve the state of all control outputs.
		virtual ELKM1API std::vector<bool> getControlOutputs() = 0;
		// TODO: Needs format specifier. 
		// Retrieve/set custom values by index, or retrieve all.
		virtual ELKM1API uint16_t getCustomValue(int index) = 0;
		virtual ELKM1API std::vector<uint16_t> getCustomValues() = 0;
		virtual ELKM1API void setCustomValue(int index, uint16_t value) = 0;
		// Try to change the user code.
		virtual ELKM1API UserCodeSuccess requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask) = 0;
		// Get or set counters by value.
		virtual ELKM1API uint16_t getCounterValue(int counter) = 0;
		virtual ELKM1API uint16_t setCounterValue(int counter, uint16_t value) = 0;
		// Display LCD text to area for displayTime seconds. Lines will be truncated if >16 characters.
		virtual ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string text) = 0;
		virtual ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string lineOne, std::string lineTwo) = 0;
		// Get device lighting status, dev 0-255. 
		virtual ELKM1API int getLightingStatus(int device) = 0;
		// Get area each keypad is assigned to. -1 if undefined.
		virtual ELKM1API std::vector<int> getKeypadAreas() = 0;
		// Get status of keys on keypads
		virtual ELKM1API KeypadFkeyStatus getKeypadFkeyStatus(int keypad) = 0;
		// Press a function key.
		virtual ELKM1API std::vector<Elk::SChimeMode> pressFunctionKey(int keypad, FKEY key) = 0;
		// Collect log data. Note: getLogs() can take up to a minute.
		virtual ELKM1API LogEntry getLogData(int index) = 0;
		virtual ELKM1API std::vector<LogEntry> getLogs() = 0;
		// Set log data. TODO: Replace logType and eventType with enums
		virtual ELKM1API void setLogData(int logType, int eventType, int zoneNumber, int area) = 0;
		// Execute lighting control commands.
		virtual ELKM1API void executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn) = 0;
		// Turn a PLC device on or off.
		virtual ELKM1API void setPLCState(char houseCode, int unitCode, bool state) = 0;
		virtual ELKM1API void togglePLCState(char houseCode, int unitCode) = 0;
		// Get the status of lights. 0, 1 = on or off, 2-99 = dim setting.
		virtual ELKM1API std::vector<int> getPLCStatus(int bank) = 0;
		// Get the current realtime clock data.
		virtual ELKM1API RTCData getRTCData() = 0;
		// Set the realtime clock data.
		virtual ELKM1API RTCData setRTCData(RTCData newData) = 0;
		// Retrieve a text description. 
		virtual ELKM1API std::string getTextDescription(TextDescriptionType type, int index) = 0;
		// Get the trouble status of the entire system.
		virtual ELKM1API SystemTroubleStatus getSystemTroubleStatus() = 0;
		// Get varying forms of temperature.
		virtual ELKM1API int getTemperature(TemperatureDevice type, int device) = 0;
		virtual ELKM1API std::vector<int> getTemperatures(TemperatureDevice type) = 0;
		// Speak a word or phrase by index. TODO: Replace with an enum
		virtual ELKM1API void speakWord(Elk::SirenWord word) = 0;
		virtual ELKM1API void speakPhrase(Elk::SirenPhrase phrase) = 0;
		// Collect omnistat 2 data, based on request given. Needs omnistat protocol knowledge.
		virtual ELKM1API std::vector<char> getOmnistat2Data(std::vector<char> request) = 0;
		// Start a task.
		virtual ELKM1API void activateTask(int taskNumber) = 0;
		// Get more complex thermostat data.
		virtual ELKM1API ThermostatData getThermostatData(int index) = 0;
		// Set certain characteristics of thermostat data. TODO: enumerate
		virtual ELKM1API ThermostatData setThermostatData(int index, int value, int element) = 0;
		// Get what areas a certain usercode can access.
		virtual ELKM1API UserCodeAccess getUserCodeAccess(std::string userCode) = 0;
		// Get the version number of the M1.
		virtual ELKM1API std::vector<int> getM1VersionNumber() = 0;
		// Bypass a zone, or unbypass all zones in an area.
		virtual ELKM1API bool zoneBypass(int zone, std::string pinCode) = 0;
		virtual ELKM1API bool setAreaBypass(int area, std::string pinCode, bool bypassed) = 0;
		// Get the zone definitions.
		virtual ELKM1API std::vector<Elk::SZoneDefinition> getZoneDefinitions() = 0;
		// Get the zone areas, 0-7. 
		virtual ELKM1API std::vector<int> getZonePartitions() = 0;
		// Get the zone statuses.
		virtual ELKM1API std::vector<ZoneState> getZoneStatuses() = 0;
		// Get the voltage of defined zones.
		virtual ELKM1API float getZoneVoltage(int zone) = 0;
		// TODO: add support for these ASCII commands:
		// TC, ZB, ZC, cw, rr

		// There are no plans to implement support for the following commands:
		// AP, AR, CD, EM, IP, IR, RE, XB, XK, ar, cd, ip, ir, xk, zt 

		// These commands are deprecated and will not be supported by the API:
		// AT, DK, NS, NZ, at
	};
}