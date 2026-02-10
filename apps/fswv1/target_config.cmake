##########################################################################
#
# FSWV1 Application - Target Configuration
#
# This file should be placed in: 
#   cfs/sample_defs/targets/[your_target]/inc/target_config.cmake
# OR
#   cfs/sample_defs/cpu1_fswv1_target.cmake
#
##########################################################################

# Set the application list for this target
# Add fswv1 to the list of applications to build

set(TGT1_APPLIST
    # Core cFS applications
    sch_lab
    to_lab
    ci_lab
    sample_app
    
    # Custom applications
    fswv1          # Add this line for FSWV1 app
)

# Alternative if using APP_LIST structure:
# list(APPEND TGT_APP_LIST fswv1)

##########################################################################
# FSWV1 Specific Configuration
##########################################################################

# If you need to set specific compiler flags for FSWV1
# target_compile_options(fswv1 PRIVATE -Wall -Wextra)

# If you need to add include directories
# target_include_directories(fswv1 PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/fswv1/fsw/inc
# )

# If you need to link additional libraries
# target_link_libraries(fswv1 gpiod)  # Already in CMakeLists.txt

##########################################################################
# UART Device Configuration (Optional Override)
##########################################################################

# You can override UART devices at compile time using compiler definitions
# Uncomment and modify as needed:

# Override IMU UART device
# target_compile_definitions(fswv1 PRIVATE 
#     UART_DEVICE="/dev/ttyAMA0"
# )

# Override Telemetry UART device  
# target_compile_definitions(fswv1 PRIVATE 
#     TELEMETRY_UART_DEVICE="/dev/ttyUSB0"
# )

# Override both
# target_compile_definitions(fswv1 PRIVATE 
#     UART_DEVICE="/dev/ttyAMA0"
#     TELEMETRY_UART_DEVICE="/dev/ttyUSB0"
# )

##########################################################################
# Build Configuration
##########################################################################

# Enable position independent code
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set C standard (if needed)
# set(CMAKE_C_STANDARD 99)

##########################################################################
# Installation Rules
##########################################################################

# Install application to target directory
# install(TARGETS fswv1 DESTINATION cpu1/cf)

# Install any configuration files
# install(FILES config/fswv1_config.tbl DESTINATION cpu1/cf)

##########################################################################
# Module Dependencies
##########################################################################

# FSWV1 requires libgpiod for GPIO control
# This is already specified in fswv1/CMakeLists.txt:
#   target_link_libraries(fswv1 gpiod)

# Make sure libgpiod is installed on target system:
#   sudo apt-get install libgpiod-dev

##########################################################################
# Notes
##########################################################################

# 1. UART Devices:
#    - IMU Input:       /dev/ttyAMA0 (default)
#    - Telemetry Output: /dev/ttyS0 (default, may conflict!)
#    
#    Recommended: Change telemetry to /dev/ttyUSB0 or /dev/ttyAMA1
#    Edit: fswv1/fsw/src/fswv1_uart_telemetry.c line 29

# 2. Required System Libraries:
#    - libgpiod (for LED control)
#    - termios (standard Linux, for UART)
#
# 3. Required Permissions:
#    - User must be in 'dialout' group for UART access
#    - GPIO access via libgpiod (no root needed)
#
# 4. UART Conflicts:
#    - ttyAMA0 and ttyS0 share GPIO pins 14/15 on Raspberry Pi
#    - Use USB adapter (/dev/ttyUSB0) for telemetry to avoid conflict
#    - OR enable UART1 (ttyAMA1) on GPIO 0/1

##########################################################################
