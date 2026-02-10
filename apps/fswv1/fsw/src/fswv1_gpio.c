/******************************************************************************
** File: fswv1_gpio.c
**
** Purpose:
**   This file contains GPIO functions for LED control on Raspberry Pi 5.
**   Uses libgpiod v2.x API properly.
**
** Requirements:
**   - libgpiod v2.0+ (apt install libgpiod-dev)
**
******************************************************************************/

#include "fswv1_app.h"
#include <gpiod.h>
#include <string.h>
#include <stdio.h>

/*
** GPIO Configuration
*/
#define LED_GPIO_PIN 17
#define GPIO_CHIP_PATH "/dev/gpiochip4"  // Raspberry Pi 5

/*
** Static variables
*/
static bool GPIO_Initialized = false;
static bool LED_State = false;
static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *request = NULL;
static unsigned int offset = LED_GPIO_PIN;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Initialize GPIO for LED control using libgpiod v2.x                    */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_InitGPIO(void)
{
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *config = NULL;
    struct gpiod_request_config *req_cfg = NULL;
    int ret;
    
    if (GPIO_Initialized)
    {
        return CFE_SUCCESS;
    }
    
    OS_printf("FSWV1_GPIO: Initializing GPIO %d using libgpiod v2...\n", LED_GPIO_PIN);
    
    /* Open GPIO chip */
    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip)
    {
        /* Try gpiochip0 as fallback */
        chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip)
        {
            CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                             "FSWV1_GPIO: Failed to open GPIO chip");
            return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
        OS_printf("FSWV1_GPIO: Opened /dev/gpiochip0\n");
    }
    else
    {
        OS_printf("FSWV1_GPIO: Opened /dev/gpiochip4\n");
    }
    
    /* Create line settings */
    settings = gpiod_line_settings_new();
    if (!settings)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to create line settings");
        gpiod_chip_close(chip);
        chip = NULL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Configure as output, active-high, initially low */
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    
    /* Create line config */
    config = gpiod_line_config_new();
    if (!config)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to create line config");
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        chip = NULL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Add settings for our GPIO line */
    ret = gpiod_line_config_add_line_settings(config, &offset, 1, settings);
    if (ret)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to add line settings");
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        chip = NULL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Create request config */
    req_cfg = gpiod_request_config_new();
    if (!req_cfg)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to create request config");
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        chip = NULL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    gpiod_request_config_set_consumer(req_cfg, "fswv1_cfs_app");
    
    /* Request the lines */
    request = gpiod_chip_request_lines(chip, req_cfg, config);
    
    /* Clean up temporary objects */
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    
    if (!request)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to request GPIO line %d", LED_GPIO_PIN);
        gpiod_chip_close(chip);
        chip = NULL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    GPIO_Initialized = true;
    LED_State = false;
    
    CFE_EVS_SendEvent(FSWV1_APP_GPIO_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                     "FSWV1_GPIO: GPIO %d initialized successfully using libgpiod v2", 
                     LED_GPIO_PIN);
    
    OS_printf("FSWV1_GPIO: GPIO %d ready, LED initialized to OFF\n", LED_GPIO_PIN);
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Set LED state                                                           */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_SetLED(bool state)
{
    enum gpiod_line_value value;
    int ret;
    
    if (!GPIO_Initialized || !request)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: GPIO not initialized");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    /* Convert bool to gpiod value */
    value = state ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
    
    /* Set the output value */
    ret = gpiod_line_request_set_value(request, offset, value);
    if (ret < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to set LED state");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    LED_State = state;
    
    OS_printf("FSWV1_GPIO: LED turned %s\n", state ? "ON" : "OFF");
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Get LED state                                                           */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_GetLED(bool *state)
{
    enum gpiod_line_value value;
    int ret;
    
    if (!GPIO_Initialized || !request)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: GPIO not initialized");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    if (state == NULL)
    {
        return OS_INVALID_POINTER;
    }
    
    /* Get the current value */
    ret = gpiod_line_request_get_value(request, offset);
    if (ret < 0)
    {
        CFE_EVS_SendEvent(FSWV1_APP_GPIO_ERR_EID, CFE_EVS_EventType_ERROR,
                         "FSWV1_GPIO: Failed to read LED state");
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    value = (enum gpiod_line_value)ret;
    *state = (value == GPIOD_LINE_VALUE_ACTIVE);
    LED_State = *state;
    
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Toggle LED state                                                        */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 FSWV1_ToggleLED(void)
{
    int32 status;
    bool current_state;
    
    /* Read current state */
    status = FSWV1_GetLED(&current_state);
    if (status != CFE_SUCCESS)
    {
        return status;
    }
    
    /* Toggle to opposite state */
    return FSWV1_SetLED(!current_state);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* Close GPIO (cleanup)                                                    */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void FSWV1_CloseGPIO(void)
{
    if (!GPIO_Initialized)
    {
        return;
    }
    
    /* Turn off LED before cleanup */
    if (request)
    {
        FSWV1_SetLED(false);
        
        /* Release the line request */
        gpiod_line_request_release(request);
        request = NULL;
    }
    
    /* Close the chip */
    if (chip)
    {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    
    GPIO_Initialized = false;
    LED_State = false;
    
    OS_printf("FSWV1_GPIO: GPIO cleanup complete\n");
}
