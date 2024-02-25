#include "mbishi.h"
#include "esphome/core/log.h"

namespace esphome {
    namespace mbishi {
        static const char *TAG = "mbishi.climate";

        // Power
        const uint32_t MBISHI_OFF = 0x08;
        const uint32_t MBISHI_ON = 0x00;

        // Operating mode
        const uint8_t MBISHI_AUTO = 0x07;
        const uint8_t MBISHI_HEAT = 0x03;
        const uint8_t MBISHI_COOL = 0x06;
        const uint8_t MBISHI_DRY = 0x05;
        const uint8_t MBISHI_FAN = 0x04;

        // Fan speed
        const uint8_t MBISHI_FAN_AUTO = 0x0F;
        const uint8_t MBISHI_FAN1 = 0x0E;
        const uint8_t MBISHI_FAN2 = 0x0D;
        const uint8_t MBISHI_FAN3 = 0x0C;
        const uint8_t MBISHI_FAN4 = 0x0B;
        const uint8_t MBISHI_HIPOWER = 0x07;
        const uint8_t MBISHI_ECONO = 0x00;

        // Vertical swing
        const uint8_t MBISHI_VS_SWING = 0xE0;
        const uint8_t MBISHI_VS_UP = 0xC0;
        const uint8_t MBISHI_VS_MUP = 0xA0;
        const uint8_t MBISHI_VS_MIDDLE = 0x80;
        const uint8_t MBISHI_VS_MDOWN = 0x60;
        const uint8_t MBISHI_VS_DOWN = 0x40;
        const uint8_t MBISHI_VS_STOP = 0x20;

        // Horizontal swing
        const uint8_t MBISHI_HS_SWING = 0x0F;
        const uint8_t MBISHI_HS_MIDDLE = 0x0C;
        const uint8_t MBISHI_HS_LEFT = 0x0E;
        const uint8_t MBISHI_HS_MLEFT = 0x0D;
        const uint8_t MBISHI_HS_MRIGHT = 0x0B;
        const uint8_t MBISHI_HS_RIGHT = 0x0A;
        const uint8_t MBISHI_HS_STOP = 0x07;
        const uint8_t MBISHI_HS_LEFTRIGHT = 0x08;
        const uint8_t MBISHI_HS_RIGHTLEFT = 0x09;

        // Only available in Auto, Cool and Heat mode
        const uint8_t MBISHI_3DAUTO_ON = 0x00;
        const uint8_t MBISHI_3DAUTO_OFF = 0x12;

        // NOT available in Fan or Dry mode
        const uint8_t MBISHI_SILENT_ON = 0x00;
        const uint8_t MBISHI_SILENT_OFF = 0x80;

        // Pulse parameters in usec
        const uint16_t MBISHI_BIT_MARK = 400;
        const uint16_t MBISHI_ONE_SPACE = 1200;
        const uint16_t MBISHI_ZERO_SPACE = 400;
        const uint16_t MBISHI_HEADER_MARK = 3200;
        const uint16_t MBISHI_HEADER_SPACE = 1600;
        const uint16_t MBISHI_MIN_GAP = 17500;

        bool MbishiClimate::on_receive(remote_base::RemoteReceiveData data) {
            ESP_LOGD(TAG, "Received some bytes");

            // The protocol sends the data twice, read here
            // uint32_t loop_read;
            
            uint8_t bytes[19] = {};

            //for (uint16_t loop = 1; loop <= 2; loop++) {
            if (!data.expect_item(MBISHI_HEADER_MARK, MBISHI_HEADER_SPACE))
                return false;
            
            // loop_read = 0;
            for (uint8_t a_byte = 0; a_byte < 19; a_byte++) {
                uint8_t byte = 0;
                for (int8_t a_bit = 0; a_bit < 8; a_bit++) {
                    if (data.expect_item(MBISHI_BIT_MARK, MBISHI_ONE_SPACE))
                        byte |= 1 << a_bit;
                    else if (!data.expect_item(MBISHI_BIT_MARK, MBISHI_ZERO_SPACE))
                        return false;
                }
                bytes[a_byte] = byte;
            }

            ESP_LOGD(TAG, 
                "Received bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            // Check the static bytes
            if (bytes[0] != 0x52 || bytes[1] != 0xAE || bytes[2] != 0xC3 || bytes[3] != 0x1A || bytes[4] != 0xE5) {
                return false;
            }

            ESP_LOGD(TAG, "Passed check 1");

            // Check the inversed bytes
            if (bytes[5] != (~bytes[6] & 0xFF)
                || bytes[7] != (~bytes[8] & 0xFF)
                || bytes[9] != (~bytes[10] & 0xFF)
                || bytes[11] != (~bytes[12] & 0xFF)
                || bytes[13] != (~bytes[14] & 0xFF)
                || bytes[15] != (~bytes[16] & 0xFF)
                || bytes[17] != (~bytes[18] & 0xFF)
            ) {
                return false;
            }
            
            ESP_LOGD(TAG, "Passed check 2");

            auto powerMode = bytes[5] & 0x08;
            auto operationMode = bytes[5] & 0x07;
            auto temperature = (~bytes[7] & 0x0F) + 17; 
            auto fanSpeed = bytes[9] & 0x0F;
            auto swingV = bytes[11] & 0xE0; // ignore the bit for the 3D auto
            auto swingH = bytes[13] & 0x0F;

            ESP_LOGD(TAG,
                "Resulting numbers: powerMode=0x%02X operationMode=0x%02X temperature=%d fanSpeed=0x%02X swingV=0x%02X swingH=0x%02X",
                powerMode, operationMode, temperature, fanSpeed, swingV, swingH
            );

            if (powerMode == MBISHI_ON) {
                // Power and operating mode
                switch (operationMode) {
                    case MBISHI_COOL:
                        this->mode = climate::CLIMATE_MODE_COOL;
                        //swingV = MBISHI_VS_UP;
                        break;
                    case MBISHI_HEAT:
                        this->mode = climate::CLIMATE_MODE_HEAT;
                        // swingV = MBISHI_VS_DOWN;
                        break;
                    case MBISHI_FAN:
                        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                        break;
                    case MBISHI_DRY:
                        this->mode = climate::CLIMATE_MODE_DRY;
                        break;
                    default:
                    case MBISHI_AUTO:
                        this->mode = climate::CLIMATE_MODE_AUTO;
                        // swingV = MBISHI_VS_MIDDLE;
                        break;
                }
            } else {
                this->mode = climate::CLIMATE_MODE_OFF;
            }

            // Temperature
            this->target_temperature = temperature;

            // Horizontal and vertical swing
            if (swingV == MBISHI_VS_SWING && swingH == MBISHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_BOTH;
            } else if (swingV == MBISHI_VS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
            } else if (swingH == MBISHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
            } else {
                this->swing_mode = climate::CLIMATE_SWING_OFF;
            }

            // Fan speed
            switch (fanSpeed) {
                case MBISHI_FAN1:
                case MBISHI_FAN2: // Only to support remote feedback
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    break;
                case MBISHI_FAN3:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    break;
                case MBISHI_FAN4:
                case MBISHI_HIPOWER: // Not yet supported. Will be added when ESPHome supports it.
                    this->fan_mode = climate::CLIMATE_FAN_HIGH;
                    break;
                case MBISHI_FAN_AUTO:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    switch (swingH) {
                        case MBISHI_HS_MIDDLE:
                            this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
                            break;
                        case MBISHI_HS_RIGHTLEFT:
                            this->fan_mode = climate::CLIMATE_FAN_FOCUS;
                            break;
                        case MBISHI_HS_LEFTRIGHT:
                            this->fan_mode = climate::CLIMATE_FAN_DIFFUSE;
                            break;
                    }
                case MBISHI_ECONO: // Not yet supported. Will be added when ESPHome supports it.
                default:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    break;
            }

            ESP_LOGD(TAG, "Finish it");
            
            this->publish_state();
            return true;
        }

        void MbishiClimate::transmit_state() {
            uint8_t remote_state[] = {
                0x52, 0xAE, 0xC3, 0x1A,
                0xE5, 0x90, 0x00, 0xF0,
                0x00, 0xF0, 0x00, 0x0D,
                0x00, 0x10, 0x00, 0xFF,
                0x00, 0x7F, 0x00
            };

            // ----------------------
            // Initial values
            // ----------------------

            auto operatingMode = MBISHI_AUTO;
            auto powerMode = MBISHI_ON;
            auto cleanMode = 0x60; // always off

            auto temperature = 22;
            auto fanSpeed = MBISHI_FAN_AUTO;
            auto swingV = MBISHI_VS_STOP;
            // auto swingH = MBISHI_HS_RIGHT;  // custom preferred value for this mode, should be MBISHI_HS_STOP
            auto swingH = MBISHI_HS_STOP;
            auto _3DAuto = MBISHI_3DAUTO_OFF;
            auto silentMode = MBISHI_SILENT_OFF;

            // ----------------------
            // Assign the values
            // ----------------------

            // Power and operating mode
            switch (this->mode) {
                case climate::CLIMATE_MODE_COOL:
                    operatingMode = MBISHI_COOL;
                    swingV = MBISHI_VS_UP; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_HEAT:
                    operatingMode = MBISHI_HEAT;
                    swingV = MBISHI_VS_DOWN; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_AUTO:
                    operatingMode = MBISHI_AUTO;
                    swingV = MBISHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    operatingMode = MBISHI_FAN;
                    swingV = MBISHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_DRY:
                    operatingMode = MBISHI_DRY;
                    swingV = MBISHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_OFF:
                default:
                    powerMode = MBISHI_OFF;
                    break;
            }

            // Temperature
            if (this->target_temperature > 17 && this->target_temperature < 31)
                temperature = this->target_temperature;

            // Horizontal and vertical swing
            switch (this->swing_mode) {
                case climate::CLIMATE_SWING_BOTH:
                    swingV = MBISHI_VS_SWING;
                    swingH = MBISHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_HORIZONTAL:
                    swingH = MBISHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_VERTICAL:
                    swingV = MBISHI_VS_SWING;
                    break;
                case climate::CLIMATE_SWING_OFF:
                default:
                    // Already on STOP
                    break;
            }

            // Fan speed
            switch (this->fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    fanSpeed = MBISHI_FAN1;
                    break;
                case climate::CLIMATE_FAN_MEDIUM:
                    fanSpeed = MBISHI_FAN3;
                    break;
                case climate::CLIMATE_FAN_HIGH:
                    fanSpeed = MBISHI_FAN4;
                    break;
                case climate::CLIMATE_FAN_MIDDLE:
                    fanSpeed = MBISHI_FAN_AUTO;
                    swingH = MBISHI_HS_MIDDLE;
                    break;
                case climate::CLIMATE_FAN_FOCUS:
                    fanSpeed = MBISHI_FAN_AUTO;
                    swingH = MBISHI_HS_RIGHTLEFT;
                    break;
                case climate::CLIMATE_FAN_DIFFUSE:
                    fanSpeed = MBISHI_FAN_AUTO;
                    swingH = MBISHI_HS_LEFTRIGHT;
                    break;
                case climate::CLIMATE_FAN_AUTO:
                default:
                    fanSpeed = MBISHI_FAN_AUTO;
                    break;
            }

            // ----------------------
            // Assign the bytes
            // ----------------------

            // Power state + operating mode
            remote_state[5] |= powerMode | operatingMode | cleanMode;

            // Temperature
            remote_state[7] |= (~((uint8_t)temperature - 17) & 0x0F);

            // Fan speed
            remote_state[9] |= fanSpeed;

            // Vertical air flow + 3D auto
            remote_state[11] |= swingV | _3DAuto;

            // Horizontal air flow
            remote_state[13] |= swingV | swingH;

            // Silent
            remote_state[15] |= silentMode;

            // There is no real checksum, but some bytes are inverted
            remote_state[6] = ~remote_state[5];
            remote_state[8] = ~remote_state[7];
            remote_state[10] = ~remote_state[9];
            remote_state[12] = ~remote_state[11];
            remote_state[14] = ~remote_state[13];
            remote_state[16] = ~remote_state[15];
            remote_state[18] = ~remote_state[17];

            // ESP_LOGD(TAG, "Sending MBISHI target temp: %.1f state: %02X mode: %02X temp: %02X", this->target_temperature, remote_state[5], remote_state[6], remote_state[7]);

            auto bytes = remote_state;
            ESP_LOGD(TAG, 
                "Sent bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            auto transmit = this->transmitter_->transmit();
            auto data = transmit.get_data();

            data->set_carrier_frequency(38000);

            // Header
            data->mark(MBISHI_HEADER_MARK);
            data->space(MBISHI_HEADER_SPACE);

            // Data
            for (uint8_t i : remote_state)
                for (uint8_t j = 0; j < 8; j++) {
                    data->mark(MBISHI_BIT_MARK);
                    bool bit = i & (1 << j);
                    data->space(bit ? MBISHI_ONE_SPACE : MBISHI_ZERO_SPACE);
                }
            data->mark(MBISHI_BIT_MARK);
            data->space(0);

            transmit.perform();
        }
    } // namespace mbishi
} // namespace esphome
