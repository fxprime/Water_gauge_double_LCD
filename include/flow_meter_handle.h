
#include "Arduino.h" 
const       byte      flowsensorIntPin  = 2;
const       time_us_t DEBOUNCE_TIME     = 1500;
volatile    time_us_t _last_fall        = 0; 
 
/* -------------------------------------------------------------------------- */
/*                       Count the pulse using interrupt                      */
/* -------------------------------------------------------------------------- */

void interruptFunc()
{
  time_us_t cur_time = micros();
  if(cur_time-_last_fall>DEBOUNCE_TIME) {
    _last_fall = cur_time;
    pulseCount++;
  }
  // 
  


}


/* -------------------------------------------------------------------------- */
/*                          Initialize flow input pin                         */
/* -------------------------------------------------------------------------- */

void Flowmeter_init()
{
  Serial.print("Flowmeter initing..\n");

  pinMode(flowsensorIntPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowsensorIntPin), interruptFunc, RISING);
  Serial.print("Flowmeter inited\n");
}

/* -------------------------------------------------------------------------- */
/*                            Get current liter now                           */
/* -------------------------------------------------------------------------- */

static inline void Flowmeter_update() {
  _pulse_now    +=  pulseCount;
  pulseCount    =   0;
} 

/* -------------------------------------------------------------------------- */
/*                        Function to refresh flowmeter                       */
/* -------------------------------------------------------------------------- */

static inline void Flowmeter_reset() {
  // Serial.println("Reset Flowmeter.");
  _pulse_trip   +=  _pulse_now;
  pulseCount    =   0;
  _pulse_now    =   0; 
}

static inline void Flowmeter_keep_reset() {
  time_ms_t         cur_time    = millis();
  static time_ms_t  last_reset  = cur_time;
  if( cur_time - last_reset > 200 ) {
    last_reset = cur_time;
    Flowmeter_reset();
  }
}