/******************************************************************************
** File: fswv1_sensor.c
**
** Purpose:
**   This file contains the sensor interface and UDP functions for FSWV1 app.
**   Uses native I2C file descriptors (not OSAL) to match working implementation.
**
******************************************************************************/

#include "fswv1_app.h"
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/*
** FSWV1 Register Definitions
*/
#define FSWV1_REG_CHIP_ID      0xD0
#define FSWV1_REG_RESET        0xE0
#define FSWV1_REG_STATUS       0xF3
#define FSWV1_REG_CTRL_MEAS    0xF4
#define FSWV1_REG_CONFIG       0xF5
#define FSWV1_REG_PRESS_MSB    0xF7
#define FSWV1_REG_TEMP_MSB     0xFA
#define FSWV1_REG_CALIB_00     0x88

#define FSWV1_CHIP_ID          0x58

/*
** Calibration data structure
*/
typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} FSWV1_CalibData_t;

/*
** Static variables - using native file descriptor instead of OSAL
*/
static int I2C_Fd = -1;  /* Native file descriptor */
static FSWV1_CalibData_t CalibData;
static int32 t_fine; /* Used in compensation calculations */

/*
** Forward declarations
*/
static int32 FSWV1_ReadCalibrationData(void);
static int32 FSWV1_CompensateTemperature(int32 adc_T);
static uint32 FSWV1_CompensatePressure(int32 adc_P);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* I2C Write Register (using native write)                                */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 FSWV1_WriteReg(uint8 reg, uint8 value)
{
    uint8 buf[2] = {reg, value};
    
    if (write(I2C_Fd, buf, 2) != 2)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* I2C Read Registers (using native read/write)                           */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 FSWV1_ReadReg(uint8 reg, uint8 *data, uint8 len)
{
    /* Write register address */
    if (write(I2C_Fd, &reg, 1) != 1)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Read data */
    if (read(I2C_Fd, data, len) != len)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialize FSWV1 sensor                                               */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_InitSensor(void)
{
    uint8 chip_id;

    /*
    ** Open I2C device using native open() (not OSAL)
    */
    I2C_Fd = open(FSWV1_I2C_DEVICE, O_RDWR);
    if (I2C_Fd < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to open I2C device %s", FSWV1_I2C_DEVICE);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    /*
    ** Set I2C slave address using ioctl (native, not OSAL)
    */
    if (ioctl(I2C_Fd, I2C_SLAVE, FSWV1_I2C_ADDRESS) < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to set I2C slave address 0x%02X", FSWV1_I2C_ADDRESS);
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    /*
    ** Read and verify chip ID
    */
    if (FSWV1_ReadReg(FSWV1_REG_CHIP_ID, &chip_id, 1) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to read chip ID");
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    if (chip_id != FSWV1_CHIP_ID)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Invalid chip ID: 0x%02X (expected 0x58)", chip_id);
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    OS_printf("FSWV1: Chip ID verified: 0x%02X\n", chip_id);

    /*
    ** Configure sensor
    ** ctrl_meas: 0x27 = osrs_t x1, osrs_p x1, normal mode
    */
    if (FSWV1_WriteReg(FSWV1_REG_CTRL_MEAS, 0x27) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to configure CTRL_MEAS");
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    /*
    ** config: 0x00 = standby 0.5ms, filter off
    */
    if (FSWV1_WriteReg(FSWV1_REG_CONFIG, 0x00) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to configure CONFIG");
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    /* Wait for sensor to stabilize */
    usleep(10000);  /* 10ms */
    
    /*
    ** Read calibration data
    */
    if (FSWV1_ReadCalibrationData() != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to read calibration data");
        close(I2C_Fd);
        I2C_Fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    OS_printf("FSWV1: Sensor initialized successfully\n");
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Read calibration data from sensor                                      */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 FSWV1_ReadCalibrationData(void)
{
    uint8 calib[24];
    
    /* Read temperature calibration (6 bytes from 0x88) */
    if (FSWV1_ReadReg(0x88, calib, 6) != CFE_SUCCESS)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    CalibData.dig_T1 = (calib[1] << 8) | calib[0];
    CalibData.dig_T2 = (calib[3] << 8) | calib[2];
    CalibData.dig_T3 = (calib[5] << 8) | calib[4];
    
    /* Read pressure calibration (18 bytes from 0x8E) */
    if (FSWV1_ReadReg(0x8E, calib, 18) != CFE_SUCCESS)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    CalibData.dig_P1 = (calib[1] << 8) | calib[0];
    CalibData.dig_P2 = (calib[3] << 8) | calib[2];
    CalibData.dig_P3 = (calib[5] << 8) | calib[4];
    CalibData.dig_P4 = (calib[7] << 8) | calib[6];
    CalibData.dig_P5 = (calib[9] << 8) | calib[8];
    CalibData.dig_P6 = (calib[11] << 8) | calib[10];
    CalibData.dig_P7 = (calib[13] << 8) | calib[12];
    CalibData.dig_P8 = (calib[15] << 8) | calib[14];
    CalibData.dig_P9 = (calib[17] << 8) | calib[16];

    OS_printf("FSWV1: Calibration data loaded\n");
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Read sensor data                                                        */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_ReadSensor(FSWV1_SensorData_t *Data)
{
    uint8 data[6];
    int32 adc_T, adc_P;
    int32 temp;
    uint32 press;
    
    if (Data == NULL)
    {
        return CFE_ES_BAD_ARGUMENT;
    }

    if (I2C_Fd < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Sensor not initialized");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    /*
    ** Read pressure and temperature data (6 bytes from 0xF7)
    ** data[0-2]: pressure (MSB, LSB, XLSB)
    ** data[3-5]: temperature (MSB, LSB, XLSB)
    */
    if (FSWV1_ReadReg(FSWV1_REG_PRESS_MSB, data, 6) != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(FSWV1_APP_SENSOR_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1: Failed to read sensor data");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Combine raw ADC values (20-bit) */
    adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    
    /* Compensate temperature (must be done first to calculate t_fine) */
    temp = FSWV1_CompensateTemperature(adc_T);
    Data->Temperature = temp / 100.0f;  /* Convert to °C */

    /* Compensate pressure (uses t_fine from temperature calculation) */
    press = FSWV1_CompensatePressure(adc_P);
    Data->Pressure = press / 25600.0f;  /* Convert to hPa */

    /* Get timestamp */
    CFE_TIME_SysTime_t time = CFE_TIME_GetTime();
    Data->Timestamp = time.Seconds;

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Compensate temperature (from FSWV1 datasheet)                         */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 FSWV1_CompensateTemperature(int32 adc_T)
{
    int32 var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32)CalibData.dig_T1 << 1))) * 
            ((int32)CalibData.dig_T2)) >> 11;
    
    var2 = (((((adc_T >> 4) - ((int32)CalibData.dig_T1)) * 
             ((adc_T >> 4) - ((int32)CalibData.dig_T1))) >> 12) *
            ((int32)CalibData.dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    
    return T;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Compensate pressure (from FSWV1 datasheet)                            */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static uint32 FSWV1_CompensatePressure(int32 adc_P)
{
    int64 var1, var2, p;

    var1 = ((int64)t_fine) - 128000;
    var2 = var1 * var1 * (int64)CalibData.dig_P6;
    var2 = var2 + ((var1 * (int64)CalibData.dig_P5) << 17);
    var2 = var2 + (((int64)CalibData.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64)CalibData.dig_P3) >> 8) +
           ((var1 * (int64)CalibData.dig_P2) << 12);
    var1 = (((((int64)1) << 47) + var1)) * ((int64)CalibData.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0; /* Avoid division by zero */
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64)CalibData.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64)CalibData.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64)CalibData.dig_P7) << 4);

    return (uint32)p;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Close sensor                                                            */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_CloseSensor(void)
{
    if (I2C_Fd >= 0)
    {
        close(I2C_Fd);
        I2C_Fd = -1;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialize UDP socket                                                   */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_InitUDP(void)
{
    int32 status;
    OS_SockAddr_t addr;

    /*
    ** Create UDP socket
    */
    status = OS_SocketOpen(&FSWV1_APP_Data.UdpSocket, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
    if (status != OS_SUCCESS)
    {
        OS_printf("FSWV1: Failed to create UDP socket, RC = %d\n", (int)status);
        return status;
    }

    /*
    ** Set up destination address
    ** Note: In a real system, you might want to make this configurable
    */
    status = OS_SocketAddrInit(&addr, OS_SocketDomain_INET);
    if (status != OS_SUCCESS)
    {
        OS_printf("FSWV1: Failed to init socket address, RC = %d\n", (int)status);
        return status;
    }

    status = OS_SocketAddrSetPort(&addr, FSWV1_UDP_PORT);
    if (status != OS_SUCCESS)
    {
        OS_printf("FSWV1: Failed to set socket port, RC = %d\n", (int)status);
        return status;
    }

    status = OS_SocketAddrFromString(&addr, FSWV1_UDP_DEST_IP);
    if (status != OS_SUCCESS)
    {
        OS_printf("FSWV1: Failed to set socket address, RC = %d\n", (int)status);
        return status;
    }

    OS_printf("FSWV1: UDP socket initialized (dest: %s:%d)\n", 
             FSWV1_UDP_DEST_IP, FSWV1_UDP_PORT);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Send data via UDP (CCSDS packet format)                                */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_SendUDP(const FSWV1_SensorData_t *SensorData, const FSWV1_IMUData_t *IMUData)
{
    int32 status;
    OS_SockAddr_t addr;
    
    if (!OS_ObjectIdDefined(FSWV1_APP_Data.UdpSocket))
    {
        return CFE_SUCCESS; /* Socket not initialized, skip */
    }

    /*
    ** Send the actual CCSDS telemetry packet
    ** The CombinedTlm structure already has the proper CCSDS header
    ** This is what ground systems expect to receive
    */
    
    /* Set up destination address */
    OS_SocketAddrInit(&addr, OS_SocketDomain_INET);
    OS_SocketAddrSetPort(&addr, FSWV1_UDP_PORT);
    OS_SocketAddrFromString(&addr, FSWV1_UDP_DEST_IP);

    /*
    ** Send the complete CCSDS telemetry packet
    ** This includes:
    ** - CCSDS Primary Header (6 bytes)
    ** - CCSDS Secondary Header (timestamp, etc.)
    ** - Telemetry Payload (Temperature, Pressure, Timestamp)
    */
    status = OS_SocketSendTo(FSWV1_APP_Data.UdpSocket, 
                            &FSWV1_APP_Data.CombinedTlm,
                            sizeof(FSWV1_APP_Data.CombinedTlm), 
                            &addr);
    
    if (status < 0)
    {
        /* Don't spam errors - UDP is best effort */
        static uint32 error_count = 0;
        if (error_count % 100 == 0)
        {
            OS_printf("FSWV1: UDP send error, RC = %d\n", (int)status);
        }
        error_count++;
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Close UDP socket                                                        */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_CloseUDP(void)
{
    if (OS_ObjectIdDefined(FSWV1_APP_Data.UdpSocket))
    {
        OS_close(FSWV1_APP_Data.UdpSocket);
        FSWV1_APP_Data.UdpSocket = OS_OBJECT_ID_UNDEFINED;
    }
}
