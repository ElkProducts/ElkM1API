/*
	ElkM1API.h: Provides an interface to allow you to talk to the M1 using whatever underlying protocol you choose,
	  using whatever transmission method you need.
	@author Zach Jaggi
*/

#pragma once
#ifdef _WIN32

#ifdef ELKM1API_EXPORTS
#define ELKM1API __declspec(dllexport)
#else
#define ELKM1API __declspec(dllimport)
#endif

#include <WinSock2.h>

#elif __linux__
#define ELKM1API  
#endif

#include "ElkM1Connection.h"
#include "ElkM1SirenWords.h"

// TODO: Cleanup, ensure only included at level they are needed
#include <array>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <queue>
#include <stdint.h>
#include <ctime>

namespace Elk
{

	/*
		Abstract class, inherited by M1Monitor.
		M1Monitor implements these functions as calls to its internal cache, which is filled as you request more information.
		That way, messages incidentally recieved because of other M1API instances or otherwise will still be used.
	*/
	// TODO: Replace with compile-time dynamic area size?
	class M1API
	{
	public:
		// Developer using this API implements this as a callback for when the M1 goes into programming mode. 
		std::function<void(bool)> onRPConnection;

		virtual ELKM1API void forEachConfiguredZone(std::function<void(int)> funct) = 0;
		virtual ELKM1API void forEachConfiguredKeypad(std::function<void(int)> funct) = 0;

		// TODO: These functions aren't sent for by us, but we still recieve them.
		// There needs to be some form of API call which gives us information from them.
		// AP - ignore?
		// AR - handle in monitor (ar)
		// AT - handle in monitor (at)
		// CC - handle in monitor
		// (cd - CD) - TODO - Control audio commands between XEP and audio devs. Ignore?
		// EE - handle in monitor
		// EM - ignore?
		// IC - handle in monitor
		// ir ~ IR - ?
		// TC - handle in monitor
		// XK - handle in monitor
		// ZC - handle in monitor
		// PC - handle in monitor
		// (rs) - used by touchscreen?

		// Describes whether or not an area is capable of arming.
		enum ArmUpMode {
			ARMUPMODE_NOTREADY = 0,
			ARMUPMODE_READY = 1,
			ARMUPMODE_READYFORCE = 2,
			ARMUPMODE_ARMEDEXITTIMER = 3,
			ARMUPMODE_ARMED = 4,
			ARMUPMODE_ARMEDFORCE = 5,
			ARMUPMODE_ARMEDBYPASS = 6
		};

		// Describes a state an alarm can be in.
		enum AlarmState {
			ALARMSTATE_NONE = 0,
			ALARMSTATE_ENTRANCE_DELAY = 1,
			ALARMSTATE_ALARM_ABORT_DELAY = 2,
			ALARMSTATE_FIRE = 3,
			ALARMSTATE_MEDICAL = 4,
			ALARMSTATE_POLICE = 5,
			ALARMSTATE_BURGLAR = 6,
			ALARMSTATE_AUX1 = 7,
			ALARMSTATE_AUX2 = 8,
			ALARMSTATE_AUX3 = 9,
			ALARMSTATE_AUX4 = 10,
			ALARMSTATE_CARBON_MONOXIDE = 11,
			ALARMSTATE_EMERGENCY = 12,
			ALARMSTATE_FREEZE = 13,
			ALARMSTATE_GAS = 14,
			ALARMSTATE_HEAT = 15,
			ALARMSTATE_WATER = 16,
			ALARMSTATE_FIRE_SUPERVISORY = 17,
			ALARMSTATE_VERIFY_FIRE = 18
		};

		// Describes the function of zones. Warning: Different between AZ and ZD, must reject invalid ones.
		enum ZoneDefinition{
			ZONEDEF_DISABLED = 0,
			ZONEDEF_BURGLAR_ENTRY_1 = 1,
			ZONEDEF_BURGLAR_ENTRY_2 = 2,
			ZONEDEF_BURGLAR_PERIMETER_INSTANT,
			ZONEDEF_BURGLAR_INTERIOR,
			ZONEDEF_BURGLAR_INTERIOR_FOLLOWER,
			ZONEDEF_BURGLAR_INTERIOR_NIGHT,
			ZONEDEF_BURGLAR_INTERIOR_NIGHT_DELAY,
			ZONEDEF_BURGLAR_24_HOUR,
			ZONEDEF_BURGLAR_BOX_TAMPER,
			ZONEDEF_FIRE_ALARM,
			ZONEDEF_FIRE_VERIFIED,
			ZONEDEF_FIRE_SUPERVISORY,
			ZONEDEF_AUX_ALARM_1,
			ZONEDEF_AUX_ALARM_2,
			ZONEDEF_KEY_FOB,
			ZONEDEF_NON_ALARM,
			ZONEDEF_CARBON_MONOXIDE,
			ZONEDEF_EMERGENCY_ALARM,
			ZONEDEF_FREEZE_ALARM,
			ZONEDEF_GAS_ALARM,
			ZONEDEF_HEAT_ALARM,
			ZONEDEF_MEDICAL_ALARM,
			ZONEDEF_POLICE_ALARM,
			ZONEDEF_POLICE_NO_INDICATION,
			ZONEDEF_WATER_ALARM,
			ZONEDEF_KEY_MOMENTARY_ARMDISARM,
			ZONEDEF_KEY_MOMENTARY_ARM_AWAY,
			ZONEDEF_KEY_MOMENTARY_ARM_STAY,
			ZONEDEF_KEY_MOMENTARY_DISARM,
			ZONEDEF_KEY_TOGGLE,
			ZONEDEF_MUTE_AUDIBLES,
			ZONEDEF_POWER_SUPERVISORY,
			ZONEDEF_TEMPERATURE,
			ZONEDEF_ANALOG_ZONE,
			ZONEDEF_PHONE_KEY,
			ZONEDEF_INTERCOM_KEY
		};

		// Reply whether or not the user code change was successful.
		enum UserCodeSuccess
		{
			USERCODE_CHANGE_SUCCESSFUL,
			USERCODE_USER_DUPLICATE,
			USERCODE_UNAUTHORIZED
		};

		enum ArmMode {
			ARM_DISARMED = 0,
			ARM_AWAY = 1,
			ARM_STAY = 2,
			ARM_STAYINSTANT = 3,
			ARM_NIGHT = 4,
			ARM_NIGHTINSTANT = 5,
			ARM_VACATION = 6,
			ARM_AWAYNEXT = 7,
			ARM_STAYNEXT = 8
		};

		enum Weekday{
			Sunday = 1,
			Monday,
			Tuesday,
			Wednesday,
			Thursday,
			Friday,
			Saturday
		};

		enum ChimeMode {
			CHIMEMODE_OFF,
			CHIMEMODE_CHIMEONLY,
			CHIMEMODE_VOICEONLY,
			CHIMEMODE_BOTH
		};

		enum FKEY {
			FKEY_1,
			FKEY_2,
			FKEY_3,
			FKEY_4,
			FKEY_5,
			FKEY_6,
			FKEY_STAR,
			FKEY_CHIME,
			FKEY_NONE // Get chime state
		};

		// Clear automatically, clear with star key, or display until the specified timeout.
		enum clearMethod{
			CLEAR = 0,
			CLEAR_WITH_STAR = 1,
			CLEAR_DISPLAY_UNTIL_TIMEOUT = 2
		};

		enum TextDescriptionType {
			TEXT_ZoneName,
			TEXT_AreaName,
			TEXT_UserName,
			TEXT_KeypadName,
			TEXT_OutputName,
			TEXT_TaskName,
			TEXT_TelephoneName,
			TEXT_LightName,
			TEXT_AlarmDurationName,
			TEXT_CustomSettings,
			TEXT_CounterName,
			TEXT_ThermostatName,
			TEXT_FKEY1,
			TEXT_FKEY2,
			TEXT_FKEY3,
			TEXT_FKEY4,
			TEXT_FKEY5,
			TEXT_FKEY6,
			TEXT_AudioZoneName,
			TEXT_AudioSourceName
		};

		enum PhysicalZoneState{
			PZS_UNCONFIGURED,
			PZS_OPEN,
			PZS_EOL,
			PZS_SHORT
		};

		enum LogicalZoneState{
			LZS_NORMAL,
			LZS_TROUBLE,
			LZS_VIOLATED,
			LZS_BYPASSED
		};

		enum TemperatureDevice{
			TEMPDEVICE_ZONE = 0,
			TEMPDEVICE_KEYPAD = 1,
			TEMPDEVICE_THERMOSTAT = 2
		};

		// Holds the structure of alarm and arm status, for one Area.
		struct ArmStatus {
			ArmMode mode;
			ArmUpMode isReady;
			AlarmState alarm;
		};

		// Structure describing characteristics of one audio device.
		struct AudioData {
			bool zoneIsOn;
			bool loudness;
			bool doNotDisturb;
			// Zero-indexed source
			int source;
			int volume;
			int bass;
			int treble;
			int balance;
			enum PartyMode {
				PARTYMODE_OFF,
				PARTYMODE_ON,
				PARTYMODE_MASTER
			} partyMode;
		};

		// C/F depends on internal settings.
		struct TemperatureData {
			std::array<int, 16> keypadTemps;
			std::array<int, 16> zoneSensorTemps;
		};

		// TODO: Use a proper time format structure
		struct LogEntry{
			int event;
			// User, zone number, etc
			int eventSubjectNumber;
			int area;
			int hour;
			int minute;
			int month;
			int day;
			int index;
			int year;
			Weekday dayOfWeek;
		};

		// Zone physical and definition state.
		struct ZoneState {
			PhysicalZoneState physicalState;
			LogicalZoneState logicalState;
		};

		// TODO: Replace with proper time format structure.
		struct RTCData {
			int seconds;
			int minutes;
			int hours;
			Weekday weekday;
			int day;
			int month;
			int year;
			bool twelveHourClock;
			bool dayBeforeMonth;
		};

		// Data returned from any thermostat.
		struct ThermostatData {
			enum ThermostatMode {
				OFF,
				HEAT,
				COOL,
				AUTO,
				EMERGENCY_HEAT
			} mode;
			bool holdCurrentTemperature;
			bool fanOn;
			int temperature;
			int heatSetPoint;
			int coolSetPoint;
			int humidity;
		};

		struct SystemTroubleStatus {
			bool ACFail;
			bool boxTamper;
			bool communicationError;
			bool EEPROMError;
			bool lowBattery;
			bool overCurrent;
			bool telephoneFault;
			bool output2;
			bool missingKeypad;
			bool zoneExpander;
			bool outputExpander;
			bool RPRemoteAccess;
			bool commonAreaNotArmed;
			bool flashMemoryError;
			bool securityAlert;
			bool serialPortExpander;
			bool lostTransmitter;
			bool GESmokeCleanMe;
			bool ethernet;
		};

		struct KeypadFkeyStatus {
			enum FkeyIllumination{
				FKEY_OFF,
				FKEY_ON,
				FKEY_BLINKING
			} illumination[6];
			bool codeRequiredForBypass;
		};

		struct UserCodeAccess {
			enum CodeType {
				CODETYPE_USER = 1,
				CODETYPE_MASTER,
				CODETYPE_INSTALLER,
				CODETYPE_ELKRP
			} codetype;
			bool usesCelcius;
			uint8_t validAreas;
		};

		// Arm or disarm a partition using the specified user code.
		virtual ELKM1API void armDisarm(int partition, ArmMode mode, std::string userCode) = 0;
		// Retrieve arm status for all partitions.
		virtual ELKM1API std::array<ArmStatus, 8> getArmStatus() = 0;
		// Get zone alarms. If not 'ZONEDEF_DISABLED' then the area is in alarm.
		virtual ELKM1API std::array<ZoneDefinition, 208> getZoneAlarms() = 0;
		// Retrieve the audio data for a device in audioZone 0-15
		virtual ELKM1API AudioData getAudioData(int audioZone) = 0;
		// Enable or disable a control output.
		virtual ELKM1API void enableControlOutput(int output, uint16_t seconds) = 0;
		virtual ELKM1API void disableControlOutput(int output) = 0;
		virtual ELKM1API void toggleControlOutput(int output) = 0;
		// Retrieve the state of all control outputs.
		virtual ELKM1API std::array<bool, 208> getControlOutputs() = 0;
		// TODO: Needs format specifier. 
		// Note: If using even TWO custom values, it's much faster to use the second command and iterate.
		// Retrieve/set custom values by index, or retrieve all.
		virtual ELKM1API uint16_t getCustomValue(int index) = 0;
		virtual ELKM1API std::array<uint16_t, 20> getCustomValues() = 0;
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
		virtual ELKM1API std::array<int, 16> getKeypadAreas() = 0;
		// Get status of keys on keypads
		virtual ELKM1API KeypadFkeyStatus getKeypadFkeyStatus(int keypad) = 0;
		// Press a function key.
		virtual ELKM1API std::array<ChimeMode, 8> pressFunctionKey(int keypad, FKEY key) = 0;
		// Collect log data. Note: getLogs() can take up to a minute.
		virtual ELKM1API LogEntry getLogData(int index) = 0;
		virtual ELKM1API std::array<M1API::LogEntry, 511> getLogs() = 0;
		// Set log data. TODO: Replace logType and eventType with enums
		virtual ELKM1API void setLogData(int logType, int eventType, int zoneNumber, int area) = 0;
		// Execute lighting control commands.
		virtual ELKM1API void executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn) = 0;
		// Turn a PLC device on or off.
		virtual ELKM1API void setPLCState(char houseCode, int unitCode, bool state) = 0;
		virtual ELKM1API void togglePLCState(char houseCode, int unitCode) = 0;
		// Get the status of lights. 0, 1 = on or off, 2-99 = dim setting.
		virtual ELKM1API std::array<int, 64> getPLCStatus(int bank) = 0;
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
		virtual ELKM1API std::array<int, 16> getTemperatures(TemperatureDevice type) = 0;
		// Speak a word or phrase by index. TODO: Replace with an enum
		virtual ELKM1API void speakWord(SirenWord word) = 0;
		virtual ELKM1API void speakPhrase(SirenPhrase phrase) = 0;
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
		virtual ELKM1API std::array<int, 3> getM1VersionNumber() = 0;
		// Bypass a zone, or unbypass all zones in an area.
		virtual ELKM1API bool zoneBypass(int zone, std::string pinCode) = 0;
		virtual ELKM1API bool setAreaBypass(int area, std::string pinCode, bool bypassed) = 0;
		// Get the zone definitions.
		virtual ELKM1API std::array<ZoneDefinition, 208> getZoneDefinitions() = 0;
		// Get the zone areas, 0-7. 
		virtual ELKM1API std::array<int, 208> getZonePartitions() = 0;
		// Get the zone statuses.
		virtual ELKM1API std::array<ZoneState, 208> getZoneStatuses() = 0;
		// Get the voltage of defined zones.
		virtual ELKM1API float getZoneVoltage(int zone) = 0;

	};


}