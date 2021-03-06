#include <AP_InertialSensor.h> // for user interactant
#include <AP_AHRS.h>
#include <RC_Channel.h>     // RC Channel Library

#include "receiver.h"
#include "device.h"
#include "BattMonitor.h"
#include "arithmetics.h"


Receiver::Receiver(Device *pHalBoard) {
  m_pHalBoard = pHalBoard;

  memset(m_cBuffer, 0, sizeof(m_cBuffer) );
  memset(m_rgChannelsRC, 0, sizeof(m_rgChannelsRC) );

  m_iPPMTimer = m_iSParseTimer_A = m_iSParseTimer_C = m_iSParseTimer = m_pHalBoard->m_pHAL->scheduler->millis();
  m_iPPMTime = m_iSParseTime_A = m_iSParseTime_C  = m_iSParseTime  = 0;

  m_eErrors = NOTHING_F;
  init_radio();
}

void Receiver::init_radio() {
  m_pRCRol = new RC_Channel(RC_ROL);
  m_pRCPit = new RC_Channel(RC_PIT);
  m_pRCThr = new RC_Channel(RC_THR);
  m_pRCYaw = new RC_Channel(RC_YAW);
  
  // setup radio
  if (m_pRCThr->radio_min == 0) {
    // cope with AP_Param not being loaded
    m_pRCThr->radio_min = RC_THR_OFF;
  }
  if (m_pRCThr->radio_max == 0) {
    // cope with AP_Param not being loaded
    m_pRCThr->radio_max = RC_THR_MAX;
  }
  
  // set rc channel ranges
  m_pRCRol->set_angle(RC_ROL_MAX*100);
  m_pRCPit->set_angle(RC_PIT_MAX*100);
  m_pRCThr->set_range(RC_THR_ACRO-RC_THR_OFF, RC_THR_MAX-RC_THR_OFF);
  m_pRCYaw->set_angle(RC_YAW_MAX*100);
}

uint_fast8_t Receiver::calc_chksum(char *str) {
  uint_fast8_t nc = 0;
  for(size_t i = 0; i < strlen(str); i++) {
    nc = (nc + str[i]) << 1;
  }
  return nc;
}

//checksum verifier
bool Receiver::verf_chksum(char *str, char *chk) {
  uint_fast8_t  nc  = calc_chksum(str);
  long chkl = strtol(chk, NULL, 16);  // supplied chksum to long
  if(chkl == static_cast<long>(nc) ) {              // compare
    return true;
  }
  return false;
}

// remote control stuff
bool Receiver::parse_ctrl_com(char* buffer) {
  if(m_rgChannelsRC == NULL) {
    return false;
  }

  char *str = strtok(buffer, "*");                      // str = roll, pit, thr, yaw
  char *chk = strtok(NULL, "*");                        // chk = chksum

  if(verf_chksum(str, chk) ) {                          // if chksum OK
    char *ch = strtok(str, ",");                        // first channel
    m_rgChannelsRC[0] = strtol(ch, NULL, 10);           // parse
    for(uint_fast8_t i = 1; i < APM_IOCHAN_CNT; i++) {  // loop through final 3 RC_CHANNELS
      char *ch = strtok(NULL, ",");
      m_rgChannelsRC[i] = strtol(ch, NULL, 10);
    }
    m_iSParseTimer = m_pHalBoard->m_pHAL->scheduler->millis(); // update last valid packet
  }
  return true;
}

// drift compensation
// maximum value is between -10 and 10 degrees
bool Receiver::parse_gyr_cor(char* buffer) {
  char *str = strtok(buffer, "*");                  // str = roll, pit, thr, yaw
  char *chk = strtok(NULL, "*");                    // chk = chksum
  bool bRet = false;

  if(verf_chksum(str, chk) ) {                      // if chksum OK
    char *cstr;

    for(uint_fast8_t i = 0; i < COMP_ARGS; i++) {   // loop through final 3 RC_CHANNELS
      if(i == 0)
        cstr = strtok (buffer, ",");
      else cstr = strtok (NULL, ",");

      switch(i) {
        case 0:
          m_pHalBoard->set_rol_cor(atof(cstr) );
          m_pHalBoard->set_rol_cor(m_pHalBoard->get_rol_cor()  > 10.f ? 10.f : m_pHalBoard->get_rol_cor() < -10.f ? -10.f : m_pHalBoard->get_rol_cor() );
          break;
        case 1:
          m_pHalBoard->set_pit_cor(atof(cstr) );
          m_pHalBoard->set_pit_cor(m_pHalBoard->get_pit_cor()  > 10.f ? 10.f : m_pHalBoard->get_pit_cor() < -10.f ? -10.f : m_pHalBoard->get_pit_cor() );
          bRet = true;
          break;
      }
    }
  }
  return bRet;
}

bool Receiver::parse_waypoint(char *buffer) {
  char *str = strtok(buffer, "*");                  // str = roll, pit, thr, yaw
  char *chk = strtok(NULL, "*");                    // chk = chksum
  bool bRet = false;

  int_fast32_t lat           = 0;
  int_fast32_t lon           = 0;
  int_fast32_t alt_cm        = 0;
  GPSPosition::UAV_TYPE flag = GPSPosition::NOTHING_F;

  if(verf_chksum(str, chk) ) {                      // if chksum OK
    char *cstr;

    for(uint_fast8_t i = 0; i < GPSP_ARGS; i++) {   // loop through final 3 RC_CHANNELS
      if(i == 0) {
        cstr = strtok (buffer, ",");
      } else {
        cstr = strtok (NULL, ",");
      }

      switch(i) {
        case 0:
          lat    = atol(cstr);
          break;
        case 1:
          lon    = atol(cstr);
          break;
        case 2:
          alt_cm = atol(cstr);
          break;
        case 3:
          // Parse the type flag
          flag = static_cast<GPSPosition::UAV_TYPE>(atoi(cstr) );
          // Override the height if the flag is HLD_ALTITUDE_F
          if(flag == GPSPosition::HLD_ALTITUDE_F) {
            bool bOK = false;
            // Measure the current height
            alt_cm = Device::get_altitude_cm(m_pHalBoard, bOK);
            // If height measurement failed, then break it
            if(!bOK) {
              flag = GPSPosition::NOTHING_F;
            }
          }
          // Indicate everything went well
          bRet = true;
          break;
      }
    }
  }
  // Set new waypoint only if the everything worked out like expected
  if(bRet == true) {
    m_Waypoint = GPSPosition(lat, lon, alt_cm, flag);
  }
  return bRet;
}

void Receiver::run_calibration() {
  float roll_trim, pitch_trim;
  while(m_pHalBoard->m_pHAL->console->available() ) {
    m_pHalBoard->m_pHAL->console->read();
  }

#if !defined( __AVR_ATmega1280__ )
  AP_InertialSensor_UserInteractStream interact(m_pHalBoard->m_pHAL->console);
  m_pHalBoard->m_pInert->calibrate_accel(&interact, roll_trim, pitch_trim);
  // Adjust AHRS
  m_pHalBoard->m_pAHRS->set_trim(Vector3f(roll_trim, pitch_trim, 0) );
#else
	m_pHalBoard->m_pHAL->console->println_P(PSTR("calibrate_accel not available on 1280") );
#endif
}

bool Receiver::parse_gyr_cal(char* buffer) {
  // If motors run: Do nothing!
  if(m_rgChannelsRC == NULL || m_pHalBoard == NULL) {
    return false;
  } else if (m_rgChannelsRC[2] > RC_THR_MIN) {
    return false;
  }
  // process cmd
  char *str = strtok(buffer, "*");                  // str = roll, pit, thr, yaw
  char *chk = strtok(NULL, "*");                    // chk = chksum
  
  if(verf_chksum(str, chk) ) {
    // process cmd
    bool bcalib = (bool)atoi(str);
    // only if quadro is _not_ armed
    if(bcalib) {
      // This functions checks whether model is ready for a calibration
      run_calibration();
    }
  }

  return true;
}

/*
 * Changes the sensor type used for the battery monitor
 */
bool Receiver::parse_bat_type(char* buffer) {
  if(m_pHalBoard == NULL) {
    return false;
  }
  // process cmd
  char *str = strtok(buffer, "*");                // str
  char *chk = strtok(NULL, "*");                  // chk = chksum

  if(verf_chksum(str, chk) ) {                    // if chksum OK
    int type = atoi(str);
    m_pHalBoard->m_pBat->setup_source(type);
  }
  return true;
}

float *Receiver::parse_pid_substr(char* buffer) {
  static float rgfPIDS[PID_BUFFER_S] = { 0 };
  char rgcPIDS[PID_BUFFER_S][32];
  memset(rgfPIDS, 0, sizeof(rgfPIDS) );
  memset(rgcPIDS, 0, sizeof(rgcPIDS) );

  size_t i = 0, iPIDcstr = 0, iPID = 0;
  for(; i < strlen(buffer); i++) {
    // String ended here
    if(buffer[i] == '\0') {
      break;
    }
    // Avoid buffer overflow
    else if(iPID >= PID_BUFFER_S) {
      break;
    }
    // Reached new variable; Go over to next char
    else if(buffer[i] == ',') {
      iPID++;
      iPIDcstr = 0;
      continue;
    }
    // Read the current variable
    else {
      rgcPIDS[iPID][iPIDcstr]   = buffer[i];
      rgcPIDS[iPID][++iPIDcstr] = '\0';
    }
  }
  for (size_t i = 0; i <= iPID; i++) {
    rgfPIDS[i] = atof(rgcPIDS[i]);
  }
  return rgfPIDS;
}

bool Receiver::parse_pid_conf(char* buffer) {
  if(m_pHalBoard == NULL) {
    return false;
  }
  else if(m_rgChannelsRC[2] > RC_THR_MIN) {        // If motors run: Do nothing!
    return false;
  }

  // process cmd
  bool bRet = false;
  char *str = strtok(buffer, "*");                  // str = roll, pit, thr, yaw
  char *chk = strtok(NULL, "*");                    // chk = chksum

  if(verf_chksum(str, chk) ) {                      // if chksum OK
    char *cstr;

    for(uint_fast8_t i = 0; i < PID_ARGS; i++) {
      if(i == 0)
        cstr = strtok (buffer, ";");
      else cstr = strtok (NULL, ";");

      float *pids = parse_pid_substr(cstr);
      switch(i) {
      case 0:
        m_pHalBoard->m_rgPIDS[PID_PIT_RATE].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_PIT_RATE].kI(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_PIT_RATE].kD(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_PIT_RATE].imax(pids[3]);
        break;
      case 1:
        m_pHalBoard->m_rgPIDS[PID_ROL_RATE].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_ROL_RATE].kI(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_ROL_RATE].kD(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_ROL_RATE].imax(pids[3]);
        break;
      case 2:
        m_pHalBoard->m_rgPIDS[PID_YAW_RATE].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_YAW_RATE].kI(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_YAW_RATE].kD(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_YAW_RATE].imax(pids[3]);
        break;
      case 3:
        m_pHalBoard->m_rgPIDS[PID_THR_RATE].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_THR_RATE].kI(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_THR_RATE].kD(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_THR_RATE].imax(pids[3]);
        break;
      case 4:
        m_pHalBoard->m_rgPIDS[PID_ACC_RATE].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_ACC_RATE].kI(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_ACC_RATE].kD(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_ACC_RATE].imax(pids[3]);
        break;
      case 5:
        m_pHalBoard->m_rgPIDS[PID_PIT_STAB].kP(pids[0]);
        m_pHalBoard->m_rgPIDS[PID_ROL_STAB].kP(pids[1]);
        m_pHalBoard->m_rgPIDS[PID_YAW_STAB].kP(pids[2]);
        m_pHalBoard->m_rgPIDS[PID_THR_STAB].kP(pids[3]);
        m_pHalBoard->m_rgPIDS[PID_ACC_STAB].kP(pids[4]);
        bRet = true;
        break;
      }
    }
  }
  return bRet;
}

bool Receiver::check_input(int_fast16_t iRol, int_fast16_t iPit, int_fast16_t iThr, int_fast16_t iYaw) {
  if(!in_range(RC_PIT_MIN, RC_PIT_MAX, iPit) ) {
    return false;
  }
  if(!in_range(RC_ROL_MIN, RC_ROL_MAX, iRol) ) {
    return false;
  }
  if(!in_range(RC_THR_OFF, RC_THR_MAX, iThr) ) {
    return false;
  }
  if(!in_range(RC_YAW_MIN, RC_YAW_MAX, iYaw) ) {
    return false;
  }
  return true;
}

/*
 * Compact remote control packet system for the radio on Uart2,
 * Everything fits into 7 bytes
 */
bool Receiver::parse_radio(char *buffer) {
  int_fast16_t thr = 1000 + (static_cast<int_fast16_t>(buffer[0]) * 100) + (int_fast16_t)buffer[1]; // 1000 - 1900
  int_fast16_t pit = static_cast<int_fast16_t>(buffer[2]) - 127;                               // -45° - 45°
  int_fast16_t rol = static_cast<int_fast16_t>(buffer[3]) - 127;                               // -45° - 45°
  int_fast16_t yaw = static_cast<int_fast16_t>(buffer[5]) * (static_cast<int_fast16_t>(buffer[4]) - 127);        // -180° - 180°
  uint_fast8_t chk = static_cast<uint_fast8_t>(buffer[6]);                                     // checksum

  // Calculate checksum
  uint_fast8_t checksum = 0;
  for(uint_fast8_t i = 0; i < 6; i++) {
    checksum = (checksum + buffer[i]) << 1;
  }

  // Validity check:
  // First checksum
  if(checksum != chk) {
    return false;
  }
  // Small validity check
  if(!check_input(rol, pit, thr, yaw) ) {
    return false;
  }
    
  // Set values
  m_rgChannelsRC[RC_ROL] = rol;
  m_rgChannelsRC[RC_PIT] = pit;
  m_rgChannelsRC[RC_THR] = thr;
  m_rgChannelsRC[RC_YAW] = yaw;

  m_iSParseTimer = m_pHalBoard->m_pHAL->scheduler->millis();           // update last valid packet
  return true;
}

bool Receiver::read_uartA(uint_fast16_t bytesAvail) {
  static uint_fast16_t offset = 0;

  bool bRet = false;
  for(; bytesAvail > 0; bytesAvail--) {
    char c = static_cast<char>(m_pHalBoard->m_pHAL->console->read() );// read next byte
    if(c == '\n' /*|| c == 'z'*/) {                     // new line or special termination signature (z): Process cmd
      m_cBuffer[offset] = '\0';                         // null terminator
      bRet = parse(m_cBuffer);
      if(bRet) {
        m_iSParseTimer_A = m_iSParseTimer;
        m_pHalBoard->set_update_rate_ms(MAIN_T_MS);
      }
      memset(m_cBuffer, 0, sizeof(m_cBuffer) ); offset = 0;
    }
    else if(c != '\r' && offset < sizeof(m_cBuffer)-1) {
      m_cBuffer[offset++] = c;                          // store in buffer and continue until newline
    }
  }

  return bRet;
}

bool Receiver::read_uartC(uint_fast16_t bytesAvail) {
  static uint_fast16_t offset = 0;

  bool bRet = false;
  for(; bytesAvail > 0; bytesAvail--) {
    // command must be exactly 8 bytes long
    if(offset > RADIO_MAX_OFFS) {                             // if message is longer than it should be
      memset(m_cBuffer, 0, sizeof(m_cBuffer) ); offset = 0;   // reset everything
      return false;                                           // and break loop
    }
    char c = static_cast<char>(m_pHalBoard->m_pHAL->uartC->read() );        // read next byte
    if(c == static_cast<char>(254) ) {                                      // this control char is not used for any other symbol
      m_cBuffer[offset] = '\0';                               // null terminator at 8th position
      if(offset != RADIO_MAX_OFFS) {                          // theoretically a broken message can still be shorter than it should be
        memset(m_cBuffer, 0, sizeof(m_cBuffer) ); offset = 0; //so break here if something was wrong
        return false;
      }
      else {                                                  // message has perfect length and stop byte
        bRet = parse_radio(m_cBuffer);
        if(bRet) {
          m_iSParseTimer_C = m_iSParseTimer;
        }
        memset(m_cBuffer, 0, sizeof(m_cBuffer) ); offset = 0;
      }
    }
    else if(offset < sizeof(m_cBuffer)-1) {
      m_cBuffer[offset++] = c;                                // store in buffer and continue until newline
    }
  }

  return bRet;
}

// Parse incoming text
// str = "%d,%d,%d,%d * checksum" % (p['roll'], p['pitch'], p['thr'], p['yaw'])
// str = "%d,%d,%d; %d,%d,%d; %d,%d,%d * checksum"
bool Receiver::parse(char *buffer) {
  if(buffer == NULL) {
    return false;
  }

  char *ctype = strtok(buffer, "#");                // type of string
  char *command = strtok(NULL, "#");                // command string

  // process cmd
  if(strcmp(ctype, "RC") == 0) {
    return parse_ctrl_com(command);
  }
  if(strcmp(ctype, "PID") == 0) {
    return parse_pid_conf(command);
  }
  if(strcmp(ctype, "CMP") == 0) {
    return parse_gyr_cor(command);
  }
  if(strcmp(ctype, "GYR") == 0) {
    return parse_gyr_cal(command);
  }
  if(strcmp(ctype, "BAT") == 0) {
    return parse_bat_type(command);
  }
  if(strcmp(ctype, "UAV") == 0) {
    return parse_waypoint(command);
  }

  return false;
}

bool Receiver::try_any() {
  bool bOK = false;

// Try rcin (PPM radio)
#if USE_RCIN
  bOK = read_rcin();
  last_rcin_t32();
#endif

// Try WiFi over uartA
#if USE_UART_A
  bOK = read_uartA(m_pHalBoard->m_pHAL->uartA->available() );
  last_parse_uartA_t32();
#endif
  
#if USE_UART_C
  if(m_iSParseTime_A > UART_A_TIMEOUT) {
    // Reduce the loop frequency only if not in UAV mode
    // If currently in other modes, radio could be still helpful
    if(!chk_fset(m_Waypoint.mode, GPSPosition::GPS_NAVIGATN_F) && !BENCH_OUT) {
      m_pHalBoard->set_update_rate_ms(FALB_T_MS);
    }

    // Try uartC
    bOK = read_uartC(m_pHalBoard->m_pHAL->uartC->available() );
  }
  last_parse_uartC_t32();
#endif

  // Update the time for the last successful parse of a control string
  last_parse_t32();
  return bOK;
}

bool Receiver::read_rcin() {
  if(!m_pHalBoard->m_pHAL->rcin->new_input() ) {
    return false;
  }

  m_pRCPit->set_pwm(m_pHalBoard->m_pHAL->rcin->read(RC_PIT) );
  m_pRCRol->set_pwm(m_pHalBoard->m_pHAL->rcin->read(RC_ROL) );
  m_pRCThr->set_pwm(m_pHalBoard->m_pHAL->rcin->read(RC_THR) );
  m_pRCYaw->set_pwm(m_pHalBoard->m_pHAL->rcin->read(RC_YAW) );
  
  int_fast16_t pit = m_pRCPit->control_in / 100;
  int_fast16_t rol = m_pRCRol->control_in / 100;
  int_fast16_t thr = m_pRCThr->control_in;
  int_fast16_t yaw = m_pRCYaw->control_in / 100;
  
  // Small validity check
  if(!check_input(rol, pit, thr, yaw) ) {
    return false;
  }

  // If check was successful we feed the input into or rc array
  m_rgChannelsRC[RC_THR] = thr;
  // dezi degree to degree
  m_rgChannelsRC[RC_PIT] = pit;
  m_rgChannelsRC[RC_ROL] = rol;
  m_rgChannelsRC[RC_YAW] = yaw;
  
  // Update timers
  m_iSParseTimer = m_iPPMTimer = m_pHalBoard->m_pHAL->scheduler->millis();           // update last valid packet
  return true;
}

uint_fast32_t Receiver::last_parse_t32() {
  m_iSParseTime = m_pHalBoard->m_pHAL->scheduler->millis() - m_iSParseTimer;

  if(m_iSParseTime > COM_PKT_TIMEOUT) {
    m_eErrors = static_cast<DEVICE_ERROR_FLAGS>(add_flag(m_eErrors, UART_TIMEOUT_F) );
  }

  return m_iSParseTime;
}

uint_fast32_t Receiver::last_parse_uartA_t32() {
  m_iSParseTime_A = m_pHalBoard->m_pHAL->scheduler->millis() - m_iSParseTimer_A;
  return m_iSParseTime_A;
}

uint_fast32_t Receiver::last_parse_uartC_t32() {
  m_iSParseTime_C = m_pHalBoard->m_pHAL->scheduler->millis() - m_iSParseTimer_C;
  return m_iSParseTime_C;
}

uint_fast32_t Receiver::last_rcin_t32() {
  m_iPPMTime = m_pHalBoard->m_pHAL->scheduler->millis() - m_iPPMTimer;
  return m_iPPMTime;
}
