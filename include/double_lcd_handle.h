#include <Arduino.h>

#include <Wire.h>              //for ESP8266 use bug free i2c driver https://github.com/enjoyneering/ESP8266-I2C-Driver
#include <LiquidCrystal_I2C.h>



#define COLUMS           16
#define ROWS             2

#define LCD_SPACE_SYMBOL 0x20  //space symbol from the LCD ROM, see p.9 of GDM2004D datasheet

LiquidCrystal_I2C rt_disp(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
LiquidCrystal_I2C sp_disp(PCF8574_ADDR_A21_A11_A00, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);



void Double_LCD_init() {
  Serial.print("Double lcd initing..\n");

  while (rt_disp.begin(COLUMS, ROWS) != 1) //colums - 20, rows - 4
  {
    Serial.println(F("PCF8574 is not connected or rt_disp pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);   
  }


  while (sp_disp.begin(COLUMS, ROWS) != 1) //colums - 20, rows - 4
  {
    Serial.println(F("PCF8574 is not connected or rt_disp pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);   
  }

  rt_disp.print(F("Realtime is OK.."));    //(F()) saves string to flash & keeps dynamic memory free
  sp_disp.print(F("Setpoint is OK.."));    //(F()) saves string to flash & keeps dynamic memory free
  delay(2000);

  rt_disp.clear();
  sp_disp.clear();




  sp_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
  sp_disp.print(F("Setpoint Liter : "));

  sp_disp.setCursor(0, 1);
  sp_disp.print(F("          L     "));
 
  Serial.print("Double lcd inited\n"); 
}

static inline void Realtime_disp_static_running() {
    rt_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
    rt_disp.print(F("Current Liter : "));

    rt_disp.setCursor(0, 1);
    rt_disp.print(F("          L     "));
}

static inline void Realtime_disp_static_delay(int countdown) {
    rt_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
    rt_disp.print(F("  Runing in   s "));
    rt_disp.setCursor(12, 0);
    rt_disp.print(countdown);
 
}

static inline void Realtime_disp_static_waiting() {
    rt_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
    rt_disp.print(F("<-- Wait"));
    rt_disp.setCursor(0, 1);
    rt_disp.print(F("    Setpoint"));
}

static inline void Setpoint_disp_static_waiting() {
    sp_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
    sp_disp.print(F("Setpoint Liter : "));

    sp_disp.setCursor(0, 1);
    sp_disp.print(F("          L     "));
}
static inline void Setpoint_disp_static_cal() {
    sp_disp.setCursor(0, 0);                 //set 1-st colum & 2-nd row, 1-st colum & row started at zero
    sp_disp.print(F("Current Ratio : "));

    sp_disp.setCursor(0, 1);
    sp_disp.print(F("             "));

}
static inline void Setpoint_disp_clear() {
    sp_disp.clear();
}
static inline void Realtime_disp_clear() {
    rt_disp.clear(); 
}

static inline void Double_LCD_update() {

            time_ms_t cur_time      = millis();
    static  time_ms_t last_st_delay = 0;
     
    


    static state_en last_state = State_get();


    /* ------------ If state change, simply clear or stamp some time ------------ */
    if      ( last_state!=ST_WAIT_SP && State_get() == ST_WAIT_SP ) {
        Realtime_disp_clear();
        Realtime_disp_static_waiting();
        Setpoint_disp_clear();
        Setpoint_disp_static_waiting();
    }else if( last_state!=ST_RUNNING && State_get() == ST_RUNNING ) {
        Realtime_disp_static_running();
    }else if( last_state!=ST_DELAY && State_get() == ST_DELAY ) {
        last_st_delay = cur_time;
    }else if( last_state!=ST_CAL && State_get() == ST_CAL ) {
        Setpoint_disp_static_cal();
    }else if( last_state!=ST_CAL_SAVE && State_get() == ST_CAL_SAVE ) {
        Realtime_disp_clear();
        Setpoint_disp_clear();
        sp_disp.setCursor(0, 0);
        sp_disp.print( F("Parameters Saved!") );
    }

    /* -- If state is running, show current litter but constrain with setpoint -- */

    if      (State_get() == ST_WAIT_SP) {
        sp_disp.setCursor(3, 1);
        sp_disp.print(_sp_liter);
        sp_disp.write(LCD_SPACE_SYMBOL);
    }       
    else if (State_get() == ST_RUNNING) { 
        rt_disp.setCursor(3, 1);
        rt_disp.print( min(Litter_now(), _sp_liter) );
        rt_disp.write(LCD_SPACE_SYMBOL);
    }

    /* ------------- If state is delaying, blink and show count down ------------ */
    else if (State_get() == ST_DELAY) {
        if( (cur_time-last_st_delay)%1000 > 500 ) {
            Realtime_disp_static_delay( ( DELAY_BEFORE_RUN - (cur_time-last_st_delay))/1000 );
        }else{
            Realtime_disp_clear();
        }
    }

    /* -- If state is calibrating, show exactly liter and not being constraint -- */
    else if (State_get() == ST_CAL) {
        rt_disp.setCursor(3, 1);
        rt_disp.print( Litter_now() );
        rt_disp.write(LCD_SPACE_SYMBOL);

        sp_disp.setCursor(3, 1);
        sp_disp.print( _cal_l_ratio_comp );
        sp_disp.write(LCD_SPACE_SYMBOL);
    }
 
    /* -------------------------- Isolate flag display -------------------------- */
    {
 
        if(Calibrating()) {
            sp_disp.setCursor(13,1);
            sp_disp.print(F("CAL"));
        }else{
            sp_disp.setCursor(13,1);
            sp_disp.print(F("   "));
        } 

    }
    
    last_state = State_get();
  
    
}