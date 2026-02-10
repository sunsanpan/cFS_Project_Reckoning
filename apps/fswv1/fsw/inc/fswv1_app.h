/******************************************************************************
** File: fswv1_app.h
**
** Purpose:
**   This file contains the main header for the FSWV1 Application
**   Includes BMP280 sensor, IMU data via UART, and LED control
**
******************************************************************************/

#ifndef FSWV1_APP_H
#define FSWV1_APP_H

/*
** Required header files
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"

#include "fswv1_app_msgids.h"
#include "fswv1_app_msg.h"

#include "osapi.h"
#include "common_types.h"

/***********************************************************************/
/*
** Configuration Parameters
*/

#define FSWV1_APP_PIPE_DEPTH 32
#define FSWV1_APP_EVENT_COUNTS 5

/***********************************************************************/
/*
** Type Definitions
*/

/*
** BMP280 Sensor Data Structure
*/
typedef struct
{
    float Temperature;
    float Pressure;
    uint32 Timestamp;
} FSWV1_SensorData_t;

/*
** IMU Data Structure (from UART)
** Format: "$,Ax,Ay,Az,Gx,Gy,Gz,Temperature,#"
*/
typedef struct
{
    float Accel_X;      /* Accelerometer X-axis */
    float Accel_Y;      /* Accelerometer Y-axis */
    float Accel_Z;      /* Accelerometer Z-axis */
    float Gyro_X;       /* Gyroscope X-axis */
    float Gyro_Y;       /* Gyroscope Y-axis */
    float Gyro_Z;       /* Gyroscope Z-axis */
    float Temperature;  /* IMU Temperature */
    uint32 Timestamp;   /* Timestamp */
} FSWV1_IMUData_t;

/*
** Global Data Structure
*/
typedef struct
{
    /*
    ** Command interface counters
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet
    */
    FSWV1_APP_HkTlm_t HkTlm;

    /*
    ** Combined telemetry packet (BMP280 + IMU)
    */
    FSWV1_APP_CombinedTlm_t CombinedTlm;

    /*
    ** Run Status variable
    */
    uint32 RunStatus;

    /*
    ** Software Bus Pipe ID
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Event table ID
    */
    CFE_EVS_BinFilter_t EventFilters[FSWV1_APP_EVENT_COUNTS];

    /*
    ** UDP Socket for telemetry transmission
    */
    osal_id_t UdpSocket;
    
    /*
    ** Sensor data
    */
    FSWV1_SensorData_t SensorData;
    
    /*
    ** IMU data
    */
    FSWV1_IMUData_t IMUData;
    
    /*
    ** App state
    */
    bool SensorEnabled;
    bool IMUEnabled;
    uint32 ReadRate;
    uint16 CombinedTlmSeqCnt;
    bool LedState;

} FSWV1_APP_Data_t;

/*
** Exported Data
*/
extern FSWV1_APP_Data_t FSWV1_APP_Data;

/***********************************************************************/
/*
** Application Function Prototypes
*/

/*
** Application entry point and main process loop
*/
void FSWV1_APP_Main(void);

/*
** Application initialization
*/
int32 FSWV1_APP_Init(void);

/*
** Application command processing
*/
void FSWV1_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void FSWV1_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);

/*
** Command handlers
*/
int32 FSWV1_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 FSWV1_APP_ResetCounters(const FSWV1_APP_ResetCountersCmd_t *Msg);
int32 FSWV1_APP_Noop(const FSWV1_APP_NoopCmd_t *Msg);
int32 FSWV1_APP_Enable(const FSWV1_APP_EnableCmd_t *Msg);
int32 FSWV1_APP_Disable(const FSWV1_APP_DisableCmd_t *Msg);
int32 FSWV1_APP_LedOn(const FSWV1_APP_LedOnCmd_t *Msg);
int32 FSWV1_APP_LedOff(const FSWV1_APP_LedOffCmd_t *Msg);
int32 FSWV1_APP_LedToggle(const FSWV1_APP_LedToggleCmd_t *Msg);
int32 FSWV1_APP_LedStatus(const FSWV1_APP_LedStatusCmd_t *Msg);

/*
** BMP280 Sensor functions
*/
int32 FSWV1_InitSensor(void);
int32 FSWV1_ReadSensor(FSWV1_SensorData_t *Data);
void FSWV1_CloseSensor(void);

/*
** UART/IMU functions (for receiving IMU data)
*/
int32 FSWV1_InitUART(void);
int32 FSWV1_ReadUART(FSWV1_IMUData_t *Data);
void FSWV1_CloseUART(void);

/*
** UART Telemetry functions (for transmitting telemetry data)
*/
int32 FSWV1_InitTelemetryUART(void);
int32 FSWV1_SendTelemetryUART(const FSWV1_SensorData_t *SensorData, const FSWV1_IMUData_t *IMUData);
void FSWV1_CloseTelemetryUART(void);

/*
** GPIO functions
*/
int32 FSWV1_InitGPIO(void);
int32 FSWV1_SetLED(bool state);
int32 FSWV1_GetLED(bool *state);
int32 FSWV1_ToggleLED(void);
void FSWV1_CloseGPIO(void);

/*
** UDP functions
*/
int32 FSWV1_InitUDP(void);
int32 FSWV1_SendUDP(const FSWV1_SensorData_t *SensorData, const FSWV1_IMUData_t *IMUData);
void FSWV1_CloseUDP(void);

/*
** Utility functions
*/
bool FSWV1_APP_VerifyCommandLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

/***********************************************************************/
/*
** Application Configuration Parameters
*/

#define FSWV1_APP_MISSION_REV 0
#define FSWV1_APP_MAJOR_VERSION 1
#define FSWV1_APP_MINOR_VERSION 0
#define FSWV1_APP_REVISION 0

/*
** I2C Configuration (BMP280)
*/
#define FSWV1_I2C_DEVICE "/dev/i2c-1"
#define FSWV1_I2C_ADDRESS 0x76

/*
** UART Configuration (IMU)
*/
#define FSWV1_UART_DEVICE "/dev/ttyAMA0"

/*
** GPIO Configuration (LED)
*/
#define FSWV1_GPIO_PIN 17

/*
** UDP Configuration
*/
#define FSWV1_UDP_PORT 1237
#define FSWV1_UDP_DEST_IP "100.99.41.92"

/*
** Sensor Read Rate
*/
#define FSWV1_DEFAULT_READ_RATE 1  /* Hz */

/*
** Event IDs
*/
#define FSWV1_APP_RESERVED_EID      0
#define FSWV1_APP_INIT_INF_EID      1
#define FSWV1_APP_COMMANDNOP_INF_EID 2
#define FSWV1_APP_COMMANDRST_INF_EID 3
#define FSWV1_APP_INVALID_MSGID_ERR_EID 4
#define FSWV1_APP_LEN_ERR_EID       5
#define FSWV1_APP_PIPE_ERR_EID      6
#define FSWV1_APP_SENSOR_ERR_EID    7
#define FSWV1_APP_UDP_ERR_EID       8
#define FSWV1_APP_ENABLE_INF_EID    9
#define FSWV1_APP_DISABLE_INF_EID   10
#define FSWV1_APP_GPIO_INIT_INF_EID 11
#define FSWV1_APP_GPIO_ERR_EID      12
#define FSWV1_APP_LED_ON_INF_EID    13
#define FSWV1_APP_LED_OFF_INF_EID   14
#define FSWV1_APP_LED_TOGGLE_INF_EID 15
#define FSWV1_APP_LED_STATUS_INF_EID 16
#define FSWV1_APP_UART_INIT_INF_EID 17
#define FSWV1_APP_UART_ERR_EID      18
#define FSWV1_APP_IMU_ERR_EID       19
#define FSWV1_APP_UART_TELEMETRY_INIT_INF_EID 20
#define FSWV1_APP_UART_TELEMETRY_ERR_EID      21

#endif /* FSWV1_APP_H */
