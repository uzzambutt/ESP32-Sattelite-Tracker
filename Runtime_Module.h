#ifndef RUNTIME_MODULE_H
#define RUNTIME_MODULE_H

// ================= EASYCOMM =================
void parseEasyComm(String cmd){
  cmd.trim();if(cmd.length()<1)return;
  if(cmd.startsWith("P ")||cmd.startsWith("p ")){int fs=cmd.indexOf(' '),ss=cmd.indexOf(' ',fs+1);if(fs!=-1&&ss!=-1){targetAz=cmd.substring(fs+1,ss).toFloat();targetEl=cmd.substring(ss+1).toFloat();}}
  else if(cmd.indexOf("AZ")!=-1||cmd.indexOf("az")!=-1){
    int ai=cmd.indexOf("AZ");if(ai==-1)ai=cmd.indexOf("az");int ei=cmd.indexOf("EL");if(ei==-1)ei=cmd.indexOf("el");
    if(ai!=-1&&ei!=-1){targetAz=cmd.substring(ai+2,ei).toFloat();int ee=cmd.indexOf(' ',ei);if(ee==-1)ee=cmd.length();targetEl=cmd.substring(ei+2,ee).toFloat();}
  }
  else if(cmd=="p"||cmd=="P"){if(tcpClient)tcpClient.printf("AZ%.2f EL%.2f\n",(float)currentAz,(float)currentEl);}
}

// ================= SGP4 RUNTIME =================
void runSGP4(){
  unsigned long now=timeClient.getEpochTime();if(now<1000000000UL)return;
  sat.findsat(now);
  unsigned long cm=millis();
  if(lastSatDist>0&&cm>lastDopplerTime){double dt=(cm-lastDopplerTime)/1000.0;if(dt>0){double rr=(sat.satDist-lastSatDist)/dt;dopplerFreq=(float)(145800000.0*(-rr/299792.458));}}
  lastSatDist=sat.satDist;lastDopplerTime=cm;satDistance=(float)sat.satDist;
  targetAz=(float)sat.satAz;targetEl=(sat.satEl<0)?0:(float)sat.satEl;
  satAltitude  =(float)sat.satAlt;   // (A4) publish true altitude for the footprint screen
  satFootprintKm=computeFootprintKm(sat.satAlt);   // (A4) true altitude, not slant range
  if(isGeoSat)nextMaxEl=(float)sat.satEl;
  if(isMoving||targetEl>0)lastActiveMs=millis();
  if(sat.satEl>=0){
    if(!passInProgress){passInProgress=true;passMaxEl=(float)sat.satEl;time_t t=(time_t)now;struct tm tn;gmtime_r(&t,&tn);snprintf(passStartBuf,sizeof(passStartBuf),"%02d:%02d:%02d",tn.tm_hour,tn.tm_min,tn.tm_sec);}
    else if((float)sat.satEl>passMaxEl)passMaxEl=(float)sat.satEl;
  } else if(passInProgress){
    passInProgress=false;if(!isGeoSat)logPass();
    // Auto-advance queue after pass
    if(queueCount>1){Serial.println("[QUEUE] Pass ended, checking next...");advanceQueue();}
  }
}

#endif // RUNTIME_MODULE_H
