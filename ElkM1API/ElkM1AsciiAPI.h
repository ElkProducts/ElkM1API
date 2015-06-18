/*
	ElkM1AsciiAPI.h: Provides an extension of the M1API class to cover ASCII-specific functions.
	@author Zach Jaggi
*/
#pragma once
#include "ElkM1Monitor.h"
#include <unordered_map>

namespace Elk {
	class M1AsciiAPI : public M1Monitor {
	private:
		// TODO: Message could easily have an identifier/contents split.
		// Message for easy output/handle of ASCII messages
		class AsciiMessage{
			std::string message;
		public:
			std::string to_string() {
				return message;
			}
			// Call to use as contents.
			AsciiMessage(std::vector<char>& other) {
				// Calls copy constructor.
				std::vector<char> cons(other);
				cons.push_back('\0');
				message = std::string(&cons[0]);
			}
			AsciiMessage(const char *other) {
				message = std::string(other);
			}
			void push_back(char character) {
				message.push_back(character);
			}
			void operator+= (const char *other) {
				message += other;
			}
			void operator+= (std::string& other) {
				message += other;
			}
			void operator+= (std::vector<char> other) {
				message.insert(message.end(), other.begin(), other.end());
			}
			/*
				Call to create an instance of a class from a full packet.
				TODO: Check checksum and length portions.
			*/
			static AsciiMessage fromTransmission(std::vector<char> other) {
				std::string message;
				message.insert(message.begin(), other.begin() + 2, other.end() - 4);
				return AsciiMessage(message.c_str());
			}
			std::vector<char> getTransmittable() {
				std::vector<char> nMessage(message.begin(), message.end());
				std::vector<char> lbytes = toAsciiHex(message.size() + 2, 2);
				nMessage.insert(nMessage.begin(), lbytes.begin(), lbytes.end());
				for (char c : genChecksum(nMessage))
					nMessage.push_back(c);
				for (char c : {'\r', '\n'})
					nMessage.push_back(c);
				return nMessage;
			}
		};

		bool versionAtLeast(int major, int minor, int release);
		ELKM1API std::vector<char> cutMessage(std::vector<char>& buffer);
		ELKM1API void handleMessage(std::vector<char> message);
		// Building a hashed function table allows things to be called in O(1) time.
		std::unordered_map < std::string, std::function<void(std::string&)>> handleMessageTable;
		static std::vector<char> genChecksum(const std::vector<char>& message);
		static std::vector<char> toAsciiHex(int value, int length);
		static std::vector<char> toAsciiDec(int value, int length);
		void fillFunctionTable();
		template <typename T>
		T cacheRequest(M1Monitor::cacheObject<T>& cacheObj, AsciiMessage& request, bool ignoreCache, int timeoutMillis);
		template <typename T>
		T cacheExistsRequest(M1Monitor::cacheObject<T>& cacheObj, AsciiMessage& request);
	public:
		std::function<void(bool)> onRPConnection;
		ELKM1API M1AsciiAPI(std::shared_ptr<M1Connection> conn);
		ELKM1API std::array<M1API::LogEntry, 511> getLogs();
		ELKM1API void collectAllNames();
		ELKM1API void forEachConfiguredZone(std::function<void(int)> funct);
		ELKM1API void forEachConfiguredKeypad(std::function<void(int)> funct);
		ELKM1API void forEachConfiguredTempDevice(std::function<void(TemperatureDevice, int)>);
		ELKM1API AudioData getAudioData(int audioZone);
		ELKM1API bool setAreaBypass(int area, std::string pinCode, bool bypassed);
		ELKM1API void setLogData(int logType, int eventType, int zoneNumber, int area);
		ELKM1API bool zoneBypass(int zone, std::string pinCode);
		ELKM1API float getZoneVoltage(int zone);
		ELKM1API float getZoneVoltage(int zone, bool ignoreCache, int timeoutMillis);
		ELKM1API int getLightingStatus(int device);
		ELKM1API int getLightingStatus(int device, bool ignoreCache, int timeoutMillis);
		ELKM1API int getTemperature(TemperatureDevice type, int device);
		ELKM1API std::array<int, 16> getTemperatures(TemperatureDevice type);
		ELKM1API KeypadFkeyStatus getKeypadFkeyStatus(int keypad);
		ELKM1API LogEntry getLogData(int index);
		ELKM1API std::array<int, 64> getPLCStatus(int bank);
		ELKM1API RTCData getRTCData();
		ELKM1API RTCData setRTCData(RTCData newData);
		ELKM1API std::array<ArmStatus, 8> getArmStatus();
		ELKM1API std::array<ArmStatus, 8> getArmStatus(bool ignoreCache, int timeoutMillis);
		ELKM1API std::array<bool, 208> getControlOutputs();
		ELKM1API std::array<ChimeMode, 8> pressFunctionKey(int keypad, FKEY key);
		ELKM1API std::array<int, 16> getKeypadAreas();
		ELKM1API std::array<int, 208> getZonePartitions();
		ELKM1API std::array<int, 3> getM1VersionNumber();
		ELKM1API std::array<uint16_t, 20> getCustomValues();
		ELKM1API std::array<ZoneDefinition, 208> getZoneAlarms();
		ELKM1API std::array<ZoneDefinition, 208> getZoneDefinitions();
		ELKM1API std::array<ZoneState, 208> getZoneStatuses();
		ELKM1API std::string getTextDescription(TextDescriptionType type, int index);
		ELKM1API std::vector<char> getOmnistat2Data(std::vector<char> request);
		ELKM1API SystemTroubleStatus getSystemTroubleStatus();
		ELKM1API ThermostatData getThermostatData(int index);
		ELKM1API ThermostatData setThermostatData(int index, int value, int element);
		ELKM1API uint16_t getCounterValue(int counter);
		ELKM1API uint16_t getCounterValue(int counter, bool ignoreCache, int timeoutMillis);
		ELKM1API uint16_t getCustomValue(int index);
		ELKM1API uint16_t getCustomValue(int index, bool ignoreCache, int timeoutMillis);
		ELKM1API uint16_t setCounterValue(int counter, uint16_t value);
		ELKM1API UserCodeAccess getUserCodeAccess(std::string userCode);
		ELKM1API UserCodeSuccess requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask);
		ELKM1API void activateTask(int taskNumber);
		ELKM1API void armDisarm(int partition, ArmMode mode, std::string userCode);
		ELKM1API void disableControlOutput(int output);
		ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string text);
		ELKM1API void displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string lineOne, std::string lineTwo);
		ELKM1API void enableControlOutput(int output, uint16_t seconds);
		ELKM1API void executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn);
		ELKM1API void setCustomValue(int index, uint16_t value);
		ELKM1API void setPLCState(char houseCode, int unitCode, bool state);
		ELKM1API void speakPhrase(SirenPhrase phrase);
		ELKM1API void speakWord(SirenWord word);
		ELKM1API void toggleControlOutput(int output);
		ELKM1API void togglePLCState(char houseCode, int unitCode);
	};
}