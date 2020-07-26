/* -------------------------------------------------------------------------- */
/*                     Water pump counter show up in 2 lcd                    */
/* -------------------------------------------------------------------------- */

#include "Arduino.h"
#include "avr/wdt.h"
#include "parameters.h"
#include "flow_meter_handle.h"
#include "double_lcd_handle.h"
#include "solenoid_control_handle.h"
#include "button_handle.h"

void setup()
{
  wdt_enable(WDTO_4S);
  State_set(ST_INITING);
  Serial.begin(115200);
  EMI_shield_init();
  Parameters_init();
  Button_init();
  Flowmeter_init();
  Double_LCD_init();
  Solenoid_init(); 
  Double_LCD_update();
  wdt_disable();
}

void loop()
{
  wdt_enable(WDTO_500MS);
  time_ms_t cur_time = millis();

  Button_update();
  Button_states();

  static time_ms_t last_10hz = millis();
  if( cur_time - last_10hz > 100 ) {
    last_10hz = cur_time;
    Flowmeter_update();
    Double_LCD_update();
  } 
  wdt_disable();
}
