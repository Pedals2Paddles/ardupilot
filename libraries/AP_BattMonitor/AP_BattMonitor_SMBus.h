#pragma once

#include <AP_Common/AP_Common.h>
#include <AP_Param/AP_Param.h>
#include <AP_Math/AP_Math.h>
#include "AP_BattMonitor_Backend.h"
#include <utility>

#define AP_BATTMONITOR_SMBUS_BUS_INTERNAL           0
#define AP_BATTMONITOR_SMBUS_BUS_EXTERNAL           1
#define AP_BATTMONITOR_SMBUS_I2C_ADDR               0x0B
#define AP_BATTMONITOR_SMBUS_TIMEOUT_MICROS         5000000 // sensor becomes unhealthy if no successful readings for 5 seconds

class AP_BattMonitor_SMBus : public AP_BattMonitor_Backend
{
public:

    /// Constructor
    AP_BattMonitor_SMBus(AP_BattMonitor &mon, uint8_t instance,
                    AP_BattMonitor::BattMonitor_State &mon_state,
                    AP_HAL::OwnPtr<AP_HAL::I2CDevice> dev)
        : AP_BattMonitor_Backend(mon, instance, mon_state),
          _dev(std::move(dev))
    {}

    // virtual destructor to reduce compiler warnings
    virtual ~AP_BattMonitor_SMBus() {}


protected:

<<<<<<< HEAD
=======
    void read(void);

    // reads the pack full charge capacity
    // returns true if the read was successful, or if we already knew the pack capacity
    bool read_full_charge_capacity(void);

>>>>>>> 5ba5908dd... AP_BattMonitor: SMBus: Fetch pack capacity
    // reads the temperature word from the battery
    // returns true if the read was successful
    bool read_temp(void);

     // read word from register
     // returns true if read was successful, false if failed
    bool read_word(uint8_t reg, uint16_t& data) const;

    // get_PEC - calculate PEC for a read or write from the battery
    // buff is the data that was read or will be written
    uint8_t get_PEC(const uint8_t i2c_addr, uint8_t cmd, bool reading, const uint8_t buff[], uint8_t len) const;

    AP_HAL::OwnPtr<AP_HAL::I2CDevice> _dev;
    bool _pec_supported; // true if PEC is supported

<<<<<<< HEAD
=======
    uint16_t _serial_number;        // battery serial number
    uint16_t _full_charge_capacity; // full charge capacity, used to stash the value before setting the parameter

>>>>>>> 5ba5908dd... AP_BattMonitor: SMBus: Fetch pack capacity
};

// include specific implementations
#include "AP_BattMonitor_SMBus_Solo.h"
#include "AP_BattMonitor_SMBus_Maxell.h"
