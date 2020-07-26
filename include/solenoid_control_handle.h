#include <Arduino.h>


const uint8_t relay_pin = 4; 

enum {
    TURN_OFF = 0,
    TURN_ON
};

/* -------------------------------------------------------------------------- */
/*                      Relay for solenoidpin initialize                      */
/* -------------------------------------------------------------------------- */

void Solenoid_init()
{
  Serial.print("Solenoid initing..\n");
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);
  Serial.print("Solenoid inited\n");
}

/* -------------------------------------------------------------------------- */
/*                           Set the Solenoid state                           */
/* -------------------------------------------------------------------------- */

static inline void Solenoid_set(uint8_t state) {
    static uint8_t last_state = 2;
    digitalWrite(relay_pin, state);

    if(last_state != state) {
        last_state = state;
        if(state)   Serial.println("Solenoid on");
        else        Serial.println("Solenoid off");
    }
    
}

void EMI_shield_init() {
    const int pinnum = sizeof(_pin_unused)/sizeof(int);
    for(int i =0;i< pinnum; i++) {
        pinMode(_pin_unused[i], OUTPUT);
        digitalWrite(_pin_unused[i], HIGH);
    }
}