/******************************************************************************
** File: fswv1_uart_telemetry.c
**
** Purpose:
**   This file contains UART interface functions for transmitting telemetry data.
**   Sends the same telemetry data that is sent via UDP to a UART port.
**
** Output Format:
**   Binary CCSDS packet format (same as UDP) OR
**   ASCII format for easier debugging: "BMP:T=%.2f,P=%.2f IMU:Ax=%.2f,Ay=%.2f,Az=%.2f,Gx=%.2f,Gy=%.2f,Gz=%.2f,T=%.2f TS=%u\n"
**
** Note: This uses a different UART than the IMU input UART to avoid conflicts.
**
******************************************************************************/

#include "fswv1_app.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*
** UART Configuration for Telemetry Output
** This uses a different UART than the IMU input (which uses /dev/ttyAMA0)
** Options:
** - /dev/ttyAMA1 - Secondary UART on Raspberry Pi (if available)
** - /dev/ttyS0   - Alternative UART
** - /dev/ttyUSB0 - USB-to-Serial adapter
** - /dev/ttyUSB1 - Another USB-to-Serial adapter
*/
#define TELEMETRY_UART_DEVICE "/dev/ttyUSB0"  /* Change this to match your hardware */
#define TELEMETRY_UART_BAUDRATE B115200
#define TELEMETRY_ASCII_FORMAT 0  /* Set to 1 for ASCII format, 0 for binary CCSDS format */

/*
** Static variables
*/
static bool TelemetryUART_Initialized = false;
static int telemetry_uart_fd = -1;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialize UART for Telemetry Transmission                             */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_InitTelemetryUART(void)
{
    struct termios tty;
    
    if (TelemetryUART_Initialized)
    {
        return CFE_SUCCESS;
    }
    
    OS_printf("FSWV1_TELEMETRY_UART: Initializing telemetry UART on %s at 115200 baud...\n", 
              TELEMETRY_UART_DEVICE);
    
    /* Open UART device */
    telemetry_uart_fd = open(TELEMETRY_UART_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (telemetry_uart_fd < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_TELEMETRY_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_TELEMETRY_UART: Failed to open %s: %s", 
                         TELEMETRY_UART_DEVICE, strerror(errno));
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Get current settings */
    if (tcgetattr(telemetry_uart_fd, &tty) != 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_TELEMETRY_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_TELEMETRY_UART: Failed to get UART attributes");
        close(telemetry_uart_fd);
        telemetry_uart_fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Configure UART settings */
    cfsetospeed(&tty, TELEMETRY_UART_BAUDRATE);
    cfsetispeed(&tty, TELEMETRY_UART_BAUDRATE);
    
    /* 8N1 mode (8 data bits, no parity, 1 stop bit) */
    tty.c_cflag &= ~PARENB;        /* No parity */
    tty.c_cflag &= ~CSTOPB;        /* 1 stop bit */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            /* 8 data bits */
    tty.c_cflag &= ~0020000000000; /* No hardware flow control (CRTSCTS) */
    tty.c_cflag |= CREAD | CLOCAL; /* Enable receiver, ignore modem lines */
    
    /* Raw input mode (for compatibility) */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    /* Raw output mode */
    tty.c_oflag &= ~OPOST;
    
    /* Set up for blocking write (telemetry transmission) */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;  /* 0.5 second timeout for write operations */
    
    /* Apply settings */
    if (tcsetattr(telemetry_uart_fd, TCSANOW, &tty) != 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_TELEMETRY_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_TELEMETRY_UART: Failed to set UART attributes");
        close(telemetry_uart_fd);
        telemetry_uart_fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Flush any existing data */
    tcflush(telemetry_uart_fd, TCIOFLUSH);
    
    TelemetryUART_Initialized = true;
    
    CFE_EVS_SendEvent(FSWV1_APP_UART_TELEMETRY_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1_TELEMETRY_UART: Telemetry UART initialized on %s at 115200 baud", 
                     TELEMETRY_UART_DEVICE);
    
    OS_printf("FSWV1_TELEMETRY_UART: Ready to transmit telemetry data\n");
    
    return CFE_SUCCESS;
}

#if TELEMETRY_ASCII_FORMAT
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Send Telemetry Data via UART (ASCII Format)                            */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 SendTelemetryASCII(const FSWV1_SensorData_t *SensorData, 
                                 const FSWV1_IMUData_t *IMUData)
{
    char buffer[512];
    int len;
    ssize_t bytes_written;
    
    /* Format telemetry data as ASCII string */
    len = snprintf(buffer, sizeof(buffer),
                   "BMP:T=%.2f,P=%.2f IMU:Ax=%.2f,Ay=%.2f,Az=%.2f,Gx=%.2f,Gy=%.2f,Gz=%.2f,T=%.2f TS=%u\n",
                   SensorData->Temperature,
                   SensorData->Pressure,
                   IMUData->Accel_X,
                   IMUData->Accel_Y,
                   IMUData->Accel_Z,
                   IMUData->Gyro_X,
                   IMUData->Gyro_Y,
                   IMUData->Gyro_Z,
                   IMUData->Temperature,
                   FSWV1_APP_Data.CombinedTlm.Payload.Timestamp);
    
    if (len < 0 || len >= sizeof(buffer))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Write to UART */
    bytes_written = write(telemetry_uart_fd, buffer, len);
    
    if (bytes_written != len)
    {
        /* Don't spam errors for every failed write */
        static uint32 error_count = 0;
        if (error_count % 100 == 0)
        {
            OS_printf("FSWV1_TELEMETRY_UART: Write error, expected %d bytes, wrote %d bytes\n",
                     len, (int)bytes_written);
        }
        error_count++;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    return CFE_SUCCESS;
}
#endif

#if !TELEMETRY_ASCII_FORMAT
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Send Telemetry Data via UART (Binary CCSDS Format)                     */
/* Converts payload to big-endian (network byte order)                    */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Helper function to swap float bytes to big-endian */
static void swap_float_to_be(float *val)
{
    uint32_t *p = (uint32_t *)val;
    *p = __builtin_bswap32(*p);
}

static void swap_uint32_to_be(uint32_t *val)
{
    *val = __builtin_bswap32(*val);
}

static int32 SendTelemetryBinary(void)
{
    ssize_t bytes_written;
    FSWV1_APP_CombinedTlm_t packet_copy;
    size_t packet_size = sizeof(FSWV1_APP_Data.CombinedTlm);
    
    /* Copy the packet to avoid modifying the original */
    memcpy(&packet_copy, &FSWV1_APP_Data.CombinedTlm, packet_size);
    
    /* CCSDS header is already in big-endian from CFE_SB_TransmitMsg
       but the payload floats need manual conversion on little-endian systems */
    
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    /* Convert all float fields to big-endian */
    swap_float_to_be(&packet_copy.Payload.BMP_Temperature);
    swap_float_to_be(&packet_copy.Payload.BMP_Pressure);
    swap_float_to_be(&packet_copy.Payload.Accel_X);
    swap_float_to_be(&packet_copy.Payload.Accel_Y);
    swap_float_to_be(&packet_copy.Payload.Accel_Z);
    swap_float_to_be(&packet_copy.Payload.Gyro_X);
    swap_float_to_be(&packet_copy.Payload.Gyro_Y);
    swap_float_to_be(&packet_copy.Payload.Gyro_Z);
    swap_float_to_be(&packet_copy.Payload.IMU_Temperature);
    swap_uint32_to_be(&packet_copy.Payload.Timestamp);
#endif
    
    /* Write the byte-swapped CCSDS packet to UART */
    bytes_written = write(telemetry_uart_fd, &packet_copy, packet_size);
    
    if (bytes_written != (ssize_t)packet_size)
    {
        static uint32 error_count = 0;
        if (error_count % 100 == 0)
        {
            OS_printf("FSWV1_TELEMETRY_UART: Write error, expected %zu bytes, wrote %d bytes\n",
                     packet_size, (int)bytes_written);
        }
        error_count++;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    return CFE_SUCCESS;
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Send Telemetry Data via UART (Public Interface)                        */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_SendTelemetryUART(const FSWV1_SensorData_t *SensorData, 
                               const FSWV1_IMUData_t *IMUData)
{
    if (!TelemetryUART_Initialized || telemetry_uart_fd < 0)
    {
        return CFE_SUCCESS; /* UART not initialized, skip silently */
    }
    
    if (SensorData == NULL || IMUData == NULL)
    {
        return OS_INVALID_POINTER;
    }
    
#if TELEMETRY_ASCII_FORMAT
    /* Send ASCII formatted data */
    return SendTelemetryASCII(SensorData, IMUData);
#else
    /* Send binary CCSDS formatted data */
    return SendTelemetryBinary();
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Close Telemetry UART (cleanup)                                          */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_CloseTelemetryUART(void)
{
    if (!TelemetryUART_Initialized)
    {
        return;
    }
    
    if (telemetry_uart_fd >= 0)
    {
        close(telemetry_uart_fd);
        telemetry_uart_fd = -1;
    }
    
    TelemetryUART_Initialized = false;
    
    OS_printf("FSWV1_TELEMETRY_UART: Telemetry UART closed\n");
}
