#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── IMU filter parameters ──────────────────────────────────────
#define MPU_ADDR       0x68
#define MPU_SDA        21
#define MPU_SCL        22
#define MPU_DLPF_CFG   4      // hardware 21 Hz LPF
#define ACCEL_LPF      0.15f  // software accel pre-smoother
#define GYRO_DEAD      0.8f   // deg/s deadband
#define SPIKE_LSB      800.0f // accel spike rejection threshold

// ── IMU correction ─────────────────────────────────────────────
#define IMU_CORRECT_ENABLED  false
#define IMU_CORRECT_THRESH   2.5f

// ── Button ─────────────────────────────────────────────────────
#define BTN_PIN     0       // IO0 — BOOT button on most ESP32 devboards
#define SCREEN_COUNT 5

// ── Satellite queue ────────────────────────────────────────────
#define MAX_QUEUED_SATS 8

// ── Pass schedule ──────────────────────────────────────────────
#define MAX_SCHEDULED_PASSES 48

// ── Auto-park ──────────────────────────────────────────────────
#define PARK_IDLE_SEC   300   // park after 5 min idle with no pass
#define PARK_AZ         0.0f
#define PARK_EL         0.0f

// ── Weather ────────────────────────────────────────────────────
#define WEATHER_INTERVAL_MS  600000UL  // fetch every 10 min

// ── Radar safe zone ────────────────────────────────────────────
#define RADAR_SAFE_TOP   164
#define RADAR_SAFE_BOT   293
#define RADAR_SAFE_LEFT    0
#define RADAR_SAFE_RIGHT 239

// ================= PINS & COLORS =================
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4
#define AZ_STEP   32
#define AZ_DIR    14
#define EL_STEP   27
#define EL_DIR    26
#define ENABLE_PIN 25

// Palette: PURPLE primary (chrome/titles/frames), GREEN accent
// (values/OK/radar/sat dot), near-black background. (B)
// Safe RGB565 macro — no hand-computed bit math (avoids off-color bugs).
#define RGB565(r,g,b) ((uint16_t)((((uint16_t)(r)&0xF8)<<8)|(((uint16_t)(g)&0xFC)<<3)|(((uint16_t)(b))>>3)))

#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_PRIMARY RGB565(190,160,225)   // light purple — titles, frames, target
#define C_ACCENT  RGB565(130,232,156)   // light green — values, OK, radar, sat dot
#define C_NEON    C_ACCENT              // alias kept: green = accent
#define C_GREEN   RGB565( 60,220,120)
#define C_LIME    RGB565(170,240,180)
#define C_AMBER   C_PRIMARY             // alias kept: amber→purple (TGT/EL/titles)
#define C_ORANGE  RGB565(255,178,140)   // soft warning (countdown)
#define C_RED     RGB565(255,112,130)
#define C_CYAN    C_PRIMARY             // footprint screen chrome → purple
#define C_MGRAY   RGB565(125,120,142)   // muted text (slightly purple-tinted)
#define C_DGRAY   RGB565( 56,50,78)     // frame/border (purple-tinted dark)
#define C_FAINT   RGB565( 36,30,56)
#define C_HDRBG   RGB565( 28,20,44)     // dark purple header bar
#define C_PNLG    RGB565( 24,44,30)     // dark green panel-title fill
#define C_PNLA    RGB565( 32,24,52)     // dark purple panel-title fill
#define C_RING    RGB565( 72,60,104)    // radar ring (purple-tint)
#define C_DRED    RGB565( 80,20,40)

#endif // CONFIG_H