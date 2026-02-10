##############################################################################
## File: CMakeLists.txt
##
## Purpose:
##   CMake build script for FSWV1 Application
##
##############################################################################

project(FSWV1_APP C)

# Create the app module
add_cfe_app(fswv1 
    fsw/src/fswv1_app.c
    fsw/src/fswv1_sensor.c
    fsw/src/fswv1_gpio.c
    fsw/src/fswv1_uart.c
    fsw/src/fswv1_uart_telemetry.c
)

# Add EDS support for message definitions
add_cfe_app_dependency(fswv1 sample_lib)

# Specify include directories
target_include_directories(fswv1 PUBLIC 
    fsw/inc
)

# Link against libgpiod for GPIO control
target_link_libraries(fswv1 gpiod)

##############################################################################
# Installation
##############################################################################

# Install configuration files (if any)
# install(FILES config/fswv1_config.tbl DESTINATION cpu1/cf)

##############################################################################
# Unit Tests (optional)
##############################################################################

# Uncomment to add unit tests
# if(ENABLE_UNIT_TESTS)
#     add_subdirectory(unit-test)
# endif()
