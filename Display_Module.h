#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

// ================= BOOT SCREENS =================
void drawBootScreen(){
  tft.fillScreen(C_BLACK);
  // purple header bar, green accent line (B/C)
  tft.fillRect(0,0,240,28,C_HDRBG);tft.fillRect(0,28,240,2,C_ACCENT);
  tft.fillRect(0,0,4,28,C_ACCENT);tft.fillRect(236,0,4,28,C_PRIMARY);
  tft.setTextColor(C_ACCENT);tft.setTextSize(1);
  tft.setCursor(8,5);tft.print("ESP32 SATELLITE TRACKER");
  tft.setTextColor(C_PRIMARY);tft.setCursor(8,16);tft.print("v10.0  //  ORBITAL OPS");
  tft.setTextColor(C_MGRAY);tft.setCursor(8,38);tft.print("DEVELOPER:");
  tft.setTextColor(C_WHITE);tft.setCursor(8,50);tft.print("Muhammad Uzzam Butt");
  tft.drawFastHLine(4,64,232,C_DGRAY);
  tft.setTextColor(C_MGRAY);tft.setCursor(8,70);tft.print("SCAN FOR SOURCE CODE:");
  const char* url="https://github.com/uzzambutt/ESP32-Sattelite-Tracker";
  uint8_t *qr=(uint8_t*)malloc(qrcodegen_BUFFER_LEN_MAX);
  uint8_t *tmp=(uint8_t*)malloc(qrcodegen_BUFFER_LEN_MAX);
  if(qr&&tmp){
    if(qrcodegen_encodeText(url,tmp,qr,qrcodegen_Ecc_LOW,
       qrcodegen_VERSION_MIN,qrcodegen_VERSION_MAX,qrcodegen_Mask_AUTO,true)){
      int qs=qrcodegen_getSize(qr),ps=3;
      int ox=(240-qs*ps)/2,oy=84;
      tft.fillRect(ox-4,oy-4,qs*ps+8,qs*ps+8,C_WHITE);
      for(int y=0;y<qs;y++)
        for(int x=0;x<qs;x++)
          if(qrcodegen_getModule(qr,x,y))
            tft.fillRect(ox+x*ps,oy+y*ps,ps,ps,C_BLACK);
    }
  }
  if(qr)free(qr);if(tmp)free(tmp);
  // tty-style footer prompt
  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_ACCENT);
  tft.setTextColor(C_PRIMARY);tft.setCursor(8,307);tft.print("user@orbital");
  tft.setTextColor(C_MGRAY);tft.setCursor(96,307);tft.print(":~$ boot");
}

// â”€â”€ Linux-tty style boot log (C) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Each line prints a dmesg-ish status tag then the message, on a black
// terminal. Auto-scrolls when it hits the bottom. A blinking block
// cursor sits after the last line like a real VT.
//   status: 0=OK(green) 1=FAILED(red) 2=BUSY/..(purple) 3=WARN(orange) 4=info(white)
void printBootLine(String msg,int status){
  Serial.println("[BOOT] "+msg);
  const int lineH=11, topY=28, botY=300, headerH=22;
  if(terminalY+lineH>botY){
    // scroll: clear region and restart under a fresh tty header
    tft.fillScreen(C_BLACK);
    tft.fillRect(0,0,240,headerH,C_HDRBG);
    tft.fillRect(0,headerH,240,2,C_ACCENT);
    tft.setTextColor(C_ACCENT);tft.setTextSize(1);
    tft.setCursor(6,7);tft.print("orbital tty1");
    tft.setTextColor(C_MGRAY);tft.setCursor(170,7);tft.print("boot log");
    tft.drawFastHLine(0,headerH+2,240,C_DGRAY);
    terminalY=topY;
  }
  tft.setTextSize(1);
  // status tag in fixed-width bracket
  const char* tag; uint16_t tc;
  switch(status){
    case 0: tag="[  OK  ]"; tc=C_ACCENT; break;
    case 1: tag="[FAILED]"; tc=C_RED;    break;
    case 2: tag="[  ..  ]"; tc=C_PRIMARY;break;
    case 3: tag="[ WARN ]"; tc=C_ORANGE; break;
    default:tag="[ INFO ]"; tc=C_MGRAY;  break;
  }
  tft.setTextColor(tc);tft.setCursor(4,terminalY);tft.print(tag);
  // message â€” truncate to fit ~26 chars after the 52px tag
  tft.setTextColor(C_WHITE);tft.setCursor(54,terminalY);
  if(msg.length()>26)msg=msg.substring(0,26);
  tft.print(msg);
  terminalY+=lineH;
  // blinking block cursor on the next prompt line
  tft.setTextColor(C_ACCENT);tft.setCursor(4,terminalY);tft.print("_");
  delay(70);
  tft.fillRect(4,terminalY,6,8,C_BLACK);  // blink off
}

// legacy entry point â€” kept so existing call sites compile; maps to INFO
void printBootTerminal(String msg){ printBootLine(msg,4); }

void tftDrawAPMode(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,24,C_DRED);tft.fillRect(0,0,4,24,C_RED);tft.fillRect(236,0,4,24,C_RED);
  tft.setTextColor(C_WHITE);tft.setTextSize(1);tft.setCursor(8,5);tft.print("! NETWORK LINK FAILED !");
  tft.setTextColor(C_AMBER);tft.setCursor(8,15);tft.print("ENTERING SETUP MODE");
  tft.drawFastHLine(0,24,240,C_RED);tft.setTextColor(C_RED);tft.setTextSize(2);
  tft.setCursor(10,32);tft.print("WIFI FAIL");
  tft.drawRoundRect(2,58,236,128,4,C_AMBER);tft.fillRect(3,59,234,12,C_PNLA);
  tft.setTextColor(C_AMBER);tft.setTextSize(1);tft.setCursor(6,61);tft.print("[ SETUP INSTRUCTIONS ]");
  tft.drawFastHLine(3,71,234,C_DGRAY);
  tft.setTextColor(C_AMBER);tft.setCursor(6,76);tft.print("1.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,76);tft.print("Join WiFi: AEROSPACE-TRACKER");
  tft.setTextColor(C_AMBER);tft.setCursor(6,92);tft.print("2.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,92);tft.print("Password: groundstation");
  tft.setTextColor(C_AMBER);tft.setCursor(6,108);tft.print("3.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,108);tft.print("Browse to: 192.168.4.1");
  tft.drawRoundRect(2,150,236,24,4,C_DGRAY);
  tft.setTextColor(C_AMBER);tft.setCursor(6,158);tft.print("WAITING FOR CREDENTIALS...");
  tft.fillRect(0,300,240,20,C_DRED);tft.setTextColor(C_WHITE);tft.setCursor(8,307);tft.print("AP MODE  |  192.168.4.1");
}

// ================= STATIC HUD FRAME =================
void tftDrawStaticFrame(){
  tft.fillScreen(C_BLACK);resetTftCache();
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_NEON);tft.fillRect(0,0,3,16,C_AMBER);
  tft.setTextColor(C_NEON);tft.setTextSize(1);tft.setCursor(7,4);tft.print("ORBITAL OPS");
  tft.setTextColor(C_MGRAY);tft.setCursor(96,4);tft.print("UTC");
  tft.drawRect(2,20,236,54,C_NEON);tft.fillRect(3,21,234,11,C_PNLG);
  tft.setTextColor(C_NEON);tft.setCursor(6,23);tft.print("TGT");
  tft.drawFastHLine(3,32,234,C_FAINT);tft.drawFastVLine(118,33,40,C_FAINT);
  tft.setTextColor(C_MGRAY);tft.setCursor(6,40);tft.print("AZ");tft.setCursor(124,40);tft.print("EL");
  const char* cl[5]={"ENG","MOT","L4S","NTP","MEM"};
  for(int i=0;i<5;i++){int x=2+i*48;tft.drawRect(x,78,44,22,C_DGRAY);tft.fillRect(x+1,79,42,8,C_FAINT);tft.setTextColor(C_MGRAY);tft.setCursor(x+13,79);tft.print(cl[i]);}
  tft.drawRect(2,104,236,42,C_AMBER);tft.fillRect(3,105,234,11,C_PNLA);
  tft.setTextColor(C_AMBER);tft.setCursor(6,107);tft.print("PASS PREDICTION");
  tft.drawFastHLine(3,116,234,C_FAINT);tft.drawFastVLine(118,117,28,C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6,120);tft.print("AOS:");tft.setCursor(124,120);tft.print("MAX:");
  tft.setCursor(6,133);tft.print("LOS:");tft.setCursor(124,133);tft.print("T- :");
  tft.setCursor(6,151);tft.print("DST:");tft.setCursor(124,151);tft.print("DOP:");
  tft.fillRect(0,302,240,18,C_HDRBG);tft.fillRect(0,300,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(6,307);
  tft.print(isAPMode?"192.168.4.1":WiFi.localIP().toString());
  tft.setTextColor(C_MGRAY);tft.setCursor(120,307);tft.print(maidenhead);
  newPathReady=true;
}

void drawRadarBackground(){
  tft.drawCircle(RCX,RCY,RR,C_RING);tft.drawCircle(RCX,RCY,RR*2/3,C_RING);tft.drawCircle(RCX,RCY,RR/3,C_RING);
  tft.drawFastVLine(RCX,RCY-RR,RR*2+1,C_RING);tft.drawFastHLine(RCX-RR,RCY,RR*2+1,C_RING);
  for(int i=0;i<360;i+=45){float rad=i*PI/180.0f;tft.drawLine(RCX+(int)((RR-4)*cosf(rad)),RCY+(int)((RR-4)*sinf(rad)),RCX+(int)(RR*cosf(rad)),RCY+(int)(RR*sinf(rad)),(i%90==0)?C_NEON:C_DGRAY);}
  tft.setTextSize(1);tft.setTextColor(C_NEON);
  tft.setCursor(RCX-2,RCY-RR-10);tft.print("N");tft.setCursor(RCX-2,RCY+RR+4);tft.print("S");
  tft.setCursor(RCX+RR+5,RCY-3);tft.print("E");tft.setCursor(RCX-RR-11,RCY-3);tft.print("W");
  tft.setTextColor(C_DGRAY);tft.setCursor(RCX+3,RCY-RR/3-4);tft.print("60");tft.setCursor(RCX+3,RCY-RR*2/3-4);tft.print("30");
}

void drawSigBars(int x,int y,int rssi){
  int lvl=(rssi>-55)?4:(rssi>-65)?3:(rssi>-75)?2:(rssi>-85)?1:0;
  for(int i=0;i<4;i++){int h=3+i*2;uint16_t c=(i<lvl)?((lvl>=3)?C_NEON:(lvl==2)?C_AMBER:C_RED):C_DGRAY;tft.fillRect(x+i*5,y+(9-h),3,h,c);}
}

// ================= TFT SCREEN 1: PASS SCHEDULE =================
void tftDrawScheduleScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_AMBER);
  tft.setTextColor(C_AMBER);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("PASS SCHEDULE  [1/5]");

  if(!scheduleReady){
    tft.setTextColor(C_MGRAY);tft.setCursor(6,30);
    tft.print(scheduleBusy?"COMPUTING...":"NO DATA â€” REFRESH");
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_AMBER);
    tft.setTextColor(C_AMBER);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  int y=22;
  for(int i=0;i<scheduleCount&&y<290;i++){
    time_t aosT=schedule[i].aos;
    struct tm tmA;gmtime_r(&aosT,&tmA);
    char buf[40];
    // (A6) %.12s caps long sat names so a 20-char name can't push the
    // elevation column off the 240px screen.
    snprintf(buf,sizeof(buf),"%02d:%02d  %.12s %5.1f",
             tmA.tm_hour,tmA.tm_min,schedule[i].satName,schedule[i].maxEl);
    bool next=(aosT>(time_t)timeClient.getEpochTime());
    tft.setTextColor(next?C_NEON:C_DGRAY);
    tft.setCursor(4,y);tft.print(buf);
    y+=11;
  }
  if(scheduleCount==0){tft.setTextColor(C_RED);tft.setCursor(6,40);tft.print("NO PASSES IN 24H");}

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_AMBER);
  tft.setTextColor(C_AMBER);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 2: IMU DRIFT GRAPH (feature 11) =================
void tftDrawDriftGraph(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("IMU DRIFT GRAPH  [2/5]");

  // Live values
  tft.setTextColor(C_AMBER);tft.setCursor(4,22);
  char buf[40];
  snprintf(buf,sizeof(buf),"PITCH %+7.2f  ROLL %+7.2f",imuPitch,imuRoll);
  tft.print(buf);
  tft.setTextColor(imuOK?C_NEON:C_RED);tft.setCursor(4,33);
  tft.print(imuOK?"IMU ONLINE":"IMU OFFLINE");

  // Graph axes
  int gx=20,gy=50,gw=200,gh=140;
  tft.drawRect(gx,gy,gw,gh,C_DGRAY);
  tft.drawFastHLine(gx,gy+gh/2,gw,C_FAINT); // zero line
  tft.setTextColor(C_DGRAY);tft.setCursor(4,gy);tft.print("+10");
  tft.setCursor(4,gy+gh/2-4);tft.print("  0");
  tft.setCursor(4,gy+gh-4);tft.print("-10");

  // Plot error history
  int histLen=errHistFull?ERR_HIST:errHistIdx;
  if(histLen>1){
    int startIdx=errHistFull?errHistIdx:0;
    float scale=gh/20.0f; // Â±10 deg full scale
    int lastX=-1,lastY=-1;
    for(int i=0;i<histLen;i++){
      int idx=(startIdx+i)%ERR_HIST;
      float err=errHistory[idx];
      err=constrain(err,-10.0f,10.0f);
      int px=gx+1+(int)((float)i/histLen*(gw-2));
      int py=gy+gh/2-(int)(err*scale);
      py=constrain(py,gy+1,gy+gh-1);
      if(lastX>=0){
        uint16_t c=(fabsf(err)>IMU_CORRECT_THRESH)?C_AMBER:C_NEON;
        tft.drawLine(lastX,lastY,px,py,c);
      }
      lastX=px;lastY=py;
    }
  }

  // Threshold markers
  float thr=IMU_CORRECT_THRESH;
  float scale=gh/20.0f;
  int thrY1=gy+gh/2-(int)(thr*scale);
  int thrY2=gy+gh/2+(int)(thr*scale);
  tft.drawFastHLine(gx,constrain(thrY1,gy,gy+gh),gw,C_DRED);
  tft.drawFastHLine(gx,constrain(thrY2,gy,gy+gh),gw,C_DRED);

  tft.setTextColor(C_MGRAY);tft.setCursor(4,200);
  snprintf(buf,sizeof(buf),"ERR NOW: %+.2f deg",(float)(imuPitch-currentEl));
  tft.print(buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 3: SATELLITE FOOTPRINT =================
void tftDrawFootprintScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_CYAN);
  tft.setTextColor(C_CYAN);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("SAT FOOTPRINT  [3/5]");

  if(!tleLoaded||satDistance<100){
    tft.setTextColor(C_MGRAY);tft.setCursor(6,30);tft.print("NO SATELLITE TRACKED");
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_CYAN);
    tft.setTextColor(C_CYAN);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  const float Re = 6371.0f;
  float altKm    = (float)satAltitude;
  float distKm   = (float)satDistance;
  float el       = (float)targetEl; if(el<0)el=0;
  float az       = (float)targetAz;

  // --- Accurate footprint geometry ---
  // half-angle of footprint at Earth's centre (radians)
  float footAngRad = acosf(Re / (Re + altKm));
  float footKm     = satFootprintKm;           // = Re * footAngRad

  // Elevation mask angle: min elevation at footprint edge = 0 deg by definition.
  // Nadir angle from satellite to footprint edge (at satellite):
  float nadirAngRad = asinf(Re * sinf(PI/2.0f + footAngRad) / (Re + altKm));
  // coverage area on Earth surface (spherical cap, km^2)
  float capArea = 2.0f * PI * Re * Re * (1.0f - cosf(footAngRad));

  // --- Stats panel (top) ---
  char buf[40];
  tft.setTextColor(C_WHITE);tft.setCursor(4,22);
  snprintf(buf,sizeof(buf),"%-20s",satName);tft.print(buf);

  tft.setTextColor(C_MGRAY);
  tft.setCursor(4,33);
  snprintf(buf,sizeof(buf),"ALT %5.0f km  SLANT %5.0f km",altKm,distKm);tft.print(buf);
  tft.setCursor(4,44);
  snprintf(buf,sizeof(buf),"FP-R %5.0f km  ARC %4.1f deg",footKm,footAngRad*180.0f/PI);tft.print(buf);
  tft.setCursor(4,55);
  snprintf(buf,sizeof(buf),"AREA %7.0f km2  EL %4.1f deg",capArea,el);tft.print(buf);

  // --- Overhead map ---
  // Scale: Re = maxR pixels, so the whole hemisphere fits in the circle.
  // The ground circle has radius maxR, representing Re km from observer.
  const int cx=120, cy=182, maxR=88;

  // Horizon ring (= full earth-scale circle)
  tft.drawCircle(cx,cy,maxR,C_DGRAY);
  // 30-deg / 60-deg elevation rings mapped to ground-distance scale
  // Simpler: ring at 1/3 and 2/3 of maxR
  tft.drawCircle(cx,cy,maxR/3,C_FAINT);
  tft.drawCircle(cx,cy,maxR*2/3,C_FAINT);
  // Cross-hairs
  tft.drawFastVLine(cx,cy-maxR,maxR*2,C_FAINT);
  tft.drawFastHLine(cx-maxR,cy,maxR*2,C_FAINT);

  // Observer dot at centre
  tft.fillCircle(cx,cy,3,C_AMBER);
  tft.setTextColor(C_AMBER);tft.setTextSize(1);
  tft.setCursor(cx+5,cy-4);tft.print("OBS");

  // Sub-satellite nadir point position
  float elRad = el * PI / 180.0f;
  float rhoRad = PI/2.0f - elRad - asinf(Re * cosf(elRad) / (Re + altKm));
  if(rhoRad < 0) rhoRad = 0;
  float nadirGroundKm = Re * rhoRad;
  int nadirPx = (int)((nadirGroundKm / Re) * maxR);  // Re km = maxR px
  nadirPx = constrain(nadirPx, 0, maxR);

  float azRad = (az - 90.0f) * PI / 180.0f;
  int nx = cx + (int)(nadirPx * cosf(azRad));
  int ny = cy + (int)(nadirPx * sinf(azRad));
  nx=constrain(nx,cx-maxR,cx+maxR);
  ny=constrain(ny,cy-maxR,cy+maxR);

  // Footprint circle: radius in km mapped to pixels (Re km = maxR px)
  int fpPx = (int)((footKm / Re) * maxR);
  fpPx = constrain(fpPx, 5, maxR*2);

  // Draw footprint circle centred on nadir
  tft.drawCircle(nx,ny,min(fpPx,maxR+10),C_NEON);
  // Inner glow ring
  if(fpPx > 6) tft.drawCircle(nx,ny,fpPx-3,C_PNLG);

  // Satellite dot at sky position (el/az → polar, centre=zenith, edge=horizon)
  float satR = maxR * (1.0f - el / 90.0f);
  int sx = cx + (int)(satR * cosf(azRad));
  int sy = cy + (int)(satR * sinf(azRad));
  sx=constrain(sx,cx-maxR,cx+maxR);
  sy=constrain(sy,cy-maxR,cy+maxR);
  tft.fillCircle(sx,sy,4,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(sx+6,sy-4);tft.print("SAT");

  // Nadir cross
  tft.drawLine(nx-5,ny,nx+5,ny,C_PRIMARY);
  tft.drawLine(nx,ny-5,nx,ny+5,C_PRIMARY);

  // Labels
  tft.setTextColor(C_DGRAY);tft.setTextSize(1);
  tft.setCursor(cx-4,cy-maxR-9);tft.print("N");
  tft.setCursor(cx+maxR+3,cy-4);tft.print("E");

  // Coverage % of Earth surface
  float covPct = (capArea / (4.0f*PI*Re*Re)) * 100.0f;
  tft.setTextColor(C_MGRAY);tft.setCursor(4,273);
  snprintf(buf,sizeof(buf),"COV %.2f%% of Earth",covPct);tft.print(buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_CYAN);
  tft.setTextColor(C_CYAN);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 4: WEATHER (feature 14) =================
// Redesigned (D): city + condition, big temp with degree glyph and
// feels-like, 2x2 stat grid (wind+compass / humidity+pressure), wind
// bar, fetch age, high-wind warning. Purple chrome, green values.
void tftDrawWeatherScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("GROUND WEATHER  [4/5]");

  if(!weather.valid){
    tft.setTextColor(C_MGRAY);tft.setTextSize(1);tft.setCursor(6,30);
    #ifdef OWM_API_KEY
    tft.print("FETCHING...");
    #else
    tft.print("NO API KEY IN secrets.h");
    #endif
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
    tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  char buf[40];
  // Header: city + condition (ASCII-safe via copyAscii in fetchWeather)
  tft.setTextColor(C_WHITE);tft.setTextSize(1);
  tft.setCursor(4,22);snprintf(buf,sizeof(buf),"%.22s",weather.city);tft.print(buf);
  tft.setTextColor(C_MGRAY);tft.setCursor(4,33);snprintf(buf,sizeof(buf),"%.22s",weather.desc);tft.print(buf);

  // Big temperature with proper degree glyph (0xF7 in GFX font)
  tft.setTextColor(C_ACCENT);tft.setTextSize(3);
  tft.setCursor(4,46);snprintf(buf,sizeof(buf),"%+.0f%cC",(double)weather.tempC,0xF7);tft.print(buf);
  // Feels-like, right of the big temp
  tft.setTextSize(1);tft.setTextColor(C_PRIMARY);
  tft.setCursor(150,52);tft.print("FEELS");
  tft.setTextColor(C_WHITE);
  tft.setCursor(150,64);snprintf(buf,sizeof(buf),"%+.0f%cC",(double)weather.feelsC,0xF7);tft.print(buf);

  tft.setTextSize(1);
  tft.drawFastHLine(0,86,240,C_DGRAY);

  // 2x2 stat grid helper
  auto cell=[&](int x,int y,const char*lbl,const char*val,uint16_t vc){
    tft.setTextSize(1);
    tft.setTextColor(C_MGRAY);tft.setCursor(x,y);tft.print(lbl);
    tft.setTextColor(vc);tft.setCursor(x,y+11);tft.print(val);
  };

  // Row 1: wind speed (color by strength) + compass bearing
  snprintf(buf,sizeof(buf),"%.1fm/s",(double)weather.windMps);
  cell(4,94,"WIND",buf,weather.windMps>10?C_RED:weather.windMps>5?C_ORANGE:C_ACCENT);
  snprintf(buf,sizeof(buf),"%.0f%c %s",(double)weather.windDeg,0xF7,windCompass(weather.windDeg));
  cell(124,94,"DIR",buf,C_WHITE);

  // Row 2: humidity + pressure
  snprintf(buf,sizeof(buf),"%d%%",weather.humidity);
  cell(4,124,"HUMID",buf,C_ACCENT);
  snprintf(buf,sizeof(buf),"%dhPa",weather.pressure);
  cell(124,124,"PRES",buf,C_PRIMARY);

  // Wind speed bar 0..20 m/s
  tft.drawRect(4,150,232,12,C_DGRAY);
  int barW=(int)(weather.windMps/20.0f*230);barW=constrain(barW,0,230);
  uint16_t bc=weather.windMps>10?C_RED:weather.windMps>5?C_ORANGE:C_ACCENT;
  if(barW>0)tft.fillRect(5,151,barW,10,bc);
  tft.setTextColor(C_DGRAY);tft.setCursor(4,165);tft.print("0");tft.setCursor(214,165);tft.print("20m/s");

  if(weather.windMps>10){
    tft.setTextColor(C_RED);tft.setCursor(4,182);tft.print("! HIGH WIND - CHECK PARK");
  }

  // Fetch time + station location
  struct tm ft;time_t ft2=weather.fetchedAt;gmtime_r(&ft2,&ft);
  tft.setTextColor(C_DGRAY);tft.setCursor(4,250);
  snprintf(buf,sizeof(buf),"UPDATED %02d:%02d UTC",ft.tm_hour,ft.tm_min);tft.print(buf);
  tft.setCursor(4,262);
  snprintf(buf,sizeof(buf),"STATION %.4f,%.4f",(double)obsLat,(double)obsLon);tft.print(buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 5: ORBITAL ELEMENTS =================
void tftDrawOrbitalScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("ORBITAL ELEMENTS  [5/6]");

  if(!tleElem.valid){
    tft.setTextColor(C_MGRAY);tft.setCursor(6,30);tft.print("NO TLE LOADED");
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
    tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  char buf[40];
  // Satellite name + NORAD ID
  tft.setTextColor(C_WHITE);tft.setTextSize(1);
  tft.setCursor(4,22);
  snprintf(buf,sizeof(buf),"%-20s",satName);tft.print(buf);
  tft.setTextColor(C_MGRAY);tft.setCursor(4,33);
  snprintf(buf,sizeof(buf),"NORAD: %d",tleElem.catalogNum);tft.print(buf);

  // Divider
  tft.drawFastHLine(4,44,232,C_DGRAY);

  // Elements table
  auto row=[&](int y,const char* label,const char* val,uint16_t vc=C_NEON){
    tft.setTextColor(C_MGRAY);tft.setCursor(4,y);tft.print(label);
    tft.setTextColor(vc);tft.setCursor(110,y);tft.print(val);
  };

  snprintf(buf,sizeof(buf),"%.3f deg",tleElem.inclination); row(52,"INCLINATION",buf);
  snprintf(buf,sizeof(buf),"%.2f deg",tleElem.raan);         row(63,"RAAN",buf);
  snprintf(buf,sizeof(buf),"%.6f",tleElem.eccentricity);    row(74,"ECCENTRICITY",buf);
  snprintf(buf,sizeof(buf),"%.0f km",tleElem.perigeeAlt);   row(85,"PERIGEE ALT",buf);
  snprintf(buf,sizeof(buf),"%.0f km",tleElem.apogeeAlt);    row(96,"APOGEE ALT",buf);
  snprintf(buf,sizeof(buf),"%.2f min",tleElem.period);      row(107,"PERIOD",buf);
  snprintf(buf,sizeof(buf),"%.4f rev/day",tleElem.meanMotion);row(118,"MEAN MOTION",buf);

  // Divider
  tft.drawFastHLine(4,131,232,C_DGRAY);

  // TLE age
  float age=tleElem.ageDays;
  uint16_t ageColor=(age>14)?C_RED:(age>7)?C_ORANGE:C_NEON;
  snprintf(buf,sizeof(buf),"%.1f DAYS%s",age,age>7?" (STALE)":"");
  row(138,"TLE AGE",buf,ageColor);

  // Orbit type
  bool isLEO=(tleElem.perigeeAlt<2000&&tleElem.perigeeAlt>200);
  bool isMEO=(tleElem.perigeeAlt>=2000&&tleElem.perigeeAlt<35000);
  bool isGEO_=(tleElem.perigeeAlt>=35000);
  const char* orbitType=isGEO_?"GEO":isMEO?"MEO":isLEO?"LEO":"SPECIAL";
  snprintf(buf,sizeof(buf),"%s",orbitType);
  row(149,"ORBIT TYPE",buf,C_PRIMARY);

  // Semi-major axis
  snprintf(buf,sizeof(buf),"%.0f km",tleElem.semiMajor);
  row(160,"SEMI-MAJOR A",buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= DYNAMIC HUD (screen 0) =================
void tftUpdateDynamic(){
  if(spiLock||isAPMode) return;
  char buf[32];

  time_t nowT=(time_t)timeClient.getEpochTime();
  struct tm tmN;gmtime_r(&nowT,&tmN);
  char ck[10];snprintf(ck,sizeof(ck),"%02d:%02d:%02d",tmN.tm_hour,tmN.tm_min,tmN.tm_sec);
  if(strcmp(ck,prev_clock)!=0){
    tft.fillRect(122,4,50,9,C_HDRBG);tft.setTextColor(C_WHITE);tft.setTextSize(1);
    tft.setCursor(122,4);tft.print(ck);strcpy(prev_clock,ck);
  }
  if((int)ntpSynced!=prev_ntp){
    tft.fillCircle(228,8,3,ntpSynced?C_NEON:C_RED);prev_ntp=ntpSynced;
    tft.fillRect(147,88,42,11,C_BLACK);
    tft.setTextColor(ntpSynced?C_NEON:C_RED);tft.setTextSize(1);
    tft.setCursor(150,90);tft.print(ntpSynced?"SYNC":"ERR");
  }
  if(strcmp(satName,prev_sat)!=0){
    tft.fillRect(30,21,207,11,C_PNLG);tft.setTextColor(C_AMBER);tft.setTextSize(1);
    tft.setCursor(32,23);snprintf(buf,sizeof(buf),"%.22s%s",satName,isGeoSat?" GEO":"");tft.print(buf);
    strncpy(prev_sat,satName,24);
  }
  float cAz=currentAz,tAz=targetAz,cEl=currentEl,tEl=targetEl;
  if(fabsf(cAz-prev_cAz)>0.05f||fabsf(tAz-prev_tAz)>0.05f){
    tft.fillRect(22,36,94,16,C_BLACK);tft.setTextColor(C_NEON);tft.setTextSize(2);
    tft.setCursor(22,36);snprintf(buf,sizeof(buf),"%05.1f",(double)cAz);tft.print(buf);
    float dAz=tAz-cAz;if(dAz>180)dAz-=360;if(dAz<-180)dAz+=360;
    tft.fillRect(6,58,110,9,C_BLACK);tft.setTextSize(1);tft.setTextColor(C_WHITE);
    tft.setCursor(6,58);snprintf(buf,sizeof(buf),">%05.1f",(double)tAz);tft.print(buf);
    tft.setTextColor(fabsf(dAz)>1.0f?C_AMBER:C_MGRAY);
    tft.setCursor(60,58);snprintf(buf,sizeof(buf),"%+06.1f",(double)dAz);tft.print(buf);
    prev_cAz=cAz;prev_tAz=tAz;
  }
  if(fabsf(cEl-prev_cEl)>0.05f||fabsf(tEl-prev_tEl)>0.05f){
    tft.fillRect(140,36,96,16,C_BLACK);tft.setTextColor(C_AMBER);tft.setTextSize(2);
    tft.setCursor(140,36);snprintf(buf,sizeof(buf),"%04.1f",(double)cEl);tft.print(buf);
    float dEl=tEl-cEl;
    tft.fillRect(124,58,110,9,C_BLACK);tft.setTextSize(1);tft.setTextColor(C_WHITE);
    tft.setCursor(124,58);snprintf(buf,sizeof(buf),">%04.1f",(double)tEl);tft.print(buf);
    tft.setTextColor(fabsf(dEl)>1.0f?C_AMBER:C_MGRAY);
    tft.setCursor(172,58);snprintf(buf,sizeof(buf),"%+05.1f",(double)dEl);tft.print(buf);
    prev_cEl=cEl;prev_tEl=tEl;
  }
  tft.setTextSize(1);
  int modeVal=onboardMode?1:0;
  if(modeVal!=prev_mode){tft.fillRect(3,88,42,11,C_BLACK);tft.setTextColor(onboardMode?C_NEON:C_AMBER);tft.setCursor(6,90);tft.print(onboardMode?"SGP4":"L4S");prev_mode=modeVal;}
  if((int)isMoving!=prev_moving){tft.fillRect(51,88,42,11,C_BLACK);tft.setTextColor(isMoving?C_AMBER:C_NEON);tft.setCursor(54,90);tft.print(isMoving?"SLEW":"IDLE");prev_moving=isMoving;}
  bool l4s=(tcpClient&&tcpClient.connected());
  if((int)l4s!=prev_l4s){tft.fillRect(99,88,42,11,C_BLACK);tft.setTextColor(l4s?C_NEON:C_MGRAY);tft.setCursor(102,90);tft.print(l4s?"LINK":"----");prev_l4s=l4s;}
  int heapK=ESP.getFreeHeap()/1024;
  if(abs(heapK-prev_heap)>2){tft.fillRect(195,88,42,11,C_BLACK);tft.setTextColor(heapK>50?C_MGRAY:C_RED);tft.setCursor(198,90);snprintf(buf,sizeof(buf),"%dK",heapK);tft.print(buf);prev_heap=heapK;}
  if(fabsf(satDistance-prev_dist)>0.5f){
    tft.fillRect(32,151,84,9,C_BLACK);tft.setTextColor(C_WHITE);tft.setCursor(32,151);
    if(satDistance>0)snprintf(buf,sizeof(buf),"%.0fkm",(double)satDistance);else strcpy(buf,"---");
    tft.print(buf);prev_dist=satDistance;
  }
  if(fabsf(dopplerFreq-prev_dop)>1.0f){
    tft.fillRect(150,151,86,9,C_BLACK);tft.setTextColor(C_NEON);tft.setCursor(150,151);
    snprintf(buf,sizeof(buf),"%+05.0fHz",(double)dopplerFreq);tft.print(buf);prev_dop=dopplerFreq;
  }
  int rssi=isAPMode?-100:WiFi.RSSI();
  if(abs(rssi-prev_rssi)>2){tft.fillRect(198,304,40,14,C_HDRBG);drawSigBars(200,306,rssi);prev_rssi=rssi;}

  unsigned long ne=timeClient.getEpochTime();
  int gs=isGeoSat?1:0;
  long cd=(nextAosTime>0)?(long)(nextAosTime-(time_t)ne):-999999;
  bool rdr=(gs!=prev_geo)||(gs&&fabsf(nextMaxEl-prev_maxElShown)>0.05f)||(!gs&&cd!=prev_countdown);
  if(rdr){
    tft.fillRect(32,120,84,9,C_BLACK);tft.fillRect(32,133,84,9,C_BLACK);
    tft.fillRect(150,120,86,9,C_BLACK);tft.fillRect(150,133,86,9,C_BLACK);
    if(gs){
      tft.setTextColor(C_NEON);tft.setCursor(32,120);tft.print("GEO ORBIT");tft.setCursor(32,133);tft.print("STATIC");
      tft.setTextColor(C_AMBER);tft.setCursor(150,120);snprintf(buf,sizeof(buf),"%.1f deg",(double)nextMaxEl);tft.print(buf);
      tft.setCursor(150,133);if(nextMaxEl>0){tft.setTextColor(C_NEON);tft.print("LOCKED");}else{tft.setTextColor(C_RED);tft.print("NO VIS");}
    } else if(nextAosTime>0){
      time_t at=nextAosTime,lt=nextLosTime;struct tm ta,tl;gmtime_r(&at,&ta);gmtime_r(&lt,&tl);
      tft.setTextColor(C_WHITE);tft.setCursor(32,120);snprintf(buf,sizeof(buf),"%02d:%02d:%02d",ta.tm_hour,ta.tm_min,ta.tm_sec);tft.print(buf);
      tft.setCursor(32,133);snprintf(buf,sizeof(buf),"%02d:%02d:%02d",tl.tm_hour,tl.tm_min,tl.tm_sec);tft.print(buf);
      tft.setTextColor(C_AMBER);tft.setCursor(150,120);snprintf(buf,sizeof(buf),"%.1f deg",(double)nextMaxEl);tft.print(buf);
      tft.setCursor(150,133);
      if(cd<=0&&(time_t)ne<lt){tft.setTextColor(C_NEON);tft.print("TRACKING");}
      else if(cd>0){tft.setTextColor(C_ORANGE);snprintf(buf,sizeof(buf),"-%02ld:%02ld:%02ld",cd/3600,(cd%3600)/60,cd%60);tft.print(buf);}
      else{tft.setTextColor(C_MGRAY);tft.print("ACQUIRING");}
    } else {
      tft.setTextColor(C_RED);tft.setCursor(32,120);tft.print(tleLoaded?"SEARCHING":"NO TLE");tft.setCursor(32,133);tft.print("--:--:--");
    }
    prev_geo=gs;prev_countdown=cd;prev_maxElShown=nextMaxEl;
  }
}

// ================= RADAR ENGINE =================
void tftRadarUpdate(){
  if(spiLock||isAPMode) return;
  auto toXY=[](float az,float el,int &x,int &y){
    float se=(el<0)?0:(el>90)?90:el;float rr=RR*(1-se/90.0f);float rad=(az-90)*PI/180.0f;
    x=clampX(RCX+(int)(rr*cosf(rad)));y=clampY(RCY+(int)(rr*sinf(rad)));
  };
  auto drawDash=[](int tx,int ty,uint16_t c){
    float dx=tx-RCX,dy=ty-RCY,len=sqrtf(dx*dx+dy*dy);if(len<1)return;
    float ux=dx/len,uy=dy/len,pos=0;bool on=true;
    while(pos<len){float end=pos+(on?6.0f:4.0f);if(end>len)end=len;
      if(on)tft.drawLine(clampX(RCX+(int)(ux*pos)),clampY(RCY+(int)(uy*pos)),clampX(RCX+(int)(ux*end)),clampY(RCY+(int)(uy*end)),c);
      pos=end;on=!on;}
  };
  auto eraseDash=[&drawDash](int tx,int ty){drawDash(tx,ty,C_BLACK);};
  auto drawDiamond=[](int cx,int cy,int r,uint16_t c){
    tft.drawLine(clampX(cx),clampY(cy-r),clampX(cx+r),clampY(cy),c);
    tft.drawLine(clampX(cx+r),clampY(cy),clampX(cx),clampY(cy+r),c);
    tft.drawLine(clampX(cx),clampY(cy+r),clampX(cx-r),clampY(cy),c);
    tft.drawLine(clampX(cx-r),clampY(cy),clampX(cx),clampY(cy-r),c);
  };
  if(newPathReady){
    tft.fillRect(RCX-RR-14,RCY-RR-14,(RR+14)*2,(RR+14)*2,C_BLACK);drawRadarBackground();
    prev_antX=prev_antY=prev_tgtX=prev_tgtY=prev_satX=prev_satY=-1;prev_alX[0]=prev_alX[1]=-1;newPathReady=false;
  } else {
    if(prev_antX>=0){tft.drawLine(RCX,RCY,prev_antX,prev_antY,C_BLACK);tft.fillCircle(prev_antX,prev_antY,4,C_BLACK);tft.fillRect(clampX(prev_antX+(prev_antX>=RCX?6:-26)),clampY(prev_antY+(prev_antY>=RCY?6:-10)),24,9,C_BLACK);}
    if(prev_tgtX>=0){eraseDash(prev_tgtX,prev_tgtY);drawDiamond(prev_tgtX,prev_tgtY,5,C_BLACK);tft.fillRect(clampX(prev_tgtX+(prev_tgtX>=RCX?8:-30)),clampY(prev_tgtY+(prev_tgtY>=RCY?7:-11)),24,9,C_BLACK);}
    if(prev_satX>=0){tft.fillCircle(prev_satX,prev_satY,4,C_BLACK);tft.drawCircle(prev_satX,prev_satY,7,C_BLACK);}
    for(int k=0;k<2;k++)if(prev_alX[k]>=0)tft.fillRect(prev_alX[k],prev_alY[k],7,9,C_BLACK);
    tft.drawCircle(RCX,RCY,RR,C_RING);tft.drawCircle(RCX,RCY,RR*2/3,C_RING);tft.drawCircle(RCX,RCY,RR/3,C_RING);
    tft.drawFastVLine(RCX,RCY-RR+1,RR*2-1,C_RING);tft.drawFastHLine(RCX-RR+1,RCY,RR*2-1,C_RING);
  }
  // Satellite footprint circle on radar (feature 13)
  if(targetEl>0.0f&&satFootprintKm>0){
    int sx,sy;toXY((float)targetAz,(float)targetEl,sx,sy);
    int fpPx=(int)((satFootprintKm/12000.0f)*RR);fpPx=constrain(fpPx,4,RR-2);
    tft.drawCircle(clampX(sx),clampY(sy),fpPx,C_DGRAY);
  }
  // Trajectory
  int len=(isGeoSat||predBusy)?0:globalPathLen;
  prev_alX[0]=prev_alX[1]=-1;
  if(len>1){
    int lx=-1,ly=-1;tft.setTextSize(1);
    for(int i=0;i<len;i++){
      int px,py;toXY(globalPath[i].az,globalPath[i].el,px,py);
      if(lx!=-1)tft.drawLine(lx,ly,px,py,C_AMBER);
      if(i==0){tft.setTextColor(C_NEON);int ax=clampX(px-3),ay=clampY(py-10);tft.setCursor(ax,ay);tft.print("A");prev_alX[0]=ax;prev_alY[0]=ay;}
      else if(i==len-1){tft.setTextColor(C_RED);int ax=clampX(px-3),ay=clampY(py-10);tft.setCursor(ax,ay);tft.print("L");prev_alX[1]=ax;prev_alY[1]=ay;}
      lx=px;ly=py;
    }
  }
  // Target arrow
  int tgtX=-1,tgtY=-1;
  if(targetEl>0.0f){
    toXY((float)targetAz,(float)targetEl,tgtX,tgtY);
    drawDash(tgtX,tgtY,C_AMBER);drawDiamond(tgtX,tgtY,5,C_AMBER);
    tft.setTextSize(1);tft.setTextColor(C_AMBER);
    tft.setCursor(clampX(tgtX+(tgtX>=RCX?8:-30)),clampY(tgtY+(tgtY>=RCY?7:-11)));tft.print("TGT");
  }
  prev_tgtX=tgtX;prev_tgtY=tgtY;
  // Actual needle
  float sae=(float)currentEl<0?0:(float)currentEl;int ax,ay;
  toXY((float)currentAz,sae,ax,ay);
  tft.drawLine(RCX,RCY,ax,ay,C_MGRAY);tft.fillCircle(ax,ay,4,C_WHITE);
  tft.setTextSize(1);tft.setTextColor(C_WHITE);
  tft.setCursor(clampX(ax+(ax>=RCX?6:-26)),clampY(ay+(ay>=RCY?6:-10)));tft.print("ANT");
  prev_antX=ax;prev_antY=ay;
  // Satellite dot
  if(targetEl>0.0f){
    int sx,sy;toXY((float)targetAz,(float)targetEl,sx,sy);
    uint16_t c=isGeoSat?C_AMBER:C_NEON;tft.fillCircle(sx,sy,4,c);tft.drawCircle(sx,sy,7,c);
    prev_satX=sx;prev_satY=sy;
  } else prev_satX=-1;
}

#endif // DISPLAY_MODULE_H

