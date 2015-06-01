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
		std::function<void(bool)> rpConnectionTrigger;

		virtual ELKM1API void forEachConfiguredZone(std::function<void(int)> funct) = 0;

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
		// IE - handle in monitor
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
			STAR,
			CHIME,
			NONE // Get chime state
		};

		// Clear automatically, clear with star key, or display until the specified timeout.
		enum clearMethod{
			CLEAR = 0,
			CLEAR_WITH_STAR = 1,
			CLEAR_DISPLAY_UNTIL_TIMEOUT = 2
		};

		enum TextDescriptionType {
			TDT_ZoneName,
			TDT_AreaName,
			TDT_UserName,
			TDT_KeypadName,
			TDT_OutputName,
			TDT_TaskName,
			TDT_TelephoneName,
			TDT_LightName,
			TDT_AlarmDurationName,
			TDT_CustomSettings,
			TDT_CounterName,
			TDT_ThermostatName,
			TDT_FKEY1,
			TDT_FKEY2,
			TDT_FKEY3,
			TDT_FKEY4,
			TDT_FKEY5,
			TDT_FKEY6,
			TDT_AudioZoneName,
			TDT_AudioSourceName
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
			bool zonePower;
			bool loudness;
			bool doNotDisturb;
			int zone;
			int source;
			int volume;
			int base;
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

		// Lighting control status.
		struct PLCStatus{
			int bank;
			std::array<int, 64> dimLevels;
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

		// TODO: Replace with bitmask
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
			// TODO: Replace with bitmask
			enum FkeyIllumination{
				FKEY_OFF,
				FKEY_ON,
				FKEY_BLINKING
			} illumination[6];
			bool codeRequiredForBypass;
		};

		struct UserCodeAccess {
			enum CodeType {
				CODETYPE_USER,
				CODETYPE_MASTER,
				CODETYPE_INSTALLER,
				CODETYPE_ELKRP
			} codetype;
			bool usesCelcius;
			uint8_t validAreas;
		};

		// (a#) Arm or disarm a partition using the specified user code.
		virtual ELKM1API void armDisarm(int partition, ArmMode mode, std::string userCode) = 0;
		// (as - AS) Retrieve arm status for all partitions.
		virtual ELKM1API std::array<ArmStatus, 8> getArmStatus() = 0;
		// (az - AZ) If not 'ZONEDEF_DISABLED' then the area is in alarm.
		virtual ELKM1API std::array<ZoneDefinition, 208> getZoneAlarms() = 0;
		// (ca - CA) Retrieve the audio data for a device on zone.
		virtual ELKM1API AudioData getAudioData(int zone) = 0;
		// (cf, cn) Enable or disable a control output.
		virtual ELKM1API void enableControlOutput(int output, int seconds) = 0;
		virtual ELKM1API void disableControlOutput(int output) = 0;
		// (ct) Toggle a control output.
		virtual ELKM1API void toggleControlOutput(int output) = 0;
		// (cs - CS)
		virtual ELKM1API std::array<bool, 208> getControlOutputs() = 0;
		// (cr - CR) TODO: Needs format specifier. Note: If using even TWO custom values, it's much faster to use the second command and iterate.
		virtual ELKM1API uint16_t getCustomValue(int index) = 0;
		virtual ELKM1API std::array<uint16_t, 20> getCustomValues() = 0;
		// (cw)
		virtual ELKM1API void setCustomValue(int index, uint16_t value) = 0;
		// (cu - CU) Ensure that the return we get from this happens after we sent the request, and the user matches.  
		virtual ELKM1API UserCodeSuccess requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask) = 0;
		// (cv - CV) Only available on M1 ver>4.1.11
		virtual ELKM1API uint16_t getCounterValue(int counter) = 0;
		// (cx - CV)
		virtual ELKM1API uint16_t setCounterValue(int counter, uint16_t value) = 0;
		// (dm) Display LCD text to area for displayTime seconds. Lines will be truncated if >16 characters.
		virtual ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string text) = 0;
		virtual ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string lineOne, std::string lineTwo) = 0;
		// (ds - DS) Get device lighting status, dev 0-255. M1 ver>4.3.9.
		virtual ELKM1API int getLightingStatus(int device) = 0;
		// (ka - KA) Get area each keypad is assigned to. -1 if undefined.
		virtual ELKM1API std::array<int, 16> getKeypadAreas() = 0;
		// (kc - KC) - Also gives us chime mode, for some reason, on m1ver >4.3.2
		virtual ELKM1API KeypadFkeyStatus getKeypadFkeyStatus(int keypad) = 0;
		// (kf - KF)
		virtual ELKM1API std::array<ChimeMode, 8> pressFunctionKey(int keypad, FKEY key) = 0;
		// (ld - LD)
		virtual ELKM1API LogEntry getLogData(int index) = 0;
		virtual ELKM1API std::array<M1API::LogEntry, 511> getLogs() = 0;
		// (le) - TODO: Replace logType and eventType with enums
		virtual ELKM1API void setLogData(int logType, int eventType, int zoneNumber, int area) = 0;
		// (pc)
		virtual ELKM1API void executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn) = 0;
		// (pf, pn) Turn a PLC device on or off.
		virtual ELKM1API void setPLCState(char houseCode, int unitCode, bool state) = 0;
		// (pt)
		virtual ELKM1API void togglePLCState(char houseCode, int unitCode) = 0;
		// (ps - PS)
		virtual ELKM1API PLCStatus getPLCStatus() = 0;
		// (rr - RR)
		virtual ELKM1API RTCData getRTCData() = 0;
		// (rw)
		virtual ELKM1API RTCData setRTCData(RTCData newData) = 0;
		// (sd - SD)
		virtual ELKM1API std::string getTextDescription(TextDescriptionType type, int index) = 0;
		// (ss - SS)
		virtual ELKM1API SystemTroubleStatus getSystemTroubleStatus() = 0;
		// (st - ST), (lw - LW), (+tr)
		virtual ELKM1API int getTemperature(TemperatureDevice type, int device) = 0;
		virtual ELKM1API std::array<int, 16> getTemperatures(TemperatureDevice type) = 0;
		// (sw, sp) TODO: Replace with an enum
		virtual ELKM1API void speakWord(int wordIndex) = 0;
		virtual ELKM1API void speakPhrase(int phraseIndex) = 0;
		// (t2 - T2)
		virtual ELKM1API std::vector<char> getOmnistat2Data(std::vector<char> request) = 0;
		// (tn)
		virtual ELKM1API void activateTask(int taskNumber) = 0;
		// (tr - TR)
		virtual ELKM1API ThermostatData getThermostatData(int index) = 0;
		// (ts)
		virtual ELKM1API ThermostatData setThermostatData(int index, int value, int element) = 0;
		// (ua - UA)
		virtual ELKM1API UserCodeAccess getUserCodeAccess(std::string userCode) = 0;
		// (vn - VN)
		virtual ELKM1API std::array<int, 3> getM1VersionNumber() = 0;
		// (zb - ZB)
		virtual ELKM1API bool zoneBypass(int zone, std::string pinCode) = 0;
		virtual ELKM1API bool setAreaBypass(int area, std::string pinCode, bool bypassed) = 0;
		// (zd - ZD)
		virtual ELKM1API std::array<ZoneDefinition, 208> getZoneDefinitions() = 0;
		// (zp - ZP)
		virtual ELKM1API std::array<int, 208> getZonePartitions() = 0;
		// (zs - ZS)
		virtual ELKM1API std::array<ZoneState, 208> getZoneStatuses() = 0;
		// (zv - ZV)
		virtual ELKM1API float getZoneVoltage(int zone) = 0;

	};
}