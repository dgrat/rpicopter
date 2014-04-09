#ifndef OUT_h
#define OUT_h

#include "global.h"
#include "containers.h"
#include "scheduler.h"


void send_comp();
void send_atti();
void send_baro();
void send_gps();
void send_bat();
void send_rc();
void send_pids_attitude();
void send_pids_altitude();

// function, delay, multiplier of the delay
Task outAtti   (&send_atti,          3,   1);
Task outRC     (&send_rc,            37,  1);
Task outComp   (&send_comp,          44,  1);
Task outBaro   (&send_baro,          66,  1);
Task outGPS    (&send_gps,           66,  2);
Task outBat    (&send_bat,           75,  1);
Task outPIDAtt (&send_pids_attitude, 133, 1);
Task outPIDAlt (&send_pids_altitude, 133, 2);

///////////////////////////////////////////////////////////
// LED OUT
///////////////////////////////////////////////////////////
void flash_leds(bool on) {
  hal.gpio->write(A_LED_PIN, on ? LED_OFF : LED_ON);
  hal.gpio->write(C_LED_PIN, on ? LED_ON : LED_OFF);
}

void leds_off() {
  hal.gpio->write(A_LED_PIN, LED_OFF);
  hal.gpio->write(C_LED_PIN, LED_OFF);
}

void leds_on() {
  hal.gpio->write(A_LED_PIN, LED_ON);
  hal.gpio->write(C_LED_PIN, LED_ON);
}
///////////////////////////////////////////////////////////
// compass
///////////////////////////////////////////////////////////
void send_comp() {
  if(!_HAL_BOARD.m_pComp->healthy() ) {
    return;
  }

  hal.console->printf("{\"type\":\"s_cmp\",\"h\":%.1f}\n",
  static_cast<double>(_HAL_BOARD.read_comp_deg() ) );
}
///////////////////////////////////////////////////////////
// attitude in degrees
///////////////////////////////////////////////////////////
void send_atti() {
  hal.console->printf("{\"type\":\"s_att\",\"r\":%.1f,\"p\":%.1f,\"y\":%.1f}\n",
  static_cast<double>(_HAL_BOARD.get_atti_raw_deg().y), 
  static_cast<double>(_HAL_BOARD.get_atti_raw_deg().x), 
  static_cast<double>(_HAL_BOARD.get_atti_raw_deg().z) );
}
///////////////////////////////////////////////////////////
// barometer
///////////////////////////////////////////////////////////
void send_baro() {
  if(!_HAL_BOARD.m_pBaro->healthy) {
    return;
  }

  BaroData baro = _HAL_BOARD.read_baro();
  hal.console->printf("{\"type\":\"s_bar\",\"p\":%.1f,\"a\":%ld,\"t\":%.1f,\"c\":%.1f,\"s\":%d}\n",
  static_cast<double>(baro.pressure_pa), 
  baro.altitude_cm, 
  static_cast<double>(baro.temperature_deg), 
  static_cast<double>(baro.climb_rate_cms), 
  static_cast<uint_fast16_t>(baro.pressure_samples) );
}
///////////////////////////////////////////////////////////
// gps
///////////////////////////////////////////////////////////
void send_gps() {
  if(!(*_HAL_BOARD.m_pGPS)->fix) {
    return;
  }

  GPSData gps = _HAL_BOARD.get_gps();
  hal.console->printf("{\"type\":\"s_gps\",\"lat\":%.1f,\"lon\":%.1f,\"a_cm\":%ld,\"g_cms\":%.1f,\"e_cms\":%.1f,\"n_cms\":%.1f,\"d_cms\":%.1f,\"h_x\":%.1f,\"h_y\":%.1f,\"h_z\":%.1f,\"g_cd\":%ld,\"sat\":%d,\"tw\":%d,\"tw_s\":%ld}\n",
                      (static_cast<double>(gps.latitude) )  / 10000000.f,
                      (static_cast<double>(gps.longitude) ) / 10000000.f,
                      gps.altitude_cm,

                      static_cast<double>(gps.gspeed_cms),
                      static_cast<double>(gps.espeed_cms),
                      static_cast<double>(gps.nspeed_cms),
                      static_cast<double>(gps.dspeed_cms),

                      static_cast<double>(gps.heading_x),
                      static_cast<double>(gps.heading_y),
                      static_cast<double>(gps.heading_z),

                      gps.gcourse_cd,
                      gps.satelites,
                      gps.time_week,
                      gps.time_week_s);
}
///////////////////////////////////////////////////////////
// battery monitor
///////////////////////////////////////////////////////////
void send_bat() {
  BattData bat = _HAL_BOARD.read_bat();
  hal.console->printf("{\"type\":\"s_bat\",\"V\":%.1f,\"A\":%.1f,\"c_mAh\":%.1f}\n",
                      static_cast<double>(bat.voltage_V), 
                      static_cast<double>(bat.current_A), 
                      static_cast<double>(bat.consumpt_mAh) );
}
///////////////////////////////////////////////////////////
// remote control
///////////////////////////////////////////////////////////
void send_rc() {
  int_fast16_t rcthr = _RECVR.m_rgChannelsRC[RC_THR];
  int_fast16_t rcyaw = _RECVR.m_rgChannelsRC[RC_YAW];
  int_fast16_t rcpit = _RECVR.m_rgChannelsRC[RC_PIT];
  int_fast16_t rcrol = _RECVR.m_rgChannelsRC[RC_ROL];

  hal.console->printf("{\"type\":\"rc_in\",\"r\":%d,\"p\":%d,\"t\":%d,\"y\":%d}\n",
                      rcrol, rcpit, rcthr, rcyaw);
}
///////////////////////////////////////////////////////////
// PID configuration
///////////////////////////////////////////////////////////
void send_pids_attitude() {
  // Capture values
  float pit_rkp   = _HAL_BOARD.m_rgPIDS[PID_PIT_RATE].kP();
  float pit_rki   = _HAL_BOARD.m_rgPIDS[PID_PIT_RATE].kI();
  float pit_rimax = _HAL_BOARD.m_rgPIDS[PID_PIT_RATE].imax();

  float rol_rkp   = _HAL_BOARD.m_rgPIDS[PID_ROL_RATE].kP();
  float rol_rki   = _HAL_BOARD.m_rgPIDS[PID_ROL_RATE].kI();
  float rol_rimax = _HAL_BOARD.m_rgPIDS[PID_ROL_RATE].imax();

  float yaw_rkp   = _HAL_BOARD.m_rgPIDS[PID_YAW_RATE].kP();
  float yaw_rki   = _HAL_BOARD.m_rgPIDS[PID_YAW_RATE].kI();
  float yaw_rimax = _HAL_BOARD.m_rgPIDS[PID_YAW_RATE].imax();

  float pit_skp   = _HAL_BOARD.m_rgPIDS[PID_PIT_STAB].kP();
  float rol_skp   = _HAL_BOARD.m_rgPIDS[PID_ROL_STAB].kP();
  float yaw_skp   = _HAL_BOARD.m_rgPIDS[PID_YAW_STAB].kP();

  hal.console->printf("{\"type\":\"pid_cnf\","
                      "\"pit_rkp\":%.2f,\"pit_rki\":%.2f,\"pit_rimax\":%.2f,"
                      "\"rol_rkp\":%.2f,\"rol_rki\":%.2f,\"rol_rimax\":%.2f,"
                      "\"yaw_rkp\":%.2f,\"yaw_rki\":%.2f,\"yaw_rimax\":%.2f,"
                      "\"pit_skp\":%.2f,\"rol_skp\":%.2f,\"yaw_skp\":%.2f}\n",
                      static_cast<double>(pit_rkp), static_cast<double>(pit_rki), static_cast<double>(pit_rimax),
                      static_cast<double>(rol_rkp), static_cast<double>(rol_rki), static_cast<double>(rol_rimax),
                      static_cast<double>(yaw_rkp), static_cast<double>(yaw_rki), static_cast<double>(yaw_rimax),
                      static_cast<double>(pit_skp), static_cast<double>(rol_skp), static_cast<double>(yaw_skp) );
}

void send_pids_altitude() {
  // Capture values
  float thr_rkp   = _HAL_BOARD.m_rgPIDS[PID_THR_RATE].kP();
  float thr_rki   = _HAL_BOARD.m_rgPIDS[PID_THR_RATE].kI();
  float thr_rimax = _HAL_BOARD.m_rgPIDS[PID_THR_RATE].imax();

  float acc_rkp   = _HAL_BOARD.m_rgPIDS[PID_ACC_RATE].kP();
  float acc_rki   = _HAL_BOARD.m_rgPIDS[PID_ACC_RATE].kI();
  float acc_rimax = _HAL_BOARD.m_rgPIDS[PID_ACC_RATE].imax();

  float thr_skp   = _HAL_BOARD.m_rgPIDS[PID_THR_STAB].kP();
  float acc_skp   = _HAL_BOARD.m_rgPIDS[PID_ACC_STAB].kP();

  hal.console->printf("{\"type\":\"pid_cnf\","
                      "\"thr_rkp\":%.2f,\"thr_rki\":%.2f,\"thr_rimax\":%.2f,"
                      "\"acc_rkp\":%.2f,\"acc_rki\":%.2f,\"acc_rimax\":%.2f,"
                      "\"thr_skp\":%.2f,\"acc_skp\":%.2f}\n",
                      static_cast<double>(thr_rkp), static_cast<double>(thr_rki), static_cast<double>(thr_rimax),
                      static_cast<double>(acc_rkp), static_cast<double>(acc_rki), static_cast<double>(acc_rimax),
                      static_cast<double>(thr_skp), static_cast<double>(acc_skp) );
}

#endif

