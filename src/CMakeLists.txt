add_executable(multicore_runner_queue
        multicore_runner_queue.c
        can2040.c
        )

target_link_libraries(multicore_runner_queue
        pico_multicore
        pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(multicore_runner_queue 1)
pico_enable_stdio_uart(multicore_runner_queue 0)

target_include_directories(multicore_runner_queue PRIVATE ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include/hardware/regs/)
target_include_directories(multicore_runner_queue PRIVATE ${PICO_SDK_PATH}/src/rp2040/)
#target_include_directories(multicore_runner_queue PRIVATE ${PICO_SDK_PATH}/src/rp2_common/cmsis/stub/CMSIS/Device/RaspberryPi/RP2040/Include)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_runner_queue)

# add url via pico_set_program_url
example_auto_set_url(multicore_runner_queue)