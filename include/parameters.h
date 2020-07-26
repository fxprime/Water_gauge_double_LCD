#include <Arduino.h>
#include <EEPROMex.h>


/* -------------------------------------------------------------------------- */
/*                              Constant variable                             */
/* -------------------------------------------------------------------------- */

#define L_PER_PULSE         1.0/78.0    //Liter per a single pulse define by sensor
#define L_PER_PRESS         0.25        //Liter per pressing button
#define L_MAX               10          //Liter max
#define L_MIN               1           //Liter min
#define DELAY_BEFORE_RUN    3000        //Millisecond
#define L_COMP_DEFAULT      1.0         //Compensate ratio default
#define SP_L_DEFAULT        5.0         //Setpoint for leter default
/* -------------------------------------------------------------------------- */
/*                                  Structuer                                 */
/* -------------------------------------------------------------------------- */

typedef enum {
    ST_INITING = 0,
    ST_WAIT_SP,
    ST_DELAY,
    ST_RUNNING,
    ST_STOP,
    ST_CAL,
    ST_CAL_SAVE
}state_en;

typedef uint32_t time_ms_t;
typedef uint64_t time_us_t;



/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

/* ----------------------------- State variable ----------------------------- */

state_en        _state;
bool            _calibrate_flag;

/* ---------------------------- Setpoint variable --------------------------- */
// int         _sp_accel = 0;
double          _sp_liter = 0;


/* --------------------------- Flowmeter variable --------------------------- */

volatile int    pulseCount        = 0;
uint32_t        _pulse_now        = 0; 

/* ---------------------- Calibration cutoff compensate --------------------- */

double          _cal_l_ratio_comp = L_COMP_DEFAULT;

/* ----------------------------- eeprome address ---------------------------- */

const int eeprom_signature = 0xb4;
const int maxAllowedWrites = 80;
const int memBase          = 350;
int _signature_address     = 100;
int _cal_l_ratio_address   = 150;
int _sp_liter_address      = 170;


/* ------------------------------- unused pin ------------------------------- */

 const int _pin_unused[] = { 0,1,3,13,A0,A1,A2,A3 };


/* -------------------------------------------------------------------------- */
/*                              Auxilary Function                             */
/* -------------------------------------------------------------------------- */
static inline double Litter_now() {
    return _pulse_now*L_PER_PULSE*_cal_l_ratio_comp;;
}
static inline void State_set(state_en st)
{
    _state = st;
}

static inline state_en State_get() {
    return _state;
}

static inline void Liter_SP_ADD()
{
    _sp_liter += L_PER_PRESS;
    _sp_liter = constrain(_sp_liter, L_MIN, L_MAX);
}
static inline void Liter_SP_MINUS()
{
    _sp_liter -= L_PER_PRESS;
    _sp_liter = constrain(_sp_liter, L_MIN, L_MAX);
}
static inline bool Liter_SP_is_enough() { 
    return _sp_liter > L_MIN;
}
static inline bool Calibrating() { return _calibrate_flag; }
static inline void Calibrate_set(bool set) {
    _calibrate_flag = set;
}
static inline bool Water_is_full() { 
    return ( Litter_now() > _sp_liter );
}
static inline void Liter_SP_reset() {
    _sp_liter = 0;
}
static inline void Cal_cmp_ADD() {
    _cal_l_ratio_comp += 0.005;
    _cal_l_ratio_comp = constrain(_cal_l_ratio_comp, 0.02, 10.0);

}
static inline void Cal_cmp_MINUS() {
    _cal_l_ratio_comp -= 0.005;
    _cal_l_ratio_comp = constrain(_cal_l_ratio_comp, 0.02, 10.0);

}
static inline void Cal_cmp_DEFAULT() {
    _cal_l_ratio_comp = 1.0;
}
static inline void SaveParameters() {
    double value_test;
    EEPROM.updateDouble(_cal_l_ratio_address, _cal_l_ratio_comp);   
    value_test = EEPROM.readDouble(_cal_l_ratio_address);
    if( fabs(_cal_l_ratio_comp - value_test) < __DBL_EPSILON__ ) {
        Serial.println("Write OK");
    }else {
        Serial.println("Write Failed!!");
    }

    EEPROM.updateDouble(_sp_liter_address, _sp_liter);   
    value_test = EEPROM.readDouble(_sp_liter_address);
    if( fabs(_sp_liter - value_test) < __DBL_EPSILON__ ) {
        Serial.println("Write OK");
    }else {
        Serial.println("Write Failed!!");
    }
}

void Parameters_init() {
    Serial.println("Parameter initing.");
    // start reading from position memBase (address 0) of the EEPROM. Set maximumSize to EEPROMSizeUno 
    // Writes before membase or beyond EEPROMSizeUno will only give errors when _EEPROMEX_DEBUG is set
    EEPROM.setMemPool(memBase, EEPROMSizeUno);

    // Set maximum allowed writes to maxAllowedWrites. 
    // More writes will only give errors when _EEPROMEX_DEBUG is set
    EEPROM.setMaxAllowedWrites(maxAllowedWrites);
    delay(100);
 

    int test_signature          = 0;
    test_signature              = EEPROM.readInt(_signature_address);

    if(test_signature == eeprom_signature) {
        _cal_l_ratio_comp   = EEPROM.readDouble(_cal_l_ratio_address);
        _sp_liter           = EEPROM.readDouble(_sp_liter_address);
        Serial.println("Found old parameter : \n\tcal_comp : " + String(_cal_l_ratio_comp,3) + "\n\tSP : " + String(_sp_liter,2));
    }else{
        EEPROM.writeInt     (_signature_address, eeprom_signature);
        EEPROM.writeDouble  (_cal_l_ratio_address, _cal_l_ratio_comp);
        Serial.println("Not found old parameter. Using default.");
    }

    Serial.println("Parameter inited.");
}