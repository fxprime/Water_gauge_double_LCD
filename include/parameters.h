#include <Arduino.h>
#include <EEPROMex.h>
#include <Bounce2.h>



/* -------------------------------------------------------------------------- */
/*                              Constant variable                             */
/* -------------------------------------------------------------------------- */
#define FLOWMETER_PPL       100.0       //Flowmeter spec
#define L_PER_PULSE         1.0/FLOWMETER_PPL    //Liter per a single pulse define by sensor
#define L_PER_PRESS         0.25        //Liter per pressing button
#define L_MAX               50          //Liter max
#define L_MIN               1           //Liter min
#define DELAY_BEFORE_RUN    3000        //Millisecond
#define L_COMP_DEFAULT      1.0         //Compensate ratio default
#define SP_L_DEFAULT        5.0         //Setpoint for leter default


#define NUM_BUTTONS 4
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {5, 7, 9, 11};
const uint8_t BUTTON_PINS_GND[NUM_BUTTONS] = {6, 8, 10, 12};


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



/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

/* ----------------------------- State variable ----------------------------- */

state_en        _state;
bool            _calibrate_flag;

/* ----------------------------- Button variable ---------------------------- */

Bounce *buttons = new Bounce[NUM_BUTTONS];


/* ---------------------------- Setpoint variable --------------------------- */
double          _sp_liter = SP_L_DEFAULT;


/* --------------------------- Flowmeter variable --------------------------- */

volatile int    pulseCount        = 0;
uint32_t        _pulse_now        = 0; 
uint32_t        _pulse_trip       = 0;

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
static inline double Litter_trip_now() {
    return _pulse_trip*L_PER_PULSE*_cal_l_ratio_comp;;
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
    _cal_l_ratio_comp *= (1+0.01/Litter_now());
    _cal_l_ratio_comp = constrain(_cal_l_ratio_comp, 0.1, 10.0);

}
static inline void Cal_cmp_MINUS() {
    _cal_l_ratio_comp *= (1-0.01/Litter_now());
    _cal_l_ratio_comp = constrain(_cal_l_ratio_comp, 0.1, 10.0);

}
static inline void Cal_cmp_DEFAULT() {
    _cal_l_ratio_comp = L_COMP_DEFAULT;
}


static inline bool UpdateDoubleParam(const int& adds, double& val) {
    double value_test;
    EEPROM.updateDouble(adds, val);   
    value_test = EEPROM.readDouble(adds);
    if( fabs(val - value_test) < __DBL_EPSILON__ ) {
        Serial.println("Write OK");
    }else {
        Serial.println("Write Failed!!");
    }
}
static inline void SaveParameters() { 
    UpdateDoubleParam(_cal_l_ratio_address, _cal_l_ratio_comp);
    UpdateDoubleParam(_sp_liter_address, _sp_liter);
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
 

    int test_signature;
    test_signature              = EEPROM.readInt(_signature_address);




    if(test_signature == eeprom_signature) {
        _cal_l_ratio_comp   = EEPROM.readDouble(_cal_l_ratio_address);
        _sp_liter           = EEPROM.readDouble(_sp_liter_address);
        if(isnan(_cal_l_ratio_comp))    { _cal_l_ratio_comp     = L_COMP_DEFAULT;};
        if(isnan(_sp_liter))            { _sp_liter             = SP_L_DEFAULT;};
        _cal_l_ratio_comp   = constrain(_cal_l_ratio_comp   , 0.1, 10.0);
        _sp_liter           = constrain(_sp_liter           , L_MIN, L_MAX);
        Serial.println("Found old parameter : \n\tcal_comp : " + String(_cal_l_ratio_comp,3) + "\n\tSP : " + String(_sp_liter,2));
    }else{
        EEPROM.writeInt     (_signature_address     , eeprom_signature);
        EEPROM.writeDouble  (_cal_l_ratio_address   , L_COMP_DEFAULT);
        EEPROM.writeDouble  (_sp_liter_address      , SP_L_DEFAULT);
        Serial.println("Not found old parameter. Using default.");
    }

    Serial.println("Parameter inited.");
}
