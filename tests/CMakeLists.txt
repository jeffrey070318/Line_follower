cmake_minimum_required(VERSION 3.22)
project(line_follower_tests C)
enable_testing()
set(CMAKE_C_STANDARD 11)
set(CAR "${CMAKE_CURRENT_LIST_DIR}/../Car")
set(COMMON_SOURCES
    ${CAR}/Bsp/bsp_gpio.c
    ${CAR}/Bsp/bsp_adc.c
    ${CAR}/Devices/Motor/motor.c
    ${CAR}/Devices/Grayscale/grayscale.c
    ${CAR}/CarTask/chassis/line_control.c
)
foreach(test IN ITEMS car chassis)
    add_executable(test_${test} test_${test}.c ${COMMON_SOURCES})
    target_include_directories(test_${test} PRIVATE
        stubs ${CAR}/Bsp ${CAR}/Devices/Motor ${CAR}/Devices/Grayscale
        ${CAR}/CarTask ${CAR}/CarTask/chassis)
    target_compile_options(test_${test} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME ${test} COMMAND test_${test})
endforeach()
target_sources(test_chassis PRIVATE ${CAR}/CarTask/chassis/chassis_task.c
    ${CAR}/CarTask/key/key_task.c ${CAR}/Devices/Key/key.c
    ${CAR}/CarTask/led/led_task.c ${CAR}/Devices/BlindLED/led.c)
target_include_directories(test_chassis PRIVATE ${CAR}/CarTask/key ${CAR}/Devices/Key
    ${CAR}/CarTask/led ${CAR}/Devices/BlindLED)
add_executable(test_key test_key.c ${CAR}/Bsp/bsp_gpio.c ${CAR}/Devices/Key/key.c)
target_include_directories(test_key PRIVATE stubs ${CAR}/Bsp ${CAR}/Devices/Key)
target_compile_options(test_key PRIVATE -Wall -Wextra -Werror)
add_test(NAME key COMMAND test_key)
add_executable(test_led test_led.c ${CAR}/Bsp/bsp_gpio.c ${CAR}/Devices/BlindLED/led.c)
target_include_directories(test_led PRIVATE stubs ${CAR}/Bsp ${CAR}/Devices/BlindLED)
target_compile_options(test_led PRIVATE -Wall -Wextra -Werror)
add_test(NAME led COMMAND test_led)
