/*
	ElkM1Definition.h: Independent file which describes data structures used in the API.
*/
#pragma once

#include <stdint.h>

namespace Elk {
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
	enum ZoneDefinition {
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

	// Workaround for SWIG vector wrapping
	struct SZoneDefinition {
		ZoneDefinition zd;
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

	struct SChimeMode {
		ChimeMode cm;
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
}