/******************************************************************************
** File: fswv1_app.c
**
** Purpose:
**   This file contains the source code for the FSWV1 Sensor Application.
**
******************************************************************************/

#include "fswv1_app.h"
#include "fswv1_app_version.h"

/*
** Global Data
*/
FSWV1_APP_Data_t FSWV1_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Application entry point and main process loop                          */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_APP_Main(void)
{
    int32 status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Perform application specific initialization
    ** Note: CFE_ES_RegisterApp() is called automatically in newer cFS versions
    */
    status = FSWV1_APP_Init();
    if (status != CFE_SUCCESS)
    {
        FSWV1_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Main app loop
    */
    while (CFE_ES_RunLoop(&FSWV1_APP_Data.RunStatus) == true)
    {
        /*
        ** Pend on receipt of command packet with timeout
        */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, FSWV1_APP_Data.CommandPipe, 500);

        if (status == CFE_SUCCESS)
        {
            FSWV1_APP_ProcessCommandPacket(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            /*
            ** Timeout occurred - perform periodic reading
            */
            
            /* Read BMP280 sensor data (if enabled) */
            if (FSWV1_APP_Data.SensorEnabled)
            {
                status = FSWV1_ReadSensor(&FSWV1_APP_Data.SensorData);
                
                if (status == CFE_SUCCESS)
                {
                    /* Update combined telemetry with BMP280 data */
                    FSWV1_APP_Data.CombinedTlm.Payload.BMP_Temperature = 
                        FSWV1_APP_Data.SensorData.Temperature;
                    FSWV1_APP_Data.CombinedTlm.Payload.BMP_Pressure = 
                        FSWV1_APP_Data.SensorData.Pressure;
                    
                    /* Print to terminal */
                    OS_printf("FSWV1: BMP Temp=%.2f°C, Press=%.2f Pa\n",
                             FSWV1_APP_Data.SensorData.Temperature,
                             FSWV1_APP_Data.SensorData.Pressure);
                }
                else
                {
                    CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                                    "FSWV1: Sensor read error, status = 0x%08X", (unsigned int)status);
                }
            }
            
            /* Read IMU data from UART (always, independent of SensorEnabled) */
            if (FSWV1_APP_Data.IMUEnabled)
            {
                status = FSWV1_ReadUART(&FSWV1_APP_Data.IMUData);
                if (status == CFE_SUCCESS)
                {
                    /* Update combined telemetry with IMU data */
                    FSWV1_APP_Data.CombinedTlm.Payload.Accel_X = FSWV1_APP_Data.IMUData.Accel_X;
                    FSWV1_APP_Data.CombinedTlm.Payload.Accel_Y = FSWV1_APP_Data.IMUData.Accel_Y;
                    FSWV1_APP_Data.CombinedTlm.Payload.Accel_Z = FSWV1_APP_Data.IMUData.Accel_Z;
                    FSWV1_APP_Data.CombinedTlm.Payload.Gyro_X = FSWV1_APP_Data.IMUData.Gyro_X;
                    FSWV1_APP_Data.CombinedTlm.Payload.Gyro_Y = FSWV1_APP_Data.IMUData.Gyro_Y;
                    FSWV1_APP_Data.CombinedTlm.Payload.Gyro_Z = FSWV1_APP_Data.IMUData.Gyro_Z;
                    FSWV1_APP_Data.CombinedTlm.Payload.IMU_Temperature = FSWV1_APP_Data.IMUData.Temperature;
                    
                    /* Print IMU data */
                    OS_printf("FSWV1: IMU Ax=%.2f Ay=%.2f Az=%.2f Gx=%.2f Gy=%.2f Gz=%.2f T=%.2f\n",
                             FSWV1_APP_Data.IMUData.Accel_X, FSWV1_APP_Data.IMUData.Accel_Y, FSWV1_APP_Data.IMUData.Accel_Z,
                             FSWV1_APP_Data.IMUData.Gyro_X, FSWV1_APP_Data.IMUData.Gyro_Y, FSWV1_APP_Data.IMUData.Gyro_Z,
                             FSWV1_APP_Data.IMUData.Temperature);
                }
                /* IMU read errors are OK - just no data available yet */
            }
            
            /* Always transmit telemetry (even if sensors disabled, send zeros) */
            FSWV1_APP_Data.CombinedTlm.Payload.Timestamp = CFE_TIME_GetTime().Seconds;
            
            /* Update sequence count */
            CFE_MSG_SetSequenceCount(CFE_MSG_PTR(FSWV1_APP_Data.CombinedTlm.TelemetryHeader),
                                    FSWV1_APP_Data.CombinedTlmSeqCnt);
            FSWV1_APP_Data.CombinedTlmSeqCnt++;
            
            /* Timestamp and transmit on Software Bus */
            CFE_SB_TimeStampMsg(CFE_MSG_PTR(FSWV1_APP_Data.CombinedTlm.TelemetryHeader));
            CFE_SB_TransmitMsg(CFE_MSG_PTR(FSWV1_APP_Data.CombinedTlm.TelemetryHeader), false);
            
            /* Send combined data via UDP */
            FSWV1_SendUDP(&FSWV1_APP_Data.SensorData, &FSWV1_APP_Data.IMUData);
            
            /* Send combined data via Telemetry UART */
            FSWV1_SendTelemetryUART(&FSWV1_APP_Data.SensorData, &FSWV1_APP_Data.IMUData);
        }
        else
        {
            CFE_EVS_SendEvent(FSWV1_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                            "FSWV1: SB pipe read error, RC = 0x%08X", (unsigned int)status);
        }
    }

    /*
    ** Cleanup before exit
    */
    FSWV1_CloseSensor();
    FSWV1_CloseUDP();
    FSWV1_CloseTelemetryUART();
    FSWV1_CloseGPIO();
    FSWV1_CloseUART();
    
    CFE_ES_ExitApp(FSWV1_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialization                                                          */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_Init(void)
{
    int32 status;

    FSWV1_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize counters
    */
    FSWV1_APP_Data.CmdCounter = 0;
    FSWV1_APP_Data.ErrCounter = 0;
    FSWV1_APP_Data.SensorEnabled = true;
    FSWV1_APP_Data.IMUEnabled = true;
    FSWV1_APP_Data.ReadRate = FSWV1_DEFAULT_READ_RATE;
    FSWV1_APP_Data.CombinedTlmSeqCnt = 0;
    FSWV1_APP_Data.CombinedTlmSeqCnt = 0;
    FSWV1_APP_Data.LedState = false;

    /*
    ** Initialize app configuration data
    */
    FSWV1_APP_Data.CommandPipe = CFE_SB_INVALID_PIPE;

    /*
    ** Register event filter table
    */
    FSWV1_APP_Data.EventFilters[0].EventID = FSWV1_APP_INIT_INF_EID;
    FSWV1_APP_Data.EventFilters[0].Mask = 0x0000;
    FSWV1_APP_Data.EventFilters[1].EventID = FSWV1_APP_COMMANDNOP_INF_EID;
    FSWV1_APP_Data.EventFilters[1].Mask = 0x0000;
    FSWV1_APP_Data.EventFilters[2].EventID = FSWV1_APP_COMMANDRST_INF_EID;
    FSWV1_APP_Data.EventFilters[2].Mask = 0x0000;

    status = CFE_EVS_Register(FSWV1_APP_Data.EventFilters,
                              FSWV1_APP_EVENT_COUNTS,
                              CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("FSWV1: Error registering events, RC = 0x%08X\n", (unsigned int)status);
        return status;
    }

    /*
    ** Create Software Bus message pipe
    */
    status = CFE_SB_CreatePipe(&FSWV1_APP_Data.CommandPipe,
                              FSWV1_APP_PIPE_DEPTH,
                              "FSWV1_CMD_PIPE");
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: Error creating pipe, RC = 0x%08X", (unsigned int)status);
        return status;
    }

    /*
    ** Subscribe to command messages
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FSWV1_APP_CMD_MID),
                             FSWV1_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: Error subscribing to CMD, RC = 0x%08X", (unsigned int)status);
        return status;
    }

    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FSWV1_APP_SEND_HK_MID),
                             FSWV1_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: Error subscribing to SEND_HK, RC = 0x%08X", (unsigned int)status);
        return status;
    }

    /*
    ** Initialize telemetry messages
    */
    CFE_MSG_Init(CFE_MSG_PTR(FSWV1_APP_Data.HkTlm.TelemetryHeader),
                CFE_SB_ValueToMsgId(FSWV1_APP_HK_TLM_MID),
                sizeof(FSWV1_APP_Data.HkTlm));

    CFE_MSG_Init(CFE_MSG_PTR(FSWV1_APP_Data.CombinedTlm.TelemetryHeader),
                CFE_SB_ValueToMsgId(FSWV1_APP_COMBINED_TLM_MID),
                sizeof(FSWV1_APP_Data.CombinedTlm));

    /*
    ** Initialize sensor
    */
    status = FSWV1_InitSensor();
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: Sensor initialization failed, RC = 0x%08X", (unsigned int)status);
        /* Continue anyway - sensor might be simulated */
    }

    /*
    ** Initialize UDP
    */
    status = FSWV1_InitUDP();
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UDP_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: UDP initialization failed, RC = 0x%08X", (unsigned int)status);
        /* Continue anyway */
    }

    /*
    ** Initialize Telemetry UART for transmitting telemetry
    */
    status = FSWV1_InitTelemetryUART();
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_TELEMETRY_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: Telemetry UART initialization failed, RC = 0x%08X", (unsigned int)status);
        /* Continue anyway */
    }

    /*
    ** Initialize GPIO for LED control
    */
    status = FSWV1_InitGPIO();
    
    /*
    ** Initialize UART for IMU data
    */
    status = FSWV1_InitUART();
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: UART initialization failed, RC = 0x%08X", (unsigned int)status);
        /* Continue anyway - UART is optional */
    }
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                        "FSWV1: GPIO initialization failed, RC = 0x%08X", (unsigned int)status);
        /* Continue anyway - LED commands will fail gracefully */
    }

    CFE_EVS_SendEvent(FSWV1_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1 App Initialized. Version %d.%d.%d.%d",
                     FSWV1_APP_MAJOR_VERSION,
                     FSWV1_APP_MINOR_VERSION,
                     FSWV1_APP_REVISION,
                     FSWV1_APP_MISSION_REV);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Process command packets                                                */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case FSWV1_APP_CMD_MID:
            FSWV1_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case FSWV1_APP_SEND_HK_MID:
            FSWV1_APP_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(FSWV1_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                            "FSWV1: Invalid command pipe message ID: 0x%x",
                            (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Process ground commands                                                */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    switch (CommandCode)
    {
        case FSWV1_APP_NOOP_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_NoopCmd_t)))
            {
                FSWV1_APP_Noop((FSWV1_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_RESET_COUNTERS_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_ResetCountersCmd_t)))
            {
                FSWV1_APP_ResetCounters((FSWV1_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_ENABLE_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_EnableCmd_t)))
            {
                FSWV1_APP_Enable((FSWV1_APP_EnableCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_DISABLE_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_DisableCmd_t)))
            {
                FSWV1_APP_Disable((FSWV1_APP_DisableCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_LED_ON_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_LedOnCmd_t)))
            {
                FSWV1_APP_LedOn((FSWV1_APP_LedOnCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_LED_OFF_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_LedOffCmd_t)))
            {
                FSWV1_APP_LedOff((FSWV1_APP_LedOffCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_LED_TOGGLE_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_LedToggleCmd_t)))
            {
                FSWV1_APP_LedToggle((FSWV1_APP_LedToggleCmd_t *)SBBufPtr);
            }
            break;

        case FSWV1_APP_LED_STATUS_CC:
            if (FSWV1_APP_VerifyCommandLength(&SBBufPtr->Msg, sizeof(FSWV1_APP_LedStatusCmd_t)))
            {
                FSWV1_APP_LedStatus((FSWV1_APP_LedStatusCmd_t *)SBBufPtr);
            }
            break;

        default:
            FSWV1_APP_Data.ErrCounter++;
            CFE_EVS_SendEvent(FSWV1_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                            "FSWV1: Invalid ground command code: %u", CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Report housekeeping                                                    */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    bool led_state;
    
    /*
    ** Update housekeeping telemetry
    */
    FSWV1_APP_Data.HkTlm.Payload.CommandCounter = FSWV1_APP_Data.CmdCounter;
    FSWV1_APP_Data.HkTlm.Payload.CommandErrorCounter = FSWV1_APP_Data.ErrCounter;
    FSWV1_APP_Data.HkTlm.Payload.SensorEnabled = FSWV1_APP_Data.SensorEnabled ? 1 : 0;
    FSWV1_APP_Data.HkTlm.Payload.ReadRate = FSWV1_APP_Data.ReadRate;
    
    /* Get current LED state */
    if (FSWV1_GetLED(&led_state) == CFE_SUCCESS)
    {
        FSWV1_APP_Data.HkTlm.Payload.LedState = led_state ? 1 : 0;
        FSWV1_APP_Data.LedState = led_state;
    }
    else
    {
        FSWV1_APP_Data.HkTlm.Payload.LedState = 0;
    }

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(FSWV1_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(FSWV1_APP_Data.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* NOOP command                                                            */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_Noop(const FSWV1_APP_NoopCmd_t *Msg)
{
    FSWV1_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(FSWV1_APP_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1: NOOP command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Reset counters command                                                  */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_ResetCounters(const FSWV1_APP_ResetCountersCmd_t *Msg)
{
    FSWV1_APP_Data.CmdCounter = 0;
    FSWV1_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(FSWV1_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Enable sensor command                                                   */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_Enable(const FSWV1_APP_EnableCmd_t *Msg)
{
    FSWV1_APP_Data.SensorEnabled = true;
    FSWV1_APP_Data.IMUEnabled = true;
    FSWV1_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(FSWV1_APP_ENABLE_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1: Sensor ENABLED");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Disable sensor command                                                  */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_Disable(const FSWV1_APP_DisableCmd_t *Msg)
{
    FSWV1_APP_Data.SensorEnabled = false;
    FSWV1_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(FSWV1_APP_DISABLE_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1: Sensor DISABLED");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Verify command length                                                   */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
bool FSWV1_APP_VerifyCommandLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool result = true;
    size_t ActualLength = 0;
    CFE_MSG_FcnCode_t FcnCode = 0;
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(FSWV1_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Invalid msg length: ID = 0x%X, CC = %u, Len = %u, Expected = %u",
                         (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode,
                         (unsigned int)ActualLength, (unsigned int)ExpectedLength);

        result = false;
        FSWV1_APP_Data.ErrCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* LED ON command                                                          */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_LedOn(const FSWV1_APP_LedOnCmd_t *Msg)
{
    int32 status;
    
    status = FSWV1_SetLED(true);
    
    if (status == CFE_SUCCESS)
    {
        FSWV1_APP_Data.CmdCounter++;
        FSWV1_APP_Data.LedState = true;
        
        CFE_EVS_SendEvent(FSWV1_APP_LED_ON_INF_EID, CFE_EVS_EventType_INFORMATION,
                         "FSWV1: LED turned ON");
    }
    else
    {
        FSWV1_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: LED ON command failed, RC = 0x%08X", (unsigned int)status);
    }
    
    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* LED OFF command                                                         */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_LedOff(const FSWV1_APP_LedOffCmd_t *Msg)
{
    int32 status;
    
    status = FSWV1_SetLED(false);
    
    if (status == CFE_SUCCESS)
    {
        FSWV1_APP_Data.CmdCounter++;
        FSWV1_APP_Data.LedState = false;
        
        CFE_EVS_SendEvent(FSWV1_APP_LED_OFF_INF_EID, CFE_EVS_EventType_INFORMATION,
                         "FSWV1: LED turned OFF");
    }
    else
    {
        FSWV1_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: LED OFF command failed, RC = 0x%08X", (unsigned int)status);
    }
    
    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* LED TOGGLE command                                                      */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_LedToggle(const FSWV1_APP_LedToggleCmd_t *Msg)
{
    int32 status;
    bool new_state;
    
    status = FSWV1_ToggleLED();
    
    if (status == CFE_SUCCESS)
    {
        FSWV1_APP_Data.CmdCounter++;
        
        /* Get the new state after toggle */
        if (FSWV1_GetLED(&new_state) == CFE_SUCCESS)
        {
            FSWV1_APP_Data.LedState = new_state;
        }
        
        CFE_EVS_SendEvent(FSWV1_APP_LED_TOGGLE_INF_EID, CFE_EVS_EventType_INFORMATION,
                         "FSWV1: LED toggled to %s", FSWV1_APP_Data.LedState ? "ON" : "OFF");
    }
    else
    {
        FSWV1_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: LED TOGGLE command failed, RC = 0x%08X", (unsigned int)status);
    }
    
    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* LED STATUS command                                                      */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_APP_LedStatus(const FSWV1_APP_LedStatusCmd_t *Msg)
{
    int32 status;
    bool led_state;
    
    status = FSWV1_GetLED(&led_state);
    
    if (status == CFE_SUCCESS)
    {
        FSWV1_APP_Data.CmdCounter++;
        FSWV1_APP_Data.LedState = led_state;
        
        CFE_EVS_SendEvent(FSWV1_APP_LED_STATUS_INF_EID, CFE_EVS_EventType_INFORMATION,
                         "FSWV1: LED status is %s", led_state ? "ON" : "OFF");
    }
    else
    {
        FSWV1_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: LED STATUS command failed, RC = 0x%08X", (unsigned int)status);
    }
    
    return status;
}
