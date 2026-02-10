/******************************************************************************
** File: fswv1_app_msg.h
**
** Purpose:
**   This file contains message definitions for the FSWV1 application.
**
******************************************************************************/

#ifndef FSWV1_APP_MSG_H
#define FSWV1_APP_MSG_H

/*
** Required header files
*/
#include "cfe_sb.h"

/*
** Command Codes
*/
#define FSWV1_APP_NOOP_CC           0
#define FSWV1_APP_RESET_COUNTERS_CC 1
#define FSWV1_APP_ENABLE_CC         2
#define FSWV1_APP_DISABLE_CC        3
#define FSWV1_APP_LED_ON_CC         4
#define FSWV1_APP_LED_OFF_CC        5
#define FSWV1_APP_LED_TOGGLE_CC     6
#define FSWV1_APP_LED_STATUS_CC     7

/*
** Command Structures
*/

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_EnableCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_DisableCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_LedOnCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_LedOffCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_LedToggleCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
} FSWV1_APP_LedStatusCmd_t;

/*
** Telemetry Structures
*/

/* Housekeeping Telemetry Payload */
typedef struct
{
    uint8  CommandCounter;
    uint8  CommandErrorCounter;
    uint8  SensorEnabled;
    uint8  IMUEnabled;
    uint32 ReadRate;
    uint8  LedState;
} FSWV1_APP_HkTlm_Payload_t;

/* Housekeeping Telemetry */
typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader;
    FSWV1_APP_HkTlm_Payload_t Payload;
} FSWV1_APP_HkTlm_t;

/* Combined Sensor + IMU Telemetry Payload */
typedef struct
{
    /* BMP280 Data */
    float  BMP_Temperature;  /* BMP280 temperature (°C) */
    float  BMP_Pressure;     /* BMP280 pressure (Pa) */
    
    /* IMU Data */
    float  Accel_X;          /* Accelerometer X-axis */
    float  Accel_Y;          /* Accelerometer Y-axis */
    float  Accel_Z;          /* Accelerometer Z-axis */
    float  Gyro_X;           /* Gyroscope X-axis */
    float  Gyro_Y;           /* Gyroscope Y-axis */
    float  Gyro_Z;           /* Gyroscope Z-axis */
    float  IMU_Temperature;  /* IMU temperature (°C) */
    
    uint32 Timestamp;        /* Timestamp */
} FSWV1_APP_CombinedTlm_Payload_t;

/* Combined Telemetry */
typedef struct
{
    CFE_MSG_TelemetryHeader_t         TelemetryHeader;
    FSWV1_APP_CombinedTlm_Payload_t  Payload;
} FSWV1_APP_CombinedTlm_t;

#endif /* FSWV1_APP_MSG_H */
