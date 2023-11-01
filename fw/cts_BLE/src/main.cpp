#include <Arduino.h>
#include <bluefruit.h>

// BLE Client Current Time Service
BLEClientCts bleCTime;

void setupBLE();
void startAdvertising();
void printCurrentTime();
void connectionCallbacks();


void startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_GENERIC_CLOCK);
  Bluefruit.Advertising.addService(bleCTime);
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void printCurrentTime() {
  uint16_t const conn_handle = 0;
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (!(conn && conn->connected() && conn->secured())) return;
  if (!bleCTime.discovered()) return;

  bleCTime.getCurrentTime();

  const char * day_of_week_str[] = { "n/a", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  
  Serial.printf("%04d-%02d-%02d ", bleCTime.Time.year, bleCTime.Time.month, bleCTime.Time.day);
  Serial.printf("%02d:%02d:%02d ", bleCTime.Time.hour, bleCTime.Time.minute, bleCTime.Time.second);
  Serial.print(day_of_week_str[bleCTime.Time.weekday]);

  int utc_offset = bleCTime.LocalInfo.timezone * 15; // in 15 minutes unit
  Serial.printf(" (UTC %+d:%02d, ", utc_offset/60, utc_offset%60);
  Serial.printf("DST %+.1f)", ((float) bleCTime.LocalInfo.dst_offset*15)/60);
  Serial.println();
}

void connect_callback(uint16_t conn_handle) {
  connectionCallbacks();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

void connection_secured_callback(uint16_t conn_handle) {
  connectionCallbacks();
}

void cts_adjust_callback(uint8_t reason) {
  const char * reason_str[] = { "Manual", "External Reference", "Change of Time Zone", "Change of DST" };
  Serial.println("iOS Device time changed due to ");
  Serial.println(reason_str[reason]);
}

void connectionCallbacks() {
  uint16_t const conn_handle = 0;
  BLEConnection* conn = Bluefruit.Connection(conn_handle);

  if (conn->secured()) {
    Serial.println("Secured");
    if (bleCTime.discovered()) {
      Serial.println("Enabling Time Adjust Notify");
      bleCTime.enableAdjust();

      Serial.print("Get Current Time chars value");
      bleCTime.getCurrentTime();

      Serial.print("Get Local Time Info chars value");
      bleCTime.getLocalTimeInfo();
      Serial.println();
    }
  } else {
    conn->requestPairing();
  }
}

void setup() {
  Serial.begin(9600);
  delay(2000);
  for(int i = 0; i < 100; i++){
    Serial.println("yaa");
  }
  Serial.println("Bluefruit52 BLE Client Current Time Example");
  Serial.println("-------------------------------------------\n");
  setupBLE();
}

void loop() {
  printCurrentTime();
  delay(1000);
  Serial.println(".........");
}

void setupBLE() {
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  Bluefruit.Security.setSecuredCallback(connection_secured_callback);
  
  bleCTime.begin();
  bleCTime.setAdjustCallback(cts_adjust_callback);

  startAdvertising();
}

