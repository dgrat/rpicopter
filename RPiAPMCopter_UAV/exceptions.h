#ifndef NAVIG_h
#define NAVIG_h

#include "containers.h"
#include "config.h"

class Device;
class Receiver;

class Exception {
private:
  Device*       m_pHalBoard;
  Receiver*     m_pReceiver;
  
  int_fast16_t  m_rgChannelsRC[APM_IOCHAN_CNT];                // Array:  Override remote control if certain exceptions happen
  uint_fast32_t m_t32Device;                                   // Timer for calculating the reduction of the throttle
  uint_fast32_t m_t32Altitude;                                 // Timer for reading the current altitude
  
  /*
   * This functions measures the height from GPS and barometer data.
   * If no sensor is working the parameter is set to false.
   * Otherwise the parameter is true.
   */
  float estim_altit(bool &);
  /*
   * Saves the current remote control command one time.
   * rls_recvr() must be called before, this function is usable again.
   * The parameter is a switch which will set to true if it was false and then the function executes. 
   * If the parameter is already true, the function will do nothing.
   */
  void lck_recvr(bool &);
  /*
   * Removes the lock for saving the remote control command.
   * The parameter is a switch which will set to false if it was true and then the function executes. 
   * If the parameter is already false, the function will do nothing.
   */
  void rls_recvr(bool &);
  /*
   * This reduces the throttle. 
   * The amount of reduction dependent on the speed and the height.
   */
  void reduce_thr(float fTime);
  /*
   * !Critical: Section!
   * This function only stops if the motors of the model stopped spinning!
   * There is no possibility to get control back before. 
   * Only use this function, if there are severe hardware problems.
   */
  void dev_take_down();   // Calls reduce_thr()
  /*
   * !Critical: Section!
   * This function stops if the receiver fetches a signal
   * So this function has less severe consequences if the remote control works again
   */
  void rcvr_take_down();  // Calls reduce_thr()
  
public:
  Exception(Device *, Receiver *);
  bool handle();                                              // Check all defines exceptions and call error handlers
};

#endif /*UAV_h*/
