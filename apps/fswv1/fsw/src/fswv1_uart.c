/******************************************************************************
** File: fswv1_uart.c
**
** Purpose:
**   This file contains UART interface functions for receiving IMU data.
**   Receives formatted string: "$,Ax,Ay,Az,Gx,Gy,Gz,Temperature,#"
**
** Expected Format:
**   "$,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#"
**   - Starts with '$'
**   - Ends with '#'
**   - 7 float values separated by commas
**   - Ax, Ay, Az: Accelerometer (m/s^2 or g)
**   - Gx, Gy, Gz: Gyroscope (deg/s or rad/s)
**   - Temperature: degrees Celsius
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
** UART Configuration
** Change UART_DEVICE to match your setup:
** - /dev/ttyAMA0 - Primary UART (GPIO 14/15) on Raspberry Pi
** - /dev/ttyS0   - Alternative UART
** - /dev/ttyUSB0 - USB-to-Serial adapter
*/
#define UART_DEVICE "/dev/ttyAMA0"  
#define UART_BAUDRATE B115200        
#define UART_BUFFER_SIZE 256

/*
** Static variables
*/
static bool UART_Initialized = false;
static int uart_fd = -1;
static char uart_buffer[UART_BUFFER_SIZE];
static int buffer_pos = 0;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialize UART for IMU data reception                                 */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_InitUART(void)
{
    struct termios tty;
    
    if (UART_Initialized)
    {
        return CFE_SUCCESS;
    }
    
    OS_printf("FSWV1_UART: Initializing UART on %s at 115200 baud...\n", UART_DEVICE);
    
    /* Open UART device */
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uart_fd < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_UART: Failed to open %s: %s", UART_DEVICE, strerror(errno));
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Get current settings */
    if (tcgetattr(uart_fd, &tty) != 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_UART: Failed to get UART attributes");
        close(uart_fd);
        uart_fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Configure UART settings */
    cfsetospeed(&tty, UART_BAUDRATE);
    cfsetispeed(&tty, UART_BAUDRATE);
    
    /* 8N1 mode (8 data bits, no parity, 1 stop bit) */
    tty.c_cflag &= ~PARENB;        /* No parity */
    tty.c_cflag &= ~CSTOPB;        /* 1 stop bit */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            /* 8 data bits */
    tty.c_cflag &= ~0020000000000;  /* CRTSCTS - No hardware flow control */       /* No hardware flow control */
    tty.c_cflag |= CREAD | CLOCAL; /* Enable receiver, ignore modem lines */
    
    /* Raw input mode */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    /* Raw output mode */
    tty.c_oflag &= ~OPOST;
    
    /* Non-blocking read with timeout */
    tty.c_cc[VMIN] = 0;   /* Non-blocking */
    tty.c_cc[VTIME] = 1;  /* 0.1 second timeout */
    
    /* Apply settings */
    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_UART_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_UART: Failed to set UART attributes");
        close(uart_fd);
        uart_fd = -1;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Flush any existing data */
    tcflush(uart_fd, TCIOFLUSH);
    
    buffer_pos = 0;
    UART_Initialized = true;
    
    CFE_EVS_SendEvent(FSWV1_APP_UART_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1_UART: UART initialized on %s at 115200 baud", UART_DEVICE);
    
    OS_printf("FSWV1_UART: Ready to receive IMU data\n");
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Parse IMU data from formatted string                                   */
/* Format: "$,Ax,Ay,Az,Gx,Gy,Gz,Temperature,#"                           */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int32 ParseIMUData(const char *str, FSWV1_IMUData_t *data)
{
    int parsed;
    
    /* Parse the format: $,Ax,Ay,Az,Gx,Gy,Gz,Temp,# */
    parsed = sscanf(str, "$,%f,%f,%f,%f,%f,%f,%f,#",
                    &data->Accel_X,
                    &data->Accel_Y,
                    &data->Accel_Z,
                    &data->Gyro_X,
                    &data->Gyro_Y,
                    &data->Gyro_Z,
                    &data->Temperature);
    
    if (parsed != 7)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Update timestamp */
    data->Timestamp = CFE_TIME_GetTime().Seconds;
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Read and parse IMU data from UART                                      */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_ReadUART(FSWV1_IMUData_t *data)
{
    char byte;
    ssize_t bytes_read;
    
    if (!UART_Initialized || uart_fd < 0)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    if (data == NULL)
    {
        return OS_INVALID_POINTER;
    }
    
    /* Read available bytes */
    while ((bytes_read = read(uart_fd, &byte, 1)) > 0)
    {
        /* Look for start marker '$' */
        if (byte == '$')
        {
            buffer_pos = 0;
            uart_buffer[buffer_pos++] = byte;
        }
        /* Look for end marker '#' */
        else if (byte == '#')
        {
            if (buffer_pos > 0 && buffer_pos < UART_BUFFER_SIZE - 1)
            {
                uart_buffer[buffer_pos++] = byte;
                uart_buffer[buffer_pos] = '\0';
                
                /* Parse the complete message */
                if (ParseIMUData(uart_buffer, data) == CFE_SUCCESS)
                {
                    buffer_pos = 0;
                    return CFE_SUCCESS;
                }
                else
                {
                    /* Parse failed, reset buffer */
                    buffer_pos = 0;
                }
            }
            else
            {
                /* Buffer overflow or invalid state, reset */
                buffer_pos = 0;
            }
        }
        /* Accumulate data between $ and # */
        else if (buffer_pos > 0 && buffer_pos < UART_BUFFER_SIZE - 1)
        {
            uart_buffer[buffer_pos++] = byte;
        }
        /* Buffer overflow protection */
        else if (buffer_pos >= UART_BUFFER_SIZE - 1)
        {
            buffer_pos = 0;  /* Reset on overflow */
        }
    }
    
    /* No complete message received yet */
    return OS_ERROR;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Close UART (cleanup)                                                    */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_CloseUART(void)
{
    if (!UART_Initialized)
    {
        return;
    }
    
    if (uart_fd >= 0)
    {
        close(uart_fd);
        uart_fd = -1;
    }
    
    UART_Initialized = false;
    buffer_pos = 0;
    
    OS_printf("FSWV1_UART: UART closed\n");
}
