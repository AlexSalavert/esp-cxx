if(CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2)
    set(SLAVE_SRC "src/i2c_slave.cpp")
else()
    set(SLAVE_SRC "")
endif()

idf_component_register(
    SRCS
        "src/i2c_master.cpp"
        "src/i2c_device.cpp"
        ${SLAVE_SRC}
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        driver
)