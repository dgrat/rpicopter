#ifndef COMMON_h
#define COMMON_h

#include "config.h"


inline double progress_f(uint_fast8_t iStep, uint_fast8_t iMax) {
  return (double)iStep*100.f/(double)iMax;
}

inline int add_flag(int flag, int mask) {
  flag |= mask;
  return flag;
}
  
inline int rem_flag(int flag, int mask) {
  flag &= ~mask;
  return flag;
}

inline bool chk_fset(int flag, int mask) {
  return flag & mask;
}

// Returns the sign of a float
inline float sign_f(float fVal) {
  return fVal >= 0.f ? 1.f : -1.f;
}

inline float pow2_f(float fVal) {
  return fVal * fVal;
}

/*
 * Return: Length of a vector "vec"
 */
inline float vecl3f_f(const Vector3f &vec) {
  return sqrt(pow2_f(vec.x) + pow2_f(vec.y) + pow2_f(vec.z) );
}

inline float wrap180_f(float x) {
  return x < -180.f ? (x + 360.f) : (x > 180.f ? (x - 360.f) : x);
}

inline float wrap360_f(float x) {
  return x < 0.f ? (x + 360.f) : (x > 360.f ? (x - 360.f) : x);
}
/*
 * This function changes the function parameter
 * Used this for performance reasons
 */
inline Vector3f wrap180_V3f(Vector3f &vec) {
  vec.x = wrap180_f(vec.x);
  vec.y = wrap180_f(vec.y);
  vec.z = wrap180_f(vec.z);
  return vec;
}

/*
 * This function changes the function parameter
 * Used this for performance reasons
 */
inline Vector3f wrap360_V3f(Vector3f &vec) {
  vec.x = wrap360_f(vec.x);
  vec.y = wrap360_f(vec.y);
  vec.z = wrap360_f(vec.z);
  return vec;
}

inline float smaller_f(float value, float bias) {
  return value < bias ? value : bias;
}

inline float bigger_f(float value, float bias) {
  return value > bias ? value : bias;
}

inline float delta_f(float fCurVal_Deg, float fOldVal_Deg) {
  float fVal = fCurVal_Deg - fOldVal_Deg;
  return fVal < -180.f ? ((fCurVal_Deg + 360) - fOldVal_Deg) : fVal > 180.f ? ((fCurVal_Deg - 360) - fOldVal_Deg) : fVal;
}

/*
 * Sigmoid transfer function
 * mod: Determines the slope (how fast the function decays when the angular values increase)
 * mod: Higher means faster decay
 */
inline float sigm_atti_f(float x, float mod){
  float val = (180.f - smaller_f(abs(mod * x), 179.9f) ) / 180.f;
  return val / sqrt(1 + pow2_f(val) );
}

inline float sigm_climb_f(float x, float mod){
  float val = smaller_f(abs(mod * x), 125.f) / 250.f;
  return val / sqrt(1 + pow2_f(val) );
}

/*
 * Fuses two sensor values together by annealing angle_fuse to angle_ref
 * mod: Determines the slope (decay) of the sigmoid activation function
 * rate: Determines how fast the annealing takes place
 */
inline int_fast32_t 
anneal_l( int_fast32_t &val2fuse, const int_fast32_t &valconst, 
          int_fast32_t time_ms, 
          int_fast32_t mod, 
          int_fast32_t rate, 
          float (*functor)(float, float) ) 
{
  int_fast32_t fR = rate * time_ms;
  int_fast32_t iFunctor = (int_fast32_t)(1.f / functor((float)valconst, (float)mod) );
  val2fuse += (valconst - val2fuse) * fR / iFunctor / 1000;

  return val2fuse;
}
 
inline float 
anneal_f( float &val2fuse, const float &valconst, 
          float time_s, 
          float mod, 
          float rate, 
          float (*functor)(float, float) )
{
  float fR = rate * time_s;
  val2fuse += (valconst - val2fuse) * fR * functor(valconst, mod);
  return val2fuse;
}

inline Vector3f 
anneal_V3f( Vector3f &val2fuse, const Vector3f &valconst, 
            float time_s, 
            float mod, 
            float rate, 
            float (*functor)(float, float) ) 
{
  anneal_f(val2fuse.x, valconst.x, time_s, mod, rate, functor);
  anneal_f(val2fuse.y, valconst.y, time_s, mod, rate, functor);
  anneal_f(val2fuse.z, valconst.z, time_s, mod, rate, functor);

  return val2fuse;
}

/*
 * Low pass filter
 */
inline int_fast32_t 
low_pass_filter_l(const int_fast32_t fCurSmple, const int_fast32_t fOldSmple, const int_fast16_t p) 
{
  const int_fast16_t q = 100 - p;
  return (fCurSmple * p + fOldSmple * q) / 100;
}

inline float 
low_pass_filter_f(const float fCurSmple, const float fOldSmple, const float p) 
{
  return fCurSmple * p + (fOldSmple * (1.f - p) );
}

inline Vector3f 
low_pass_filter_V3f(const Vector3f &fCurSmple, const Vector3f &fOldSmple, const float p) 
{
  return fCurSmple * p + (fOldSmple * (1.f - p) );
}

#endif
