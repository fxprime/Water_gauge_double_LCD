#include <Bounce2.h>

#define NUM_BUTTONS 4
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {5, 7, 9, 11};
const uint8_t BUTTON_PINS_GND[NUM_BUTTONS] = {6, 8, 10, 12};


/* -------------------------------------------------------------------------- */
/*                            Button symbol design                            */
/* -------------------------------------------------------------------------- */

enum
{
  BT_PLUS = 0,
  BT_MINUS,
  BT_START,
  BT_CAL
};

enum
{
  BT_ST_PRESSING = 0,
  BT_ST_UNPRESSED = 1
};

Bounce *buttons = new Bounce[NUM_BUTTONS];


/* -------------------------------------------------------------------------- */
/*               Initialize Button input pin and make it pullup               */
/* -------------------------------------------------------------------------- */

void Button_init()
{
  Serial.print("Buttons initing..\n");
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    pinMode(BUTTON_PINS_GND[i], OUTPUT);
    digitalWrite(BUTTON_PINS_GND[i], LOW);
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP); //setup the bounce instance for the current button
    buttons[i].interval(25);                         // interval in ms
  }
  Serial.print("Buttons inited\n");
}


/* -------------------------------------------------------------------------- */
/*                       Keep update button time to time                      */
/* -------------------------------------------------------------------------- */

static inline void Button_update()
{
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    // Update the Bounce instance :
    buttons[i].update();
    // If it fell, flag the need to toggle the LED
    if (buttons[i].fell())
    {
      Serial.print(String(i) + " Click.\n");
    }
  }
}


static inline void Button_states() {

          time_ms_t cur_time      = millis(); 
  static  time_ms_t last_delay_ts = 0;


  switch(_state) {


    /* -------------------------------------------------------------------------- */
    /*                       State for initialize Something                       */
    /* -------------------------------------------------------------------------- */
    case ST_INITING: {
      Solenoid_set(TURN_OFF);
      State_set(ST_WAIT_SP);
      
      break;
    }


    /* -------------------------------------------------------------------------- */
    /*                        State for wait user set Liter                       */
    /* -------------------------------------------------------------------------- */
    case ST_WAIT_SP: {
      Solenoid_set(TURN_OFF);
      Flowmeter_keep_reset();
             
      static time_ms_t  last_add        = cur_time;
      static time_ms_t  last_unpress    = cur_time;
      bool              long_time_press = (cur_time - last_unpress > 1000);


      /* --------------------------- Prevent human prank -------------------------- */
      if(    buttons[BT_PLUS].read()  == BT_ST_PRESSING 
          && buttons[BT_MINUS].read() == BT_ST_PRESSING) 
          break;


      /* ---------------------------- Single press add ---------------------------- */
      if(buttons[BT_PLUS].rose() && !long_time_press ) {
        Liter_SP_ADD(); 
      }else if(buttons[BT_MINUS].rose() && !long_time_press ) {
        Liter_SP_MINUS(); 
      }


      /* ------------- Single press handle and time stamp when unpress ------------ */
      if(buttons[BT_PLUS].read() == BT_ST_PRESSING) {
        if(cur_time - last_add > 200 && long_time_press) {
          last_add = cur_time;
          Liter_SP_ADD();
        }
        
      }else if(buttons[BT_MINUS].read() == BT_ST_PRESSING) {
        if(cur_time - last_add > 200 && long_time_press) {
          last_add = cur_time;
          Liter_SP_MINUS();
        }
         
      }else if(buttons[BT_PLUS].read()  == BT_ST_UNPRESSED 
            && buttons[BT_MINUS].read() == BT_ST_UNPRESSED) {
        last_unpress      = cur_time;
      } 


      /* -- Go next if setpoint is set more than threshold and button start press - */
      if(buttons[BT_START].rose() && Liter_SP_is_enough()) {
        SaveParameters();
        State_set(ST_DELAY);
        last_delay_ts = cur_time;
      }


      /* ---------- If press cal button in ST_WAIT_SP let toggle cal flag --------- */
      if(buttons[BT_CAL].rose()) {
        Calibrate_set(!Calibrating());
      }
      break;
    }


    /* -------------------------------------------------------------------------- */
    /*                       State for wait a bit before run                      */
    /* -------------------------------------------------------------------------- */
    case ST_DELAY: {
      if( cur_time - last_delay_ts > DELAY_BEFORE_RUN) {
        State_set(ST_RUNNING); 
        Serial.println("Timeout");
      }
      break;
    }


    /* -------------------------------------------------------------------------- */
    /*                         State For running solenoid                         */
    /* -------------------------------------------------------------------------- */
    case ST_RUNNING: {
      Solenoid_set(TURN_ON);

      if(Water_is_full() && !Calibrating()) {
        State_set(ST_STOP);
      }else if(Water_is_full() && Calibrating()) {
        State_set(ST_CAL);
      }
      
      break;
    }

    /* -------------------------------------------------------------------------- */
    /*                       State For Stop if water is full                      */
    /* -------------------------------------------------------------------------- */
    case ST_STOP: {
      Solenoid_set(TURN_OFF);



      if(buttons[BT_START].rose()) {
        Flowmeter_reset();
        // Liter_SP_reset();
        State_set(ST_WAIT_SP);
      }


      
      break;
    }


    /* -------------------------------------------------------------------------- */
    /*                        State for recalibrate purpose                       */
    /* -------------------------------------------------------------------------- */
    case ST_CAL: {

      Solenoid_set(TURN_OFF);
















      static time_ms_t  last_add        = cur_time;
      static time_ms_t  last_unpress    = cur_time;
      bool              long_time_press = (cur_time - last_unpress > 1000);


      /* --------------------------- Prevent human prank -------------------------- */
      if(    buttons[BT_PLUS].read()  == BT_ST_PRESSING 
          && buttons[BT_MINUS].read() == BT_ST_PRESSING) 
          break;


      /* ---------------------------- Single press add ---------------------------- */
      if( buttons[BT_PLUS].rose() && !long_time_press ) {
        Cal_cmp_ADD(); 
      }else if(buttons[BT_MINUS].rose() && !long_time_press ) {
        Cal_cmp_MINUS(); 
      }


      /* ------------- Single press handle and time stamp when unpress ------------ */
      if( buttons[BT_PLUS].read() == BT_ST_PRESSING) {
        if(cur_time - last_add > 200 && long_time_press) {
          last_add = cur_time;
          Cal_cmp_ADD();
        }
        
      }else if(buttons[BT_MINUS].read() == BT_ST_PRESSING) {
        if(cur_time - last_add > 200 && long_time_press) {
          last_add = cur_time;
          Cal_cmp_MINUS();
        }
         
      }else if(buttons[BT_PLUS].read()  == BT_ST_UNPRESSED 
            && buttons[BT_MINUS].read() == BT_ST_UNPRESSED) {
        last_unpress      = cur_time;
      } 






      if(buttons[BT_START].rose()) {
        Flowmeter_reset();
        // Liter_SP_reset();
        State_set(ST_CAL_SAVE);
        SaveParameters();
        last_delay_ts = cur_time;
      }else if(buttons[BT_CAL].rose()) {
        Flowmeter_reset();
        // Liter_SP_reset();
        State_set(ST_WAIT_SP);
      }

      
      break;
    }
    case ST_CAL_SAVE : {
      if( cur_time - last_delay_ts > DELAY_BEFORE_RUN) {
        State_set(ST_WAIT_SP);
        Calibrate_set(false);
      }
      break;
    }
  }

}