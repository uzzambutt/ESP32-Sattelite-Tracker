#ifndef CELESTIAL_MATH_H
#define CELESTIAL_MATH_H

#include <Arduino.h>

// Calculates precise Sun Azimuth and Elevation for solar tracking
void getSunPosition(time_t unixTime, double lat, double lon, float &az, float &el) {
  double JD = (unixTime / 86400.0) + 2440587.5;
  double n = JD - 2451545.0; 
  double L = fmod(280.460 + 0.9856474 * n, 360.0);
  double g = fmod(357.528 + 0.9856003 * n, 360.0) * PI / 180.0;
  double lambda = (L + 1.915 * sin(g) + 0.020 * sin(2*g)) * PI / 180.0;
  double eps = (23.439 - 0.0000004 * n) * PI / 180.0;
  
  double alpha = atan2(cos(eps) * sin(lambda), cos(lambda));
  double delta = asin(sin(eps) * sin(lambda));
  
  double GMST = fmod(280.46061837 + 360.98564736629 * n, 360.0);
  double H = (fmod(GMST + lon, 360.0) * PI / 180.0) - alpha;
  
  double latR = lat * PI / 180.0;
  double sinEl = sin(latR)*sin(delta) + cos(latR)*cos(delta)*cos(H);
  el = asin(sinEl) * 180.0 / PI;
  
  double cosAz = (sin(delta) - sin(latR)*sinEl) / (cos(latR) * cos(asin(sinEl)));
  az = acos(constrain(cosAz, -1.0, 1.0)) * 180.0 / PI;
  if (sin(H) > 0) az = 360.0 - az;
}

// Calculates approximate Moon Azimuth and Elevation for EME (Moonbounce)
void getMoonPosition(time_t unixTime, double lat, double lon, float &az, float &el) {
  double d = (unixTime / 86400.0) + 2440587.5 - 2451545.0;
  double L = (218.316 + 13.176396 * d);
  double M = (134.963 + 13.064993 * d) * PI / 180.0;
  double F = (93.272 + 13.229350 * d) * PI / 180.0;
  
  double l = L + 6.289 * sin(M);
  double b = 5.128 * sin(F);
  double lambda = l * PI / 180.0;
  double beta = b * PI / 180.0;
  double eps = (23.439 - 0.0000004 * d) * PI / 180.0;
  
  double alpha = atan2(sin(lambda)*cos(eps) - tan(beta)*sin(eps), cos(lambda));
  double delta = asin(sin(beta)*cos(eps) + cos(beta)*sin(eps)*sin(lambda));
  
  double GMST = fmod(280.46061837 + 360.98564736629 * d, 360.0);
  double H = (fmod(GMST + lon, 360.0) * PI / 180.0) - alpha;
  
  double latR = lat * PI / 180.0;
  double sinEl = sin(latR)*sin(delta) + cos(latR)*cos(delta)*cos(H);
  el = asin(sinEl) * 180.0 / PI;
  
  double cosAz = (sin(delta) - sin(latR)*sinEl) / (cos(latR) * cos(asin(sinEl)));
  az = acos(constrain(cosAz, -1.0, 1.0)) * 180.0 / PI;
  if (sin(H) > 0) az = 360.0 - az;
}

// Calculates geometric Faraday Polarization Angle (Squint) for Yagi twisting
float computeSquint(double obsLat, double obsLon, double satLat, double satLon) {
  double dLon = (satLon - obsLon) * PI / 180.0;
  double lat1 = obsLat * PI / 180.0;
  double lat2 = satLat * PI / 180.0;
  double y = sin(dLon) * cos(lat2);
  double x = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dLon);
  return atan2(y, x) * 180.0 / PI;
}

#endif
