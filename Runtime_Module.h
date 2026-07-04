#ifndef RUNTIME_MODULE_H
#define RUNTIME_MODULE_H



// ================= EASYCOMM / HAMLIB =================
bool parseEasyComm(String cmd){
  cmd.trim();if(cmd.length()<1)return false;
  
  if(cmd=="p"){
    if(tcpClient)tcpClient.printf("%.1f\n%.1f\n",(float)currentAz,(float)currentEl);
    return true;
  }
  else if(cmd.startsWith("P ")){
    int fs=cmd.indexOf(' '),ss=cmd.indexOf(' ',fs+1);
    if(fs!=-1&&ss!=-1){
      targetAz=cmd.substring(fs+1,ss).toFloat();
      targetEl=cmd.substring(ss+1).toFloat();
      if(tcpClient)tcpClient.println("RPRT 0");
      return true;
    }
  }
  else if(cmd=="S" || cmd=="S "){
    if(tcpClient)tcpClient.println("RPRT 0");
    return true;
  }
  else if(cmd.indexOf("AZ")!=-1||cmd.indexOf("az")!=-1){
    int ai=cmd.indexOf("AZ");if(ai==-1)ai=cmd.indexOf("az");
    int ei=cmd.indexOf("EL");if(ei==-1)ei=cmd.indexOf("el");
    if(ai!=-1&&ei!=-1){
      targetAz=cmd.substring(ai+2,ei).toFloat();
      int ee=cmd.indexOf(' ',ei);if(ee==-1)ee=cmd.length();
      targetEl=cmd.substring(ei+2,ee).toFloat();
    }
  }
  else if(cmd=="AZ EL" || cmd=="P"){
    if(tcpClient)tcpClient.printf("AZ%.1f EL%.1f\n",(float)currentAz,(float)currentEl);
    return true;
  }
  return false;
}

// ================= SGP4 RUNTIME =================
void runSGP4(){
  unsigned long now=timeClient.getEpochTime();if(now<1000000000UL)return;
  unsigned long cm=millis();
  
  if (isSun || isMoon) {
    float az=0, el=0;
    if (isSun) { getSunPosition(now, obsLat, obsLon, az, el); satDistance = 149597870.0; }
    else       { getMoonPosition(now, obsLat, obsLon, az, el); satDistance = 384400.0; }
    
    targetAz = az;
    targetEl = (el < 0) ? 0 : el;
    dopplerFreq = 0; doppler435 = 0; doppler1268 = 0; polarizationSquint = 0;
    
    if (el >= 0) {
      if (!passInProgress) {
        passInProgress = true; passMaxEl = el;
        time_t t=(time_t)now;struct tm tn;gmtime_r(&t,&tn);
        snprintf(passStartBuf,sizeof(passStartBuf),"%02d:%02d:%02d",tn.tm_hour,tn.tm_min,tn.tm_sec);
      }
      if (el > passMaxEl) passMaxEl = el;
    } else if (el < 0 && passInProgress) {
      passInProgress = false; logPass();
    }
    if (isMoving || targetEl > 0) lastActiveMs = cm;
    return;
  }

  sat.findsat(now);
  if(lastSatDist>0&&cm>lastDopplerTime){
    double dt=(cm-lastDopplerTime)/1000.0;
    if(dt>0){
      double rr=(sat.satDist-lastSatDist)/dt;
      dopplerFreq=(float)(145800000.0*(-rr/299792.458));
      doppler435  =(float)(435800000.0*(-rr/299792.458));
      doppler1268 =(float)(1268000000.0*(-rr/299792.458));
    }
  }
  lastSatDist=sat.satDist;lastDopplerTime=cm;satDistance=(float)sat.satDist;
  targetAz=(float)sat.satAz;
  float rawEl=(sat.satEl<0)?0:(float)sat.satEl;
  // Atmospheric refraction correction (Bennett 1982).
  // Lifts the apparent elevation for low-angle passes so the antenna
  // points at the refracted position, not the geometric one.
  if(rawEl>0.5f&&rawEl<85.0f){
    float refr=1.02f/(60.0f*tanf((rawEl+10.3f/(rawEl+5.11f))*PI/180.0f));
    rawEl+=refr;
    if(rawEl>90.0f)rawEl=90.0f;
  }
  targetEl=rawEl;
  satAltitude  =(float)sat.satAlt;
  satFootprintKm=computeFootprintKm(sat.satAlt);
  satLat=(float)sat.satLat;   // sub-satellite geodetic latitude (degrees)
  satLon=(float)sat.satLon;   // sub-satellite longitude (degrees)
  polarizationSquint = computeSquint(obsLat, obsLon, satLat, satLon);
  if(isGeoSat)nextMaxEl=(float)sat.satEl;
  if(isMoving||targetEl>0)lastActiveMs=millis();
  if(sat.satEl>=0){
    if(!passInProgress){passInProgress=true;passMaxEl=(float)sat.satEl;time_t t=(time_t)now;struct tm tn;gmtime_r(&t,&tn);snprintf(passStartBuf,sizeof(passStartBuf),"%02d:%02d:%02d",tn.tm_hour,tn.tm_min,tn.tm_sec);}
    else if((float)sat.satEl>passMaxEl)passMaxEl=(float)sat.satEl;
  } else if(passInProgress){
    passInProgress=false;if(!isGeoSat)logPass();
    if(queueCount>1){Serial.println("[QUEUE] Pass ended, checking next...");advanceQueue();}
  }
}

#endif // RUNTIME_MODULE_H
