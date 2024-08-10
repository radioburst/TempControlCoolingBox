#ifndef _globals_h_
#define _globals_h_

typedef enum STATES_t
{
    SLEEP = 0,
    WAKE_UP,
    IDLE,
    MEASURE_ALL,
    EDIT_TEMP_MODE,
    MENU,

} STATES_t;

extern volatile STATES_t state;

typedef enum OPERATION_MODES_t
{
    GAS = 0,
    DC,
    AC
} OPERATION_MODES_t;

extern OPERATION_MODES_t mode;

extern float fSetTemp;

extern float fTempThrPos, fTempThrNeg;

#endif