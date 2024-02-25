#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
    namespace mbishi {
        // Temperature
        const uint8_t MBISHI_TEMP_MIN = 18; // Celsius
        const uint8_t MBISHI_TEMP_MAX = 30; // Celsius

        class MbishiClimate : public climate_ir::ClimateIR {
            public:
                MbishiClimate() : climate_ir::ClimateIR(
                    MBISHI_TEMP_MIN, MBISHI_TEMP_MAX, 1.0f, true, true,
                    std::set<climate::ClimateFanMode>{
                        climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                        climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH,
                        climate::CLIMATE_FAN_MIDDLE, climate::CLIMATE_FAN_FOCUS,
                        climate::CLIMATE_FAN_DIFFUSE
                    },
                    std::set<climate::ClimateSwingMode>{
                        climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                        climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH
                    }
                ) {}

            protected:
                void transmit_state() override;
                /// Handle received IR Buffer
                bool on_receive(remote_base::RemoteReceiveData data) override;

        };
    } // namespace mbishi
} // namespace esphome
