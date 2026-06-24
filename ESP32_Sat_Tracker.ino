#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>
#include <Sgp4.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <HTTPClient.h>
#include "qrcodegen.h"
#include "secrets.h"

// ================================================================
//  MPU6050  GY-521   VCC->3.3V  GND->GND  SDA->21  SCL->22
// ================================================================
#include <Wire.h>
#include "Config.h"
#include "Celestial_Math.h"

// -- Kalman filter state (replaces complementary filter) ---------
// Two independent 2-state Kalman filters, one per axis.
// State vector: [angle, gyro_bias]
// Measurement:  accelerometer-derived angle
struct KalmanAxis {
  float angle   = 0;   // estimated angle (deg)
  float bias    = 0;   // estimated gyro bias (deg/s)
  float P[2][2] = {{1,0},{0,1}}; // error covariance

  // Process noise covariance
  static constexpr float Q_angle = 0.001f;
  static constexpr float Q_bias  = 0.003f;
  // Measurement noise covariance
  static constexpr float R_meas  = 0.03f;

  float update(float newAngle, float newRate, float dt) {
    // Predict
    angle += dt * (newRate - bias);
    P[0][0] += dt*(dt*P[1][1] - P[0][1] - P[1][0] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;
    // Update
    float S  = P[0][0] + R_meas;
    float K0 = P[0][0] / S;
    float K1 = P[1][0] / S;
    float y  = newAngle - angle;
    angle += K0 * y;
    bias  += K1 * y;
    float P00 = P[0][0];
    float P01 = P[0][1];
    P[0][0] -= K0 * P00;
    P[0][1] -= K0 * P01;
    P[1][0] -= K1 * P00;
    P[1][1] -= K1 * P01;
    return angle;
  }

  void seed(float a, float b=0){ angle=a; bias=b; }
};

static KalmanAxis kalPitch, kalRoll;

// -- IMU public outputs ------------------------------------------
volatile float imuPitch = 0, imuRoll = 0;
volatile bool  imuOK    = false;

// -- IMU internals -----------------------------------------------
static float _lpfAx=0,_lpfAy=0,_lpfAz=0;
static bool  _seeded=false;
static float _gBiasX=0,_gBiasY=0;
static float _aBiasX=0,_aBiasY=0,_aBiasZ=0;
static unsigned long _lastUs=0;

// â”€â”€ IMU error history for TFT drift graph â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define ERR_HIST 60
static float errHistory[ERR_HIST];
static int   errHistIdx=0;
static bool  errHistFull=false;

// â”€â”€ Button â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Screen 0: Main HUD (existing)
// Screen 1: Pass Schedule
// Screen 2: IMU Drift Graph
// Screen 3: Satellite Footprint
// Screen 4: Weather
volatile int  currentScreen = 0;
volatile bool screenChanged  = false;
static unsigned long lastBtnMs = 0;

// â”€â”€ Satellite queue (feature 4) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct QueuedSat {
  char name[25];
  char line1[70];
  char line2[70];
  bool valid;
};
QueuedSat satQueue[MAX_QUEUED_SATS];
int  queueHead    = 0;   // index of currently tracked sat
int  queueCount   = 0;

// â”€â”€ Pass schedule (feature 6) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct ScheduledPass {
  char    satName[25];
  time_t  aos;
  time_t  los;
  float   maxEl;
  float   aosAz;
  float   probScore; // Heuristic viability score (0-100)
};
ScheduledPass schedule[MAX_SCHEDULED_PASSES];
int  scheduleCount   = 0;
bool scheduleReady   = false;
bool scheduleBusy    = false;
volatile bool scheduleDirty = false;   // (A3) recompute when queue changes / web refresh

// â”€â”€ Auto-park (feature 1) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool     parked         = false;
unsigned long lastActiveMs = 0;

// â”€â”€ Weather (feature 14) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Requires free API key from openweathermap.org
// Set OWM_API_KEY in secrets.h as:  #define OWM_API_KEY "your_key"
struct WeatherData {
  float   tempC      = 0;
  float   feelsC     = 0;     // (D) apparent/feels-like temp
  float   windMps    = 0;
  float   windDeg    = 0;
  int     humidity   = 0;
  int     pressure   = 0;     // (D) hPa
  int     visibility = 0;     // (D) metres
  char    desc[32]   = "---";
  char    city[32]   = "---";
  time_t  fetchedAt  = 0;
  bool    valid      = false;
};
WeatherData weather;
unsigned long lastWeatherFetch = 0;
char weatherCityName[48] = "";   // if non-empty, overrides lat/lon for OWM query

#include "Web_Pages.h"

// ================= OBJECTS =================
Adafruit_ST7789 tft      = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
AccelStepper    azStepper(AccelStepper::DRIVER, AZ_STEP, AZ_DIR);
AccelStepper    elStepper(AccelStepper::DRIVER, EL_STEP, EL_DIR);
AsyncWebServer  webServer(80);
AsyncWebSocket  ws("/ws");
WiFiServer      tcpServer(4533);
WiFiClient      tcpClient;
WiFiUDP         ntpUDP;
NTPClient       timeClient(ntpUDP,"pool.ntp.org",0,60000);
Preferences     prefs;
Sgp4            sat;
static Sgp4     predSat;
static Sgp4     schedSat;  // dedicated instance for schedule computation

#if GPS_ENABLED
HardwareSerial gpsSerial(2);
#endif

// ================= STATE =================
char  tleLine1[70]="";
char  tleLine2[70]="";
char  satName[25] ="NO SAT";
volatile bool   tleLoaded    =false;
volatile bool   isGeoSat     =false;
volatile bool   isSun        =false;
volatile bool   isMoon       =false;
volatile bool   spiLock      =false;
volatile float  currentAz    =0,currentEl=0;
volatile float  targetAz     =0,targetEl =0;
volatile float  polarizationSquint = 0;
volatile bool   isMoving     =false;
const float     STEPS_PER_DEG=8.88f;
volatile bool   onboardMode  =false;
volatile bool   isAPMode     =false;
volatile bool   triggerReboot=false;
double obsLat=31.52,obsLon=74.35,obsAlt=0.21;
String maidenhead="MM71dl";
volatile bool   ntpSynced    =false;
volatile double lastSatDist  =0;
volatile float  dopplerFreq  =0,satDistance=0;
volatile float  doppler435   =0,doppler1268=0;  // multi-band Doppler (Hz)
volatile float  satFootprintKm=0;
volatile float  satAltitude  =0;
volatile float  satLat       =0,satLon=0;        // sub-satellite point
float           accumAzTarget=0;                 // cable-wrap-safe accumulated Az
bool            accumAzInit  =false;
float           prevRawAz    =0;
unsigned long   lastDopplerTime=0,lastSGP4Update=0;
volatile time_t nextAosTime  =0,nextLosTime=0;
volatile float  nextMaxEl    =0;
volatile bool   predBusy     =false;

struct PassLogEntry{char sat[25];char time[20];float maxEl;};
PassLogEntry passLog[10];
int   passLogCount  =0;
bool  passInProgress=false;
float passMaxEl     =0;
char  passStartBuf[20]="";

struct PathPoint{float az,el;};
PathPoint     globalPath[45];
volatile int  globalPathLen=0;
unsigned long lastPathCalc =0;
volatile bool newPathReady =false;

// ---- TFT cache ----
float prev_cAz,prev_cEl,prev_tAz,prev_tEl,prev_dist,prev_dop;
int   prev_rssi,prev_heap;
int   prev_moving,prev_ntp,prev_l4s,prev_mode,prev_geo;
char  prev_sat[25],prev_clock[10];
long  prev_countdown;
float prev_maxElShown;
int   prev_antX,prev_antY,prev_tgtX,prev_tgtY,prev_satX,prev_satY;
int   prev_alX[2],prev_alY[2];
int   terminalY=30;
TaskHandle_t Core0Task;
const int RCX=120,RCY=228,RR=55;

// ---- Space weather ----
struct SpaceWeather{ float kp=0; bool valid=false; time_t fetchedAt=0; };
SpaceWeather spaceWx;
unsigned long lastSpaceWxFetch=0;

// ---- TLE orbital elements ----
struct TLEElements{
  int   catalogNum=0;
  float inclination=0,raan=0,eccentricity=0;
  float argPerigee=0,meanMotion=0,period=0;
  float semiMajor=0,perigeeAlt=0,apogeeAlt=0;
  float ageDays=0;
  bool  valid=false;
};
TLEElements tleElem;

inline int clampX(int x){return x<RADAR_SAFE_LEFT?RADAR_SAFE_LEFT:x>RADAR_SAFE_RIGHT?RADAR_SAFE_RIGHT:x;}
inline int clampY(int y){return y<RADAR_SAFE_TOP?RADAR_SAFE_TOP:y>RADAR_SAFE_BOT?RADAR_SAFE_BOT:y;}

// ================= IMU =================
bool mpuBegin(){
  Wire.begin(MPU_SDA,MPU_SCL);Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x6B);Wire.write(0x00);
  if(Wire.endTransmission(true)!=0)return false;
  delay(200);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1A);Wire.write(MPU_DLPF_CFG&0x07);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1B);Wire.write(0x00);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1C);Wire.write(0x00);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x75);Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)1,(uint8_t)true);
  if(!Wire.available())return false;
  uint8_t who=Wire.read();
  if(who!=0x68&&who!=0x70&&who!=0x71&&who!=0x72)return false;

  Serial.println("[IMU] Calibrating...");delay(500);
  long sGx=0,sGy=0,sAx=0,sAy=0,sAz=0;int n=0;
  for(int i=0;i<200;i++){
    Wire.beginTransmission(MPU_ADDR);Wire.write(0x3B);Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)14,(uint8_t)true);
    if(Wire.available()<14){delay(5);continue;}
    int16_t ax=(Wire.read()<<8)|Wire.read();
    int16_t ay=(Wire.read()<<8)|Wire.read();
    int16_t az=(Wire.read()<<8)|Wire.read();
    Wire.read();Wire.read();
    int16_t gx=(Wire.read()<<8)|Wire.read();
    int16_t gy=(Wire.read()<<8)|Wire.read();
    Wire.read();Wire.read();
    sAx+=ax;sAy+=ay;sAz+=az;sGx+=gx;sGy+=gy;n++;delay(5);
  }
  if(n<20)return false;
  _gBiasX=(sGx/(float)n)/131.0f;_gBiasY=(sGy/(float)n)/131.0f;
  float mAx=sAx/(float)n,mAy=sAy/(float)n,mAz=sAz/(float)n;
  _aBiasX=mAx;_aBiasY=mAy;_aBiasZ=mAz-16384.0f;
  _lpfAx=mAx-_aBiasX;_lpfAy=mAy-_aBiasY;_lpfAz=mAz-_aBiasZ;
  _seeded=true;
  float bootPitch=atan2f(_lpfAx,sqrtf(_lpfAy*_lpfAy+_lpfAz*_lpfAz))*180.0f/PI;
  float bootRoll =atan2f(_lpfAy,_lpfAz)*180.0f/PI;
  kalPitch.seed(bootPitch,_gBiasX);
  kalRoll.seed(bootRoll,_gBiasY);
  imuPitch=bootPitch;imuRoll=bootRoll;imuOK=true;
  _lastUs=micros();
  Serial.printf("[IMU] Boot pitch=%.2f roll=%.2f\n",imuPitch,imuRoll);
  return true;
}

void mpuRead(){
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x3B);
  if(Wire.endTransmission(false)!=0){imuOK=false;return;}
  Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)14,(uint8_t)true);
  if(Wire.available()<14){imuOK=false;return;}
  int16_t raw_ax=(Wire.read()<<8)|Wire.read();
  int16_t raw_ay=(Wire.read()<<8)|Wire.read();
  int16_t raw_az=(Wire.read()<<8)|Wire.read();
  Wire.read();Wire.read();
  int16_t raw_gx=(Wire.read()<<8)|Wire.read();
  int16_t raw_gy=(Wire.read()<<8)|Wire.read();
  Wire.read();Wire.read();

  float ax=(float)raw_ax-_aBiasX,ay=(float)raw_ay-_aBiasY,az=(float)raw_az-_aBiasZ;
  float gx=((float)raw_gx/131.0f)-_gBiasX,gy=((float)raw_gy/131.0f)-_gBiasY;
  if(fabsf(gx)<GYRO_DEAD)gx=0;if(fabsf(gy)<GYRO_DEAD)gy=0;
  if(_seeded){
    if(fabsf(ax-_lpfAx)>SPIKE_LSB)ax=_lpfAx;
    if(fabsf(ay-_lpfAy)>SPIKE_LSB)ay=_lpfAy;
    if(fabsf(az-_lpfAz)>SPIKE_LSB)az=_lpfAz;
  }
  _lpfAx=ACCEL_LPF*ax+(1-ACCEL_LPF)*_lpfAx;
  _lpfAy=ACCEL_LPF*ay+(1-ACCEL_LPF)*_lpfAy;
  _lpfAz=ACCEL_LPF*az+(1-ACCEL_LPF)*_lpfAz;

  float accPitch=atan2f(_lpfAx,sqrtf(_lpfAy*_lpfAy+_lpfAz*_lpfAz))*180.0f/PI;
  float accRoll =atan2f(_lpfAy,_lpfAz)*180.0f/PI;

  unsigned long nowUs=micros();
  float dt=(nowUs-_lastUs)*1e-6f;
  _lastUs=nowUs;
  if(dt<0.0005f||dt>0.5f)dt=0.02f;

  // Kalman filter update (replaces complementary filter)
  imuPitch=kalPitch.update(accPitch,gx,dt);
  imuRoll =kalRoll.update(accRoll, gy,dt);

  // Record error history for drift graph
  float err=imuPitch-(float)currentEl;
  errHistory[errHistIdx]=err;
  errHistIdx=(errHistIdx+1)%ERR_HIST;
  if(errHistIdx==0)errHistFull=true;

  imuOK=true;
}

void mpuCorrect(volatile float &cel,AccelStepper &els,float spd,bool mov){
#if IMU_CORRECT_ENABLED
  if(!imuOK||mov)return;
  float ph=(imuPitch<0)?0:imuPitch;
  float er=ph-(float)cel;
  if(fabsf(er)>IMU_CORRECT_THRESH){
    Serial.printf("[IMU] Correct err=%.2f\n",er);
    els.setCurrentPosition((long)(ph*spd));cel=ph;
  }
#else
  (void)cel;(void)els;(void)spd;(void)mov;
#endif
}

// ================= SATELLITE FOOTPRINT (feature 13) =================
// Returns the ground radius (km) of the satellite's visibility circle:
// r = R_earth * acos(R/(R+h)), with h = TRUE altitude in km.
// BUG FIX (A4): the old version passed slant range (satDist) and did
// (satDist-6371) to fake an altitude â€” that is geometrically wrong and
// made the footprint ring shrink/grow as the sat moved across the sky.
// The Sgp4 lib exposes the true altitude as sat.satAlt (sgp4pred.h:81),
// so callers now pass that directly.
float computeFootprintKm(double altKm){
  float alt=(float)altKm;
  if(alt<100.0f)alt=100.0f;     // clamp to a sane LEO floor
  float R=6371.0f;
  float footKm=R*acosf(R/(R+alt));
  return footKm;
}

// ================= WEATHER (feature 14) =================
// Copy up to n-1 chars from src into dst, then ALWAYS null-terminate,
// and strip non-ASCII bytes (the Adafruit_GFX font has no UTF-8 glyphs,
// so accented city names like "SÃ£o Paulo" / "MÃ¼nchen" would render as
// garbage). BUG FIX (A5): the old strncpy() left the buffer
// un-terminated when the source was >=31 chars, so snprintf("%s",city)
// read past the buffer â†’ the glitched/flickering city name on screen.
static void copyAscii(char *dst,const char *src,size_t n){
  if(n==0)return;
  size_t i=0;
  if(src){
    for(;i<n-1 && src[i];i++){
      char c=src[i];
      // keep printable ASCII only; replace everything else with a space
      dst[i]=(c>=0x20 && c<0x7f)?c:' ';
    }
  }
  dst[i]='\0';   // always terminate
}

// Parse orbital elements from the globally loaded TLE lines.
// Called whenever a new TLE is loaded. Re-compute on demand.
void parseTLEElements(){
  if(strlen(tleLine1)<68||strlen(tleLine2)<68){tleElem.valid=false;return;}
  char buf[14];
  // Catalog number (L1 chars 2-6)
  strncpy(buf,tleLine1+2,5);buf[5]='\0';tleElem.catalogNum=atoi(buf);
  // Epoch: YYDDD.DDDDDDDD (L1 chars 18-31)
  int yy=(tleLine1[18]-'0')*10+(tleLine1[19]-'0');
  float epochYear=(yy>=57)?1900.0f+yy:2000.0f+yy;
  strncpy(buf,tleLine1+20,12);buf[12]='\0';
  float epochDay=atof(buf);
  float epochUnix=(epochYear-1970.0f)*365.25f*86400.0f+(epochDay-1.0f)*86400.0f;
  float nowUnix=(float)timeClient.getEpochTime();
  tleElem.ageDays=(nowUnix-epochUnix)/86400.0f;
  // Inclination (L2 chars 8-15)
  strncpy(buf,tleLine2+8,8);buf[8]='\0';tleElem.inclination=atof(buf);
  // RAAN (L2 chars 17-24)
  strncpy(buf,tleLine2+17,8);buf[8]='\0';tleElem.raan=atof(buf);
  // Eccentricity (L2 chars 26-32, implied decimal point)
  strncpy(buf,tleLine2+26,7);buf[7]='\0';tleElem.eccentricity=atof(buf)*1e-7f;
  // Arg of perigee (L2 chars 34-41)
  strncpy(buf,tleLine2+34,8);buf[8]='\0';tleElem.argPerigee=atof(buf);
  // Mean motion rev/day (L2 chars 52-62)
  strncpy(buf,tleLine2+52,11);buf[11]='\0';tleElem.meanMotion=atof(buf);
  // Derived: period, semi-major axis, perigee/apogee altitudes
  if(tleElem.meanMotion>0){
    tleElem.period=1440.0f/tleElem.meanMotion;           // minutes
    float nRad=tleElem.meanMotion*2.0f*PI/86400.0f;      // rad/s
    float mu=398600.4418f;                               // km^3/s^2
    float a=powf(mu/(nRad*nRad),1.0f/3.0f);             // km
    tleElem.semiMajor=a;
    float e=tleElem.eccentricity;
    const float Re=6371.0f;
    tleElem.perigeeAlt=a*(1.0f-e)-Re;
    tleElem.apogeeAlt =a*(1.0f+e)-Re;
  }
  tleElem.valid=true;
  Serial.printf("[TLE] Parsed: NORAD %d  incl %.2f  per %.1fmin  age %.1fd\n",
    tleElem.catalogNum,tleElem.inclination,tleElem.period,tleElem.ageDays);
}

// Fetch current Kp geomagnetic index from NOAA Space Weather.
// Response is a JSON array-of-arrays; we just want the last row's second element.
void fetchSpaceWeather(){
  if(isAPMode||!ntpSynced)return;
  HTTPClient http;
  http.begin("https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json");
  http.setTimeout(8000);
  int code=http.GET();
  if(code==200){
    String body=http.getString();
    // Find last occurrence of '"Kp":'
    int last=body.lastIndexOf("\"Kp\":");
    if(last>0){
      int end=body.indexOf(',',last);
      if(end<0) end=body.indexOf('}',last);
      if(end>0){
        String kpStr=body.substring(last+5,end);
        float kp=kpStr.toFloat();
        if(kp>=0&&kp<=9){
          spaceWx.kp=kp;
          spaceWx.valid=true;
          spaceWx.fetchedAt=timeClient.getEpochTime();
          Serial.printf("[SPACE-WX] Kp=%.2f\n",kp);
        }
      }
    }
  } else { Serial.printf("[SPACE-WX] HTTP %d\n",code); }
  http.end();
}

void fetchWeather(){
  if(isAPMode||!ntpSynced)return;
  #ifndef OWM_API_KEY
  return;
  #endif
  String url;
  if(strlen(weatherCityName)>0){
    url="http://api.openweathermap.org/data/2.5/weather?q="+
        String(weatherCityName)+"&units=metric&appid=" OWM_API_KEY;
  } else {
    url="http://api.openweathermap.org/data/2.5/weather?lat="+
        String(obsLat,4)+"&lon="+String(obsLon,4)+
        "&units=metric&appid=" OWM_API_KEY;
  }
  HTTPClient http;
  http.begin(url);
  http.setTimeout(8000);
  int code=http.GET();
  if(code==200){
    String body=http.getString();
    StaticJsonDocument<1024> doc;
    if(!deserializeJson(doc,body)){
      weather.tempC   =doc["main"]["temp"]|0.0f;
      weather.feelsC  =doc["main"]["feels_like"]|weather.tempC;   // (D)
      weather.windMps =doc["wind"]["speed"]|0.0f;
      weather.windDeg =doc["wind"]["deg"]|0.0f;
      weather.humidity=doc["main"]["humidity"]|0;
      weather.pressure=doc["main"]["pressure"]|0;                 // (D)
      weather.visibility=doc["visibility"]|0;                     // (D)
      const char* d=doc["weather"][0]["description"]|"---";
      const char* c=doc["name"]|"---";
      copyAscii(weather.desc,d,sizeof(weather.desc));   // (A5) terminate + ASCII
      copyAscii(weather.city,c,sizeof(weather.city));
      weather.fetchedAt=timeClient.getEpochTime();
      weather.valid=true;
      Serial.printf("[WX] %s %.1fÂ°C feels %.1f wind %.1fm/s\n",
                    weather.city,weather.tempC,weather.feelsC,weather.windMps);
    }
  } else if(code==404&&strlen(weatherCityName)>0){
    Serial.printf("[WX] City not found (404), reverting to lat/lon\n");
    weatherCityName[0]='\0';
  } else {
    Serial.printf("[WX] HTTP %d\n",code);
  }
  http.end();
}

// (D) 16-point compass label from a wind bearing in degrees.
static const char* windCompass(float deg){
  static const char* lbl[16]={"N","NNE","NE","ENE","E","ESE","SE","SSE",
                              "S","SSW","SW","WSW","W","WNW","NW","NNW"};
  int i=((int)((deg+11.25f)/22.5f))%16;
  return lbl[i];
}

// ================= AUTO-PARK (feature 1) & PRE-POINT =================
void checkAutopark(){
  if(isAPMode||!onboardMode)return;
  unsigned long nowEpoch = timeClient.getEpochTime();
  bool passActive=(nextAosTime>0 && (time_t)nowEpoch>=nextAosTime && (time_t)nowEpoch<=nextLosTime);
  bool prePointActive=(nextAosTime>0 && (time_t)nowEpoch>=nextAosTime-PRE_POINT_SEC && (time_t)nowEpoch<nextAosTime);
  
  if(passActive){lastActiveMs=millis();parked=false;return;}
  
  if(prePointActive){
     if(parked) { Serial.println("[PRE-POINT] Slewing to AOS coordinates"); }
     parked=false;
     lastActiveMs=millis();
     if(globalPathLen>0) {
        targetAz = globalPath[0].az;
        targetEl = globalPath[0].el;
     }
     return;
  }
  
  if(isMoving){lastActiveMs=millis();}
  if(!parked&&millis()-lastActiveMs>PARK_IDLE_SEC*1000UL){
    parked=true;
    targetAz=PARK_AZ;targetEl=PARK_EL;
    Serial.println("[PARK] Parking antenna â€” idle timeout");
  }
}

// ================= SATELLITE QUEUE (feature 4) =================
bool tleIsGeo();   // forward decl: defined later, used by advanceQueue()
void advanceQueue(){
  if(queueCount<=1)return;
  // Rotate: remove head, shift down
  for(int i=0;i<queueCount-1;i++) satQueue[i]=satQueue[i+1];
  queueCount--;queueHead=0;
  // Load next satellite
  if(queueCount>0&&satQueue[0].valid){
    strncpy(satName,satQueue[0].name,24);
    isSun=(strcmp(satName,"SUN")==0);
    isMoon=(strcmp(satName,"MOON")==0);
    if(isSun||isMoon){
      tleLoaded=true;isGeoSat=false;
      nextAosTime=nextLosTime=0;lastPathCalc=0;newPathReady=true;
      Serial.printf("[QUEUE] Advanced to celestial: %s\n",satName);
    }else{
      strncpy(tleLine1,satQueue[0].line1,69);
      strncpy(tleLine2,satQueue[0].line2,69);
      sat.site(obsLat,obsLon,obsAlt);
      sat.init(satName,tleLine1,tleLine2);
      isGeoSat=tleIsGeo();tleLoaded=true;
      nextAosTime=nextLosTime=0;lastPathCalc=0;newPathReady=true;
      Serial.printf("[QUEUE] Advanced to: %s\n",satName);
    }
  }
}

// ================= PASS SCHEDULE (feature 6) =================
// Runs on Core0 task. Scans next 24h for all queued satellites.
void computeSchedule(){
  if(scheduleBusy||!ntpSynced||isAPMode)return;
  scheduleBusy=true;scheduleCount=0;scheduleReady=false;
  unsigned long nowT=timeClient.getEpochTime();
  if(nowT<1000000000UL){scheduleBusy=false;return;}

  for(int q=0;q<queueCount&&scheduleCount<MAX_SCHEDULED_PASSES;q++){
    if(!satQueue[q].valid)continue;
    if(strcmp(satQueue[q].name,"SUN")==0 || strcmp(satQueue[q].name,"MOON")==0)continue;
    schedSat.site(obsLat,obsLon,obsAlt);
    schedSat.init(satQueue[q].name,satQueue[q].line1,satQueue[q].line2);

    unsigned long t=nowT,limit=nowT+86400;
    while(t<limit&&scheduleCount<MAX_SCHEDULED_PASSES){
      schedSat.findsat(t);
      if(schedSat.satEl>0){
        // Back up to AOS
        int lim=0;
        while(schedSat.satEl>0&&lim++<120){t-=30;schedSat.findsat(t);}
        t+=30;
        time_t aosT=(time_t)t;
        float maxEl=0;float aosAz=(float)schedSat.satAz;
        // Scan pass
        unsigned long tt=t;
        while(true){
          schedSat.findsat(tt);
          if(schedSat.satEl<0)break;
          if(schedSat.satEl>maxEl)maxEl=schedSat.satEl;
          tt+=30;
          if(tt-t>7200)break; // safety
        }
        // Calculate Viability / Probability Score
        float pScore = 20.0f + (maxEl * 0.8f); // Base score from elevation
        if(spaceWx.valid) {
          if(spaceWx.kp >= 6) pScore -= 40.0f;       // Severe storm kills VHF/UHF
          else if(spaceWx.kp >= 4) pScore -= 15.0f;  // Unsettled
        }
        if(weather.valid) {
          if(weather.windMps > 15) pScore -= 20.0f;  // High wind shakes the antenna
        }
        if(pScore < 0) pScore = 0;
        if(pScore > 99.9f) pScore = 99.9f;

        // Record
        strncpy(schedule[scheduleCount].satName,satQueue[q].name,24);
        schedule[scheduleCount].aos=aosT;
        schedule[scheduleCount].los=(time_t)tt;
        schedule[scheduleCount].maxEl=maxEl;
        schedule[scheduleCount].aosAz=aosAz;
        schedule[scheduleCount].probScore=pScore;
        scheduleCount++;
        t=tt+60; // skip past this pass
      } else {
        t+=60;
      }
      vTaskDelay(pdMS_TO_TICKS(1)); // yield
    }
  }
  // Sort by AOS time
  for(int i=0;i<scheduleCount-1;i++){
    for(int j=i+1;j<scheduleCount;j++){
      if(schedule[j].aos<schedule[i].aos){
        ScheduledPass tmp=schedule[i];schedule[i]=schedule[j];schedule[j]=tmp;
      }
    }
  }
  scheduleReady=true;scheduleBusy=false;
  Serial.printf("[SCHED] %d passes computed\n",scheduleCount);
}

// ================= HELPERS =================
bool tleIsGeo(){
  if(strlen(tleLine2)<63)return false;
  double mm=atof(tleLine2+52);return(mm>0.1&&mm<2.0);
}

void maidenheadToLatLon(String grid,double &lat,double &lon){
  grid.toUpperCase();if(grid.length()<4)return;
  lon=(grid[0]-'A')*20.0-180.0;lat=(grid[1]-'A')*10.0-90.0;
  lon+=(grid[2]-'0')*2.0;lat+=(grid[3]-'0')*1.0;
  if(grid.length()>=6){lon+=((tolower(grid[4])-'a')*5.0)/60.0;lat+=((tolower(grid[5])-'a')*2.5)/60.0;}
  lon+=1.0;lat+=0.5;
}

void resetTftCache(){
  prev_cAz=prev_cEl=prev_tAz=prev_tEl=-999;prev_dist=prev_dop=-1e9;
  prev_rssi=999;prev_heap=-1;prev_moving=prev_ntp=prev_l4s=prev_mode=prev_geo=-1;
  prev_sat[0]='\0';prev_clock[0]='\0';prev_countdown=-999999;prev_maxElShown=-999;
  prev_antX=prev_antY=prev_tgtX=prev_tgtY=prev_satX=prev_satY=-1;
  prev_alX[0]=prev_alX[1]=-1;
}

String buildTelemetryJson(){
  StaticJsonDocument<2560> doc;
  doc["tAz"]=(float)targetAz;doc["tEl"]=(float)targetEl;
  doc["cAz"]=(float)currentAz;doc["cEl"]=(float)currentEl;
  doc["isMoving"]=isMoving;doc["rssi"]=isAPMode?0:WiFi.RSSI();
  doc["freeHeap"]=ESP.getFreeHeap()/1024;doc["uptime"]=millis()/1000;
  doc["mode"]=onboardMode?1:0;doc["sat"]=satName;doc["grid"]=maidenhead;
  doc["doppler"]=(float)dopplerFreq;
  doc["doppler435"]=(float)doppler435;
  doc["doppler1268"]=(float)doppler1268;
  doc["dist"]=(float)satDistance;
  doc["satLat"]=(float)satLat;doc["satLon"]=(float)satLon;
  doc["geo"]=isGeoSat;doc["parked"]=parked;
  doc["imuPitch"]=(float)imuPitch;doc["imuRoll"]=(float)imuRoll;doc["imuOK"]=imuOK;
  doc["footprintR"]=(float)satFootprintKm;
  doc["squint"]=(float)polarizationSquint;
  doc["wxCityOverride"]=weatherCityName;
  // Space weather
  doc["kpValid"]=spaceWx.valid;
  if(spaceWx.valid) doc["kp"]=spaceWx.kp;
  // TLE age
  doc["tleAge"]=tleElem.valid?tleElem.ageDays:0.0f;
  // Orbital elements sub-object
  if(tleElem.valid){
    JsonObject el=doc.createNestedObject("tleElem");
    el["catalogNum"]=tleElem.catalogNum;
    el["inclination"]=tleElem.inclination;
    el["raan"]=tleElem.raan;
    el["eccentricity"]=tleElem.eccentricity;
    el["perigeeAlt"]=tleElem.perigeeAlt;
    el["apogeeAlt"]=tleElem.apogeeAlt;
    el["period"]=tleElem.period;
  }
  // Weather
  doc["wxValid"]=weather.valid;
  if(weather.valid){
    doc["wxTemp"]=weather.tempC;doc["wind"]=weather.windMps;
    doc["wxWindDeg"]=weather.windDeg;doc["wxHum"]=weather.humidity;
    doc["wxDesc"]=weather.desc;doc["wxCity"]=weather.city;
    doc["wxFeels"]=weather.feelsC;doc["wxPressure"]=weather.pressure;   // (D)
    doc["wxVis"]=weather.visibility;                                     // (D)
  }
  // Queue summary
  JsonArray qa=doc.createNestedArray("queue");
  for(int i=0;i<queueCount;i++){
    JsonObject o=qa.createNestedObject();
    o["name"]=satQueue[i].name;o["active"]=(i==queueHead);
  }
  String out;serializeJson(doc,out);return out;
}

// ================= FORWARD DECLARATIONS =================
bool parseEasyComm(String cmd);
void runSGP4();
void calculatePathPrediction();
void tftDrawStaticFrame();
void tftDrawAPMode();
void tftUpdateDynamic();
void tftRadarUpdate();
void tftDrawScheduleScreen();
void tftDrawDriftGraph();
void tftDrawFootprintScreen();
void tftDrawWeatherScreen();
void setupWebServer();

#include "Display_Module.h"
#include "Runtime_Module.h"

// ================= PASS LOG =================
void logPass(){
  if(passLogCount>=10){for(int i=0;i<9;i++)passLog[i]=passLog[i+1];passLogCount=9;}
  strncpy(passLog[passLogCount].sat,satName,24);
  strncpy(passLog[passLogCount].time,passStartBuf,19);
  passLog[passLogCount].maxEl=passMaxEl;passLogCount++;
  Serial.printf("[SGP4] Pass: %s MaxEl %.1f\n",satName,passMaxEl);
}

// ================= WEB SERVER =================
void setupWebServer(){
  webServer.on("/",HTTP_GET,[](AsyncWebServerRequest *r){r->send_P(200,"text/html",isAPMode?wifi_html:index_html);});

  webServer.on("/api/wifi",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<256> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String ns=doc["ssid"]|"",np=doc["pass"]|"";
        if(ns.length()>0){prefs.begin("sattracker",false);prefs.putString("wifi_ssid",ns);prefs.putString("wifi_pass",np);prefs.end();triggerReboot=true;}
      }
    });

  webServer.on("/api/status",HTTP_GET,[](AsyncWebServerRequest *r){r->send(200,"application/json",buildTelemetryJson());});

  webServer.on("/api/path",HTTP_GET,[](AsyncWebServerRequest *r){
    if(!tleLoaded||!ntpSynced||!onboardMode||predBusy){r->send(200,"application/json","[]");return;}
    String j="[";for(int i=0;i<globalPathLen;i++){if(i)j+=",";j+="{\"az\":"+String(globalPath[i].az,1)+",\"el\":"+String(globalPath[i].el,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/passlog",HTTP_GET,[](AsyncWebServerRequest *r){
    String j="[";for(int i=0;i<passLogCount;i++){if(i)j+=",";j+="{\"sat\":\""+String(passLog[i].sat)+"\",\"time\":\""+String(passLog[i].time)+"\",\"maxEl\":"+String(passLog[i].maxEl,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/schedule",HTTP_GET,[](AsyncWebServerRequest *r){
    if(!scheduleReady){r->send(200,"application/json","[]");return;}
    String j="[";for(int i=0;i<scheduleCount;i++){if(i)j+=",";j+="{\"sat\":\""+String(schedule[i].satName)+"\",\"aos\":"+String((long)schedule[i].aos)+",\"los\":"+String((long)schedule[i].los)+",\"maxEl\":"+String(schedule[i].maxEl,1)+",\"aosAz\":"+String(schedule[i].aosAz,1)+",\"score\":"+String(schedule[i].probScore,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/config",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<256> doc;if(!deserializeJson(doc,String((char*)data,len))){
        if(doc.containsKey("grid")){maidenhead=doc["grid"].as<String>();maidenheadToLatLon(maidenhead,obsLat,obsLon);prefs.begin("sattracker",false);prefs.putString("grid",maidenhead);prefs.end();lastPathCalc=0;}
        if(doc.containsKey("obMode")){onboardMode=doc["obMode"].as<bool>();prefs.begin("sattracker",false);prefs.putBool("obMode",onboardMode);prefs.end();lastPathCalc=0;}
      }
    });

  // Weather city override
  webServer.on("/api/weather/city",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<128> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String city=doc["city"]|"";
        city.trim();
        strncpy(weatherCityName,city.c_str(),sizeof(weatherCityName)-1);
        weatherCityName[sizeof(weatherCityName)-1]='\0';
        prefs.begin("sattracker",false);prefs.putString("wxCity",city);prefs.end();
        lastWeatherFetch=0; // force immediate refetch on next cycle
        if(city.length()>0) Serial.printf("[WX] City set: %s\n",weatherCityName);
        else Serial.println("[WX] City cleared, using lat/lon");
      }
    });

  webServer.on("/api/tle",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<512> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String name=doc["name"]|"UNKNOWN",l1=doc["line1"]|"",l2=doc["line2"]|"";
        bool isCelestial = (name=="SUN"||name=="MOON");
        if(isCelestial || (l1.length()>0&&l2.length()>0)){
          if(!isCelestial) {
            File f=LittleFS.open("/tle.txt","w");if(f){f.println(name);f.println(l1);f.println(l2);f.close();}
            name.toCharArray(satName,25);l1.toCharArray(tleLine1,70);l2.toCharArray(tleLine2,70);
            sat.site(obsLat,obsLon,obsAlt);sat.init(satName,tleLine1,tleLine2);
            isGeoSat=tleIsGeo();
          } else {
            name.toCharArray(satName,25);tleLine1[0]='\0';tleLine2[0]='\0';
            isGeoSat=false;
          }
          isSun=(strcmp(satName,"SUN")==0);isMoon=(strcmp(satName,"MOON")==0);
          tleLoaded=true;nextAosTime=nextLosTime=0;lastPathCalc=0;newPathReady=true;
          if(queueCount>=MAX_QUEUED_SATS) queueCount=MAX_QUEUED_SATS-1;
          memmove(&satQueue[1],&satQueue[0],sizeof(QueuedSat)*queueCount);
          strncpy(satQueue[0].name,satName,24);
          strncpy(satQueue[0].line1,tleLine1,69);
          strncpy(satQueue[0].line2,tleLine2,69);
          satQueue[0].valid=true;queueCount++;queueHead=0;
          scheduleDirty=true;
          Serial.printf("[TRACK] Switched to: %s\n",satName);
        }
      }
    });

  webServer.on("/api/queue",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      if(queueCount>=MAX_QUEUED_SATS){
        AsyncWebServerResponse *res=r->beginResponse(200,"application/json","{\"status\":\"full\"}");r->send(res);return;
      }
      StaticJsonDocument<512> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String name=doc["name"]|"",l1=doc["line1"]|"",l2=doc["line2"]|"";
        bool isCel = (name=="SUN"||name=="MOON");
        if(name.length()>0 && (isCel || l1.length()>0)){
          strncpy(satQueue[queueCount].name,name.c_str(),24);
          strncpy(satQueue[queueCount].line1,l1.c_str(),69);
          strncpy(satQueue[queueCount].line2,l2.c_str(),69);
          satQueue[queueCount].valid=true;queueCount++;
          scheduleDirty=true;
          Serial.printf("[QUEUE] Added: %s (%d total)\n",satQueue[queueCount-1].name,queueCount);
        }
      }
      r->send(200,"application/json","{\"status\":\"ok\"}");
    });

  webServer.on("/api/queue/remove",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<64> doc;if(!deserializeJson(doc,String((char*)data,len))){
        int idx=doc["idx"]|0;
        if(idx>=0&&idx<queueCount){for(int i=idx;i<queueCount-1;i++)satQueue[i]=satQueue[i+1];queueCount--;if(queueHead>=queueCount)queueHead=0;scheduleDirty=true;}
      }
    });

  webServer.on("/api/queue/clear",HTTP_POST,[](AsyncWebServerRequest *r){queueCount=0;queueHead=0;scheduleDirty=true;r->send(200,"application/json","{\"status\":\"ok\"}");});

  // (A3) web "REFRESH SCHEDULE" kicks a recompute; client then GETs /api/schedule.
  webServer.on("/api/schedule/refresh",HTTP_POST,[](AsyncWebServerRequest *r){scheduleDirty=true;r->send(200,"application/json","{\"status\":\"ok\"}");});

  webServer.on("/api/manual",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<128> doc;if(!deserializeJson(doc,String((char*)data,len))&&!onboardMode&&!isAPMode){
        if(doc.containsKey("az"))targetAz=doc["az"].as<float>();if(doc.containsKey("el"))targetEl=doc["el"].as<float>();
      }
    });

  ws.onEvent([](AsyncWebSocket*,AsyncWebSocketClient *c,AwsEventType t,void*,uint8_t*,size_t){if(t==WS_EVT_CONNECT)c->text(buildTelemetryJson());});
  webServer.addHandler(&ws);webServer.begin();
}

// ================= PASS PREDICTION =================
void calculatePathPrediction(){
  if(!tleLoaded||!ntpSynced||!onboardMode||isAPMode){globalPathLen=0;newPathReady=true;return;}
  if(isSun||isMoon){globalPathLen=0;predBusy=false;newPathReady=true;return;}
  unsigned long nowT=timeClient.getEpochTime();
  if(nowT<1000000000UL)return;
  predBusy=true;predSat.site(obsLat,obsLon,obsAlt);predSat.init(satName,tleLine1,tleLine2);isGeoSat=tleIsGeo();
  if(isGeoSat){predSat.findsat(nowT);nextMaxEl=predSat.satEl;nextAosTime=(predSat.satEl>0)?(time_t)nowT:0;nextLosTime=0;globalPathLen=0;predBusy=false;newPathReady=true;return;}
  unsigned long t=nowT;predSat.findsat(t);bool found=false;
  if(predSat.satEl>0){int lim=0;while(predSat.satEl>0&&lim++<80){t-=30;predSat.findsat(t);if(lim%10==0)vTaskDelay(pdMS_TO_TICKS(2));}t+=30;found=true;}
  else{for(int i=0;i<1440&&!found;i++){t+=60;predSat.findsat(t);if(predSat.satEl>0){int lim=0;while(predSat.satEl>0&&lim++<12){t-=10;predSat.findsat(t);}t+=10;found=true;}if(i%30==0)vTaskDelay(pdMS_TO_TICKS(2));}}
  if(!found){if(nextLosTime>(time_t)nowT||nextAosTime>(time_t)nowT){predBusy=false;return;}nextAosTime=nextLosTime=0;nextMaxEl=0;globalPathLen=0;predBusy=false;newPathReady=true;return;}
  static PathPoint tmp[45];float mEl=0;int pl=0;unsigned long tt=t;
  for(int i=0;i<80;i++){predSat.findsat(tt);if(predSat.satEl<0&&i>0)break;if(predSat.satEl>=0){if(predSat.satEl>mEl)mEl=predSat.satEl;if(pl<45){tmp[pl].az=predSat.satAz;tmp[pl].el=predSat.satEl;pl++;}}tt+=30;if(i%10==0)vTaskDelay(pdMS_TO_TICKS(2));}
  memcpy((void*)globalPath,tmp,sizeof(PathPoint)*pl);nextAosTime=(time_t)t;nextLosTime=(time_t)tt;nextMaxEl=mEl;globalPathLen=pl;predBusy=false;newPathReady=true;
  Serial.printf("[SGP4] Pass AOS+%lds MaxEl %.1f (%d pts)\n",(long)(nextAosTime-nowT),mEl,pl);
}

// ================= CORE 0 TASK =================
void Core0TaskCode(void *pvParameters){
  static unsigned long lastWeather=0,lastSchedule=0,lastWifiCheck=0,lastWsPush=0,lastTft=0;
  for(;;){
    esp_task_wdt_reset();
    if(WiFi.status()==WL_CONNECTED&&!isAPMode)if(timeClient.update())ntpSynced=true;
    if(millis()-lastWifiCheck>10000&&!isAPMode){lastWifiCheck=millis();if(WiFi.status()!=WL_CONNECTED)WiFi.reconnect();}
    if(!tcpClient||!tcpClient.connected()){tcpClient=tcpServer.available();if(tcpClient)tcpClient.setNoDelay(true);}
    if(tcpClient&&tcpClient.available()){tcpClient.setTimeout(5);String cmd=tcpClient.readStringUntil('\n');if(cmd.length()>0){bool ack=false;if(!onboardMode&&!isAPMode)ack=parseEasyComm(cmd);if(!ack)tcpClient.println("OK");}}

#if GPS_ENABLED
    while(gpsSerial.available()){
      String nmea = gpsSerial.readStringUntil('\n');
      processNMEA(nmea);
    }
#endif

    bool passExpired=(nextLosTime>0&&!isGeoSat&&(time_t)timeClient.getEpochTime()>nextLosTime&&millis()-lastPathCalc>10000);
    if((millis()-lastPathCalc>60000||passExpired)&&!isAPMode){calculatePathPrediction();lastPathCalc=millis();}

    // Auto-advance queue when pass ends
    if(passExpired&&queueCount>1){advanceQueue();}

    // Weather fetch
    if(millis()-lastWeather>WEATHER_INTERVAL_MS&&!isAPMode){fetchWeather();lastWeather=millis();}
    // Space weather (Kp index) - every hour
    if((millis()-lastSpaceWxFetch>SPACE_WX_INTERVAL_MS||lastSpaceWxFetch==0)&&!isAPMode){fetchSpaceWeather();lastSpaceWxFetch=millis();}

    // Schedule compute: run in a dedicated task so Core0 (TFT / WiFi) never freezes.
    // We only spawn the task when not already running (schedTaskRunning guard).
    static volatile bool schedTaskRunning=false;
    if((scheduleDirty||millis()-lastSchedule>1800000)&&!isAPMode&&queueCount>0&&!schedTaskRunning){
      scheduleDirty=false;lastSchedule=millis();schedTaskRunning=true;
      xTaskCreatePinnedToCore([](void* p){computeSchedule();*((volatile bool*)p)=false;vTaskDelete(NULL);},"SchedTask",8192,(void*)&schedTaskRunning,1,NULL,0);
    }

    if(millis()-lastWsPush>1000&&!isAPMode){if(ws.count()>0)ws.textAll(buildTelemetryJson());lastWsPush=millis();}

    // TFT update â€” only screen 0 needs frequent updates; others are static
    if(millis()-lastTft>300&&!isAPMode){
      if(screenChanged){
        screenChanged=false;
        switch(currentScreen){
          case 0:tftDrawStaticFrame();break;
          case 1:tftDrawScheduleScreen();break;
          case 2:tftDrawDriftGraph();break;
          case 3:tftDrawFootprintScreen();break;
          case 4:tftDrawWeatherScreen();break;
          case 5:tftDrawOrbitalScreen();break;
        }
      }
      if(currentScreen==0){tftUpdateDynamic();tftRadarUpdate();}
      else if(currentScreen==2){tftDrawDriftGraph();}  // drift graph updates live
      lastTft=millis();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);delay(500);
  Serial.println("\n====================================");
  Serial.println(" ESP32 Orbital Ops v10.0");
  Serial.println("====================================");
  esp_task_wdt_init(30,true);esp_task_wdt_add(NULL);

  // Button
  pinMode(BTN_PIN,INPUT_PULLUP);

  pinMode(ENABLE_PIN,OUTPUT);digitalWrite(ENABLE_PIN,HIGH);
  azStepper.setMaxSpeed(2000);azStepper.setAcceleration(1000);
  elStepper.setMaxSpeed(2000);elStepper.setAcceleration(1000);

  tft.init(240,320);tft.setRotation(0);tft.invertDisplay(false);
  drawBootScreen();delay(3000);
  // (C) switch to the Linux-tty boot log
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,22,C_HDRBG);tft.fillRect(0,22,240,2,C_ACCENT);
  tft.setTextColor(C_ACCENT);tft.setTextSize(1);tft.setCursor(6,7);tft.print("orbital tty1");
  tft.setTextColor(C_MGRAY);tft.setCursor(170,7);tft.print("boot log");
  tft.drawFastHLine(0,24,240,C_DGRAY);
  terminalY=28;
  tft.setTextColor(C_PRIMARY);tft.setTextSize(1);
  tft.setCursor(4,terminalY);tft.print("orbital login: root");
  terminalY+=11;
  tft.setTextColor(C_MGRAY);tft.setCursor(4,terminalY);tft.print("Welcome to Orbital Ops v10.0");
  terminalY+=11;

#if GPS_ENABLED
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  printBootLine("GPS module init on UART2",2);
#endif

  printBootLine("Initializing IMU...",2);
  if(mpuBegin()){
    for(int i=0;i<50;i++){mpuRead();delay(10);}
    printBootLine("IMU Kalman p="+String(imuPitch,1),0);
#if IMU_CORRECT_ENABLED
    float be=(imuPitch<0)?0:imuPitch;elStepper.setCurrentPosition((long)(be*STEPS_PER_DEG));
#endif
  } else {imuOK=false;printBootLine("IMU NOT FOUND",3);}

  printBootLine("Mounting LittleFS",2);
  if(!LittleFS.begin(false))LittleFS.begin(true);
  printBootLine("LittleFS mounted",0);

  printBootLine("Loading NVRAM prefs",2);
  prefs.begin("sattracker",true);
  maidenhead=prefs.getString("grid",maidenhead);onboardMode=prefs.getBool("obMode",false);
  String savedSSID=prefs.getString("wifi_ssid",ssid),savedPass=prefs.getString("wifi_pass",password);
  { String wc=prefs.getString("wxCity",""); if(wc.length()>0){strncpy(weatherCityName,wc.c_str(),sizeof(weatherCityName)-1);weatherCityName[sizeof(weatherCityName)-1]='\0';} }
  prefs.end();
  maidenheadToLatLon(maidenhead,obsLat,obsLon);sat.site(obsLat,obsLon,obsAlt);
  printBootLine("Prefs: grid "+maidenhead,0);

  if(LittleFS.exists("/tle.txt")){
    File f=LittleFS.open("/tle.txt","r");
    if(f){
      String n=f.readStringUntil('\n');n.trim();n.toCharArray(satName,25);
      String l1=f.readStringUntil('\n');l1.trim();l1.toCharArray(tleLine1,70);
      String l2=f.readStringUntil('\n');l2.trim();l2.toCharArray(tleLine2,70);f.close();
      if(strlen(tleLine1)>0&&strlen(tleLine2)>0){
        sat.init(satName,tleLine1,tleLine2);isGeoSat=tleIsGeo();tleLoaded=true;
        // Pre-populate queue with saved satellite
        strncpy(satQueue[0].name,satName,24);strncpy(satQueue[0].line1,tleLine1,69);strncpy(satQueue[0].line2,tleLine2,69);satQueue[0].valid=true;queueCount=1;
        parseTLEElements(); // Parse orbital elements immediately on boot
        printBootLine("TLE loaded: "+String(satName),0);
      }
    }
  } else {
    printBootLine("No saved TLE",3);
  }

  printBootLine("Coupling WiFi radio",2);
  WiFi.mode(WIFI_STA);WiFi.begin(savedSSID.c_str(),savedPass.c_str());WiFi.setSleep(false);
  unsigned long t0=millis();
  while(WiFi.status()!=WL_CONNECTED){if(millis()-t0>15000){isAPMode=true;break;}delay(500);}

  if(isAPMode){
    printBootLine("Network timeout",1);
    printBootLine("Setup gateway mode",3);
    WiFi.disconnect();WiFi.mode(WIFI_AP);WiFi.softAP("AEROSPACE-TRACKER","groundstation");
    setupWebServer();tcpServer.begin();delay(500);tftDrawAPMode();return;
  }
  printBootLine("WiFi link up",0);
  printBootLine("IP "+WiFi.localIP().toString(),0);
  if(MDNS.begin("sattracker")){MDNS.addService("http","tcp",80);MDNS.addService("easycomm","tcp",4533);printBootLine("mDNS sattracker.local",0);}

  printBootLine("Syncing reference clock",2);
  timeClient.begin();timeClient.update();
  if(timeClient.getEpochTime()>1000000){ntpSynced=true;printBootLine("NTP sync OK",0);}
  else printBootLine("NTP sync pending",3);

  ArduinoOTA.setHostname("sattracker");ArduinoOTA.begin();
  setupWebServer();tcpServer.begin();
  printBootLine("TCP/Web endpoints up",0);

  // Initial weather fetch
  printBootLine("Fetching weather",2);fetchWeather();lastWeatherFetch=millis();
  printBootLine(weather.valid?("WX "+String(weather.city)):String("WX fetch failed"),weather.valid?0:3);

  if(tleLoaded&&ntpSynced&&onboardMode){
    printBootLine("Compiling SGP4 matrix",2);calculatePathPrediction();lastPathCalc=millis();
    printBootLine("SGP4 matrix ready",0);
  }
  if(queueCount>0&&ntpSynced){
    printBootLine("Building pass schedule",2);computeSchedule();
    printBootLine(String(scheduleCount)+" passes queued",0);
  }

  printBootLine("Starting Ops HUD",0);delay(500);
  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN,LOW);
  lastActiveMs=millis();

  xTaskCreatePinnedToCore(Core0TaskCode,"Core0Task",32768,NULL,1,&Core0Task,0);
}

// ================= LOOP (CORE 1) =================
void loop(){
  esp_task_wdt_reset();
  if(triggerReboot){delay(1000);ESP.restart();}
  if(isAPMode)return;

  ArduinoOTA.handle();

  // Button handler â€” IO0 cycles screens
  if(digitalRead(BTN_PIN)==LOW&&millis()-lastBtnMs>300){
    lastBtnMs=millis();
    currentScreen=(currentScreen+1)%SCREEN_COUNT;
    screenChanged=true;
    Serial.printf("[BTN] Screen %d\n",currentScreen);
  }

  float calcAz = (float)targetAz;
  float calcEl = (float)targetEl;
#if ALLOW_FLIP_OVER
  static float baseAz = -1;
  if (passInProgress && passMaxEl > 82.0f && !isSun && !isMoon) {
      if (currentEl < 45.0f && baseAz < 0) baseAz = calcAz;
      if (baseAz >= 0) {
         float azDiff = abs(calcAz - baseAz);
         if (azDiff > 90.0f && azDiff < 270.0f) {
            calcAz = fmodf(calcAz + 180.0f, 360.0f);
            calcEl = 180.0f - calcEl;
         }
      }
  } else {
      baseAz = -1;
  }
  calcEl = constrain(calcEl, 0.0f, 180.0f);
#else
  calcEl = constrain(calcEl, 0.0f, 90.0f);
#endif

  // Azimuth shortest-path with cable wrap protection.
  // Instead of always going to the raw 0-360 position, compute the
  // delta from the last raw target to the new raw target and accumulate.
  // This ensures the motor never takes the long way around (350->10 goes
  // +20, not -340) and caps cumulative rotation at AZ_WRAP_LIMIT degrees.
  {
    float rawAz=fmodf(calcAz+360.0f,360.0f);
    if(!accumAzInit){accumAzTarget=rawAz;prevRawAz=rawAz;accumAzInit=true;}
    float delta=rawAz-prevRawAz;
    if(delta>180.0f)delta-=360.0f;
    if(delta<-180.0f)delta+=360.0f;
    prevRawAz=rawAz;
    accumAzTarget+=delta;
    // Cable wrap: if accumulated rotation exceeds limit, prefer the other direction
    if(accumAzTarget>AZ_WRAP_LIMIT)accumAzTarget-=360.0f;
    if(accumAzTarget<-AZ_WRAP_LIMIT)accumAzTarget+=360.0f;
    azStepper.moveTo((long)(accumAzTarget*STEPS_PER_DEG));
  }
  elStepper.moveTo((long)(calcEl*STEPS_PER_DEG));
  azStepper.run();elStepper.run();
  if(elStepper.currentPosition()<0)elStepper.setCurrentPosition(0);
  currentAz=fmodf(azStepper.currentPosition()/STEPS_PER_DEG+3600.0f,360.0f);
  currentEl=elStepper.currentPosition()/STEPS_PER_DEG;
  if(currentEl<0)currentEl=0;
  isMoving=azStepper.isRunning()||elStepper.isRunning();

  // IMU at 20 Hz
  static unsigned long lastImuMs=0;
  if(imuOK&&millis()-lastImuMs>=50){mpuRead();mpuCorrect(currentEl,elStepper,STEPS_PER_DEG,isMoving);lastImuMs=millis();}

  // Auto-park check
  checkAutopark();

  static unsigned long lastLog=0;
  if(onboardMode){
    if(tleLoaded&&ntpSynced&&millis()-lastSGP4Update>1000){runSGP4();lastSGP4Update=millis();}
    if(millis()-lastLog>5000){
      if(!tleLoaded)Serial.println("[SGP4] Waiting for TLE...");
      else if(!ntpSynced)Serial.println("[NTP] Waiting...");
      else Serial.printf("[TRACK] AZ%.1f EL%.1f IMU p%.2f r%.2f WX %.1fm/s\n",(float)targetAz,(float)targetEl,imuPitch,imuRoll,weather.windMps);
      lastLog=millis();
    }
  } else {
    if(millis()-lastLog>10000){Serial.println("[NET] Listening 4533");lastLog=millis();}
  }
}


