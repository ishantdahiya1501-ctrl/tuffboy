#include <Arduino.h>
#include <esp_system.h>
#include <Wire.h>
#include <WiFi.h>

// Change these values to match the final TuffBoy PCB pinout.
static const int LED_PIN = 8;
static const int I2C_SDA_PIN = 6;
static const int I2C_SCL_PIN = 7;
static const int UART_RX_PIN = 4;
static const int UART_TX_PIN = 5;

static const uint32_t UART_BAUD = 115200;
static const uint32_t LED_INTERVAL_MS = 500;
static const uint32_t STATUS_INTERVAL_MS = 5000;
static const uint32_t UART_TEST_INTERVAL_MS = 5000;
static const uint32_t WIFI_SCAN_INTERVAL_MS = 30000;
static const uint32_t SERIAL_COMMAND_TIMEOUT_MS = 250;
static const uint32_t UART_RX_DRAIN_INTERVAL_MS = 1000;
static const size_t COMMAND_BUFFER_SIZE = 64;
static const size_t EVENT_LOG_SIZE = 16;

static const char DEVICE_NAME[] = "TuffBoy";
static const char FIRMWARE_VERSION[] = "0.1.0-dev";

HardwareSerial ExpansionSerial(1);

bool ledState = false;
bool uartReady = false;
bool i2cReady = false;
bool wifiReady = false;
bool periodicWifiScanEnabled = true;
uint32_t bootTime = 0;
uint32_t lastLedChange = 0;
uint32_t lastStatus = 0;
uint32_t lastUartMessage = 0;
uint32_t lastWifiScan = 0;
uint32_t lastUartDrain = 0;
uint32_t uartMessagesSent = 0;
uint32_t uartBytesReceived = 0;
uint32_t wifiScansCompleted = 0;
uint32_t i2cScansCompleted = 0;
uint8_t lastI2CDeviceCount = 0;
int lastWiFiNetworkCount = -1;
char serialCommand[COMMAND_BUFFER_SIZE] = {};
size_t serialCommandLength = 0;

struct EventLogEntry {
  uint32_t time;
  char text[48];
};

EventLogEntry eventLog[EVENT_LOG_SIZE] = {};
size_t eventLogNext = 0;

void recordEvent(const char *text) {
  eventLog[eventLogNext].time = millis();
  strncpy(eventLog[eventLogNext].text, text, sizeof(eventLog[eventLogNext].text) - 1);
  eventLog[eventLogNext].text[sizeof(eventLog[eventLogNext].text) - 1] = '\0';
  eventLogNext = (eventLogNext + 1) % EVENT_LOG_SIZE;
}

void printSeparator(char character = '-') {
  for (uint8_t index = 0; index < 42; ++index) Serial.print(character);
  Serial.println();
}

void printHeader(const char *title) {
  Serial.println();
  printSeparator();
  Serial.print("[ ");
  Serial.print(title);
  Serial.println(" ]");
  printSeparator();
}

const char *yesNo(bool value) {
  return value ? "yes" : "no";
}

const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED: return "connected";
    case WL_NO_SSID_AVAIL: return "no SSID available";
    case WL_CONNECT_FAILED: return "connection failed";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected";
    case WL_IDLE_STATUS: return "idle";
    default: return "unknown";
  }
}

void printChipInfo() {
  printHeader("Chip information");
  Serial.print("Device name: ");
  Serial.println(DEVICE_NAME);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("Chip model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("CPU frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("Flash size: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");
  Serial.print("Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" free / ");
  Serial.print(ESP.getHeapSize());
  Serial.println(" total bytes");
  Serial.print("PSRAM: ");
  Serial.print(ESP.getPsramSize());
  Serial.println(" bytes");
  Serial.print("Sketch size: ");
  Serial.print(ESP.getSketchSize());
  Serial.print(" / ");
  Serial.print(ESP.getFreeSketchSpace());
  Serial.println(" bytes free flash space");
}

void printPinConfiguration() {
  printHeader("Pin configuration");
  Serial.print("LED: GPIO ");
  Serial.println(LED_PIN);
  Serial.print("I2C SDA: GPIO ");
  Serial.println(I2C_SDA_PIN);
  Serial.print("I2C SCL: GPIO ");
  Serial.println(I2C_SCL_PIN);
  Serial.print("Expansion UART RX: GPIO ");
  Serial.println(UART_RX_PIN);
  Serial.print("Expansion UART TX: GPIO ");
  Serial.println(UART_TX_PIN);
  Serial.println("Edit the constants at the top of this file for another PCB revision.");
}

void printMemoryStatus() {
  printHeader("Memory");
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("Minimum free heap: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes");
  Serial.print("Largest free block: ");
  Serial.print(ESP.getMaxAllocHeap());
  Serial.println(" bytes");
  Serial.print("PSRAM free: ");
  Serial.print(ESP.getFreePsram());
  Serial.println(" bytes");
}

void printStatus() {
  printHeader("TuffBoy status");
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" s");
  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());
  Serial.print("LED: ");
  Serial.println(ledState ? "on" : "off");
  Serial.print("UART ready: ");
  Serial.println(yesNo(uartReady));
  Serial.print("I2C ready: ");
  Serial.println(yesNo(i2cReady));
  Serial.print("Wi-Fi ready: ");
  Serial.println(yesNo(wifiReady));
  Serial.print("Wi-Fi status: ");
  Serial.println(wifiStatusName(WiFi.status()));
  Serial.print("Wi-Fi scans completed: ");
  Serial.println(wifiScansCompleted);
  Serial.print("I2C scans completed: ");
  Serial.println(i2cScansCompleted);
  Serial.print("UART test messages sent: ");
  Serial.println(uartMessagesSent);
  printMemoryStatus();
}

bool scanI2C() {
  printHeader("I2C bus scan");
  if (!i2cReady) {
    Serial.println("[I2C] Bus is not initialized; scan skipped.");
    recordEvent("I2C scan skipped");
    return false;
  }
  Serial.println("[I2C] Scanning addresses 0x01 through 0x7E...");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("[I2C] Device found at 0x");
      if (address < 16) Serial.print('0');
      Serial.println(address, HEX);
      ++found;
    }
  }
  if (found == 0) Serial.println("[I2C] No devices found.");
  else {
    Serial.print("[I2C] Devices found: ");
    Serial.println(found);
  }
  lastI2CDeviceCount = found;
  ++i2cScansCompleted;
  recordEvent(found == 0 ? "I2C scan complete: empty" : "I2C devices detected");
  return true;
}

bool scanWiFi() {
  printHeader("Wi-Fi scan");
  if (!wifiReady) {
    Serial.println("[Wi-Fi] Station mode is not initialized; scan skipped.");
    recordEvent("Wi-Fi scan skipped");
    return false;
  }
  Serial.println("[Wi-Fi] Scanning nearby networks...");
  WiFi.scanDelete();
  int networkCount = WiFi.scanNetworks(false, true);
  if (networkCount < 0) {
    Serial.print("[Wi-Fi] Scan failed, error ");
    Serial.println(networkCount);
    lastWiFiNetworkCount = networkCount;
    recordEvent("Wi-Fi scan failed");
    return false;
  }
  lastWiFiNetworkCount = networkCount;
  if (networkCount == 0) {
    Serial.println("[Wi-Fi] No networks found.");
    ++wifiScansCompleted;
    recordEvent("Wi-Fi scan complete: empty");
    return true;
  }
  Serial.print("[Wi-Fi] Networks found: ");
  Serial.println(networkCount);
  for (int index = 0; index < networkCount; ++index) {
    Serial.print("  ");
    Serial.print(index + 1);
    Serial.print(". ");
    Serial.print(WiFi.SSID(index));
    Serial.print("  RSSI ");
    Serial.print(WiFi.RSSI(index));
    Serial.print(" dBm  channel ");
    Serial.println(WiFi.channel(index));
  }
  WiFi.scanDelete();
  ++wifiScansCompleted;
  recordEvent("Wi-Fi scan complete");
  return true;
}

void printHelp() {
  printHeader("Command reference");
  Serial.println("status       Show system and peripheral status");
  Serial.println("info         Show chip, flash, RAM, and firmware information");
  Serial.println("memory       Show current heap and PSRAM information");
  Serial.println("pins         Show configurable GPIO assignments");
  Serial.println("wifi         Scan for nearby Wi-Fi networks");
  Serial.println("wifi auto    Toggle the periodic Wi-Fi scan");
  Serial.println("i2c          Scan the I2C bus");
  Serial.println("uart         Send an immediate UART test message");
  Serial.println("led          Toggle the status LED");
  Serial.println("events       Show recent firmware events");
  Serial.println("uptime       Print uptime in seconds");
  Serial.println("help         Show this command list");
  Serial.println("clear        Clear the terminal with blank lines");
}

void printEventLog() {
  printHeader("Recent events");
  bool hasEvents = false;
  for (size_t offset = 0; offset < EVENT_LOG_SIZE; ++offset) {
    size_t index = (eventLogNext + offset) % EVENT_LOG_SIZE;
    if (eventLog[index].text[0] == '\0') continue;
    hasEvents = true;
    Serial.print("[");
    Serial.print(eventLog[index].time / 1000);
    Serial.print("s] ");
    Serial.println(eventLog[index].text);
  }
  if (!hasEvents) Serial.println("No events recorded yet.");
}

void setLed(bool enabled) {
  ledState = enabled;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

void toggleLed() {
  setLed(!ledState);
  Serial.print("[LED] ");
  Serial.println(ledState ? "on" : "off");
  recordEvent(ledState ? "LED turned on" : "LED turned off");
}

void sendUartTestMessage() {
  if (!uartReady) {
    Serial.println("[UART] UART is not initialized; message skipped.");
    recordEvent("UART message skipped");
    return;
  }
  ExpansionSerial.println("TuffBoy UART test message");
  ++uartMessagesSent;
  Serial.println("[UART] Test message transmitted.");
  recordEvent("UART test message sent");
}

void printUptime() {
  uint32_t seconds = (millis() - bootTime) / 1000;
  Serial.print("TuffBoy has been running for ");
  Serial.print(seconds);
  Serial.println(" seconds.");
}

void handleCommand(const char *command) {
  if (strcmp(command, "status") == 0) printStatus();
  else if (strcmp(command, "info") == 0) printChipInfo();
  else if (strcmp(command, "memory") == 0) printMemoryStatus();
  else if (strcmp(command, "pins") == 0) printPinConfiguration();
  else if (strcmp(command, "wifi") == 0) scanWiFi();
  else if (strcmp(command, "wifi auto") == 0) {
    periodicWifiScanEnabled = !periodicWifiScanEnabled;
    Serial.print("[Wi-Fi] Periodic scanning: ");
    Serial.println(periodicWifiScanEnabled ? "enabled" : "disabled");
  } else if (strcmp(command, "i2c") == 0) scanI2C();
  else if (strcmp(command, "uart") == 0) sendUartTestMessage();
  else if (strcmp(command, "led") == 0) {
    toggleLed();
  } else if (strcmp(command, "events") == 0) printEventLog();
  else if (strcmp(command, "uptime") == 0) printUptime();
  else if (strcmp(command, "help") == 0) printHelp();
  else if (strcmp(command, "clear") == 0) {
    for (uint8_t line = 0; line < 30; ++line) Serial.println();
  } else if (strlen(command) > 0) {
    Serial.print("Unknown command: ");
    Serial.println(command);
    Serial.println("Type 'help' to list available commands.");
  }
}

void finishSerialCommand() {
  serialCommand[serialCommandLength] = '\0';
  for (size_t index = 0; index < serialCommandLength; ++index) {
    if (serialCommand[index] >= 'A' && serialCommand[index] <= 'Z') {
      serialCommand[index] = serialCommand[index] - 'A' + 'a';
    }
  }
  size_t first = 0;
  while (serialCommand[first] == ' ' || serialCommand[first] == '\t') ++first;
  if (first > 0) memmove(serialCommand, serialCommand + first, serialCommandLength - first + 1);
  serialCommandLength = strlen(serialCommand);
  while (serialCommandLength > 0 &&
         (serialCommand[serialCommandLength - 1] == ' ' || serialCommand[serialCommandLength - 1] == '\t')) {
    serialCommand[--serialCommandLength] = '\0';
  }
  handleCommand(serialCommand);
  serialCommandLength = 0;
  serialCommand[0] = '\0';
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      if (serialCommandLength > 0) finishSerialCommand();
    } else if (serialCommandLength < COMMAND_BUFFER_SIZE - 1 && character >= 32 && character <= 126) {
      serialCommand[serialCommandLength++] = character;
      serialCommand[serialCommandLength] = '\0';
    } else if (serialCommandLength >= COMMAND_BUFFER_SIZE - 1) {
      Serial.println("Command too long; input discarded.");
      serialCommandLength = 0;
      serialCommand[0] = '\0';
    }
  }
}

void drainExpansionUart() {
  if (!uartReady || millis() - lastUartDrain < UART_RX_DRAIN_INTERVAL_MS) return;
  lastUartDrain = millis();
  while (ExpansionSerial.available() > 0) {
    int received = ExpansionSerial.read();
    if (received >= 0) {
      ++uartBytesReceived;
      Serial.print("[UART RX] 0x");
      if (received < 16) Serial.print('0');
      Serial.println(received, HEX);
    }
  }
}

void initializeLed() {
  pinMode(LED_PIN, OUTPUT);
  setLed(false);
  Serial.print("[LED] GPIO ");
  Serial.print(LED_PIN);
  Serial.println(" configured as output.");
  recordEvent("LED initialized");
}

void initializeExpansionUart() {
  Serial.print("[UART] Initializing TX GPIO ");
  Serial.print(UART_TX_PIN);
  Serial.print(" / RX GPIO ");
  Serial.println(UART_RX_PIN);
  ExpansionSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  uartReady = true;
  Serial.println("[UART] Expansion UART ready at 115200 baud.");
  recordEvent("Expansion UART initialized");
}

void initializeI2C() {
  Serial.print("[I2C] Initializing SDA GPIO ");
  Serial.print(I2C_SDA_PIN);
  Serial.print(" / SCL GPIO ");
  Serial.println(I2C_SCL_PIN);
  i2cReady = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.print("[I2C] Initialization ");
  Serial.println(i2cReady ? "successful." : "failed.");
  recordEvent(i2cReady ? "I2C initialized" : "I2C initialization failed");
}

void initializeWiFi() {
  Serial.println("[Wi-Fi] Starting station mode.");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  wifiReady = true;
  Serial.print("[Wi-Fi] MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[Wi-Fi] Station mode ready.");
  recordEvent("Wi-Fi station mode initialized");
}

void printBootSummary() {
  printHeader("Boot complete");
  Serial.println("TuffBoy is running.");
  Serial.println("Status LED heartbeat is active.");
  Serial.println("Periodic UART and Wi-Fi tasks are active.");
  Serial.println("Type 'help' for the serial command list.");
  Serial.println();
}

void handlePeriodicTasks(uint32_t now) {
  if (now - lastLedChange >= LED_INTERVAL_MS) {
    lastLedChange = now;
    setLed(!ledState);
  }
  if (now - lastUartMessage >= UART_TEST_INTERVAL_MS) {
    lastUartMessage = now;
    sendUartTestMessage();
  }
  if (now - lastStatus >= STATUS_INTERVAL_MS) {
    lastStatus = now;
    Serial.print("[heartbeat] uptime ");
    Serial.print(now / 1000);
    Serial.print(" s, heap ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
  }
  if (periodicWifiScanEnabled && now - lastWifiScan >= WIFI_SCAN_INTERVAL_MS) {
    lastWifiScan = now;
    scanWiFi();
  }
}

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  uint32_t serialStart = millis();
  while (!Serial && millis() - serialStart < 1500) yield();
  Serial.println();
  printHeader("TuffBoy boot sequence");
  Serial.println("Device: TuffBoy ESP32-C5");
  Serial.println("Framework: Arduino");
  Serial.println("Serial console: 115200 baud");
  Serial.println("Initializing hardware...");
  initializeLed();
  initializeExpansionUart();
  initializeI2C();
  initializeWiFi();
  printChipInfo();
  scanI2C();
  scanWiFi();
  lastWifiScan = millis();
  lastStatus = millis();
  lastUartMessage = millis();
  printBootSummary();
  recordEvent("Boot sequence complete");
}

void loop() {
  uint32_t now = millis();
  handleSerialCommands();
  handlePeriodicTasks(now);
  drainExpansionUart();
  yield();
}
