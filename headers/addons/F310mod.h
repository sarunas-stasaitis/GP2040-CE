//
// Created by frozenform on 6/6/25.
//

#ifndef F310MOD_H
#define F310MOD_H


#include "gpaddon.h"
#include "GamepadEnums.h"
#include "BoardConfig.h"
#include "enums.pb.h"

#ifndef F310MOD_ENABLED
#define F310MOD_ENABLED 0
#endif

// IO Module Name
#define AddonName "F310MOD"

#define D0 26
#define D1 27
#define D2 28
#define D3 29
#define D4 6
#define D5 7
#define D6 0
#define D7 3
#define D8 4
#define D9 2
#define D10 1

#define A0 D0

#define PIN_STATUS_LED D2

#define PIN_M_OUT_0 D3
#define PIN_M_OUT_1 D4
#define PIN_M_OUT_2 D5
#define PIN_M_OUT_3 D6

#define PIN_M_IN_0 D7
#define PIN_M_IN_1 D8
#define PIN_M_IN_2 D9
#define PIN_M_IN_3 D10

#define PIN_ANALOG_IN D0
//multiplexer select pins
#define S0 D7
#define S1 D8
#define S2 D9
#define S3 D10

#define ANALOG_SELECT_MASK 0b0000000000000011110
//select pin masks
#define MS0 (1U << S0)
#define MS1 (1U << S1)
#define MS2 (1U << S2)
#define MS3 (1U << S3)

#define ANALOG_SELECT_X1   0
#define ANALOG_SELECT_Y1   MS0
#define ANALOG_SELECT_Z1   (MS3 | MS0)
#define ANALOG_SELECT_X2   (MS0 | MS1)
#define ANALOG_SELECT_Y2   MS1
#define ANALOG_SELECT_Z2   MS3

#define AXIS_X1 0
#define AXIS_Y1 1
#define AXIS_Z1 2
#define AXIS_X2 3
#define AXIS_Y2 4
#define AXIS_Z2 5
#define AXIS_COUNT 6

#define XINPUT_A           GAMEPAD_MASK_B1
#define XINPUT_B           GAMEPAD_MASK_B2
#define XINPUT_X           GAMEPAD_MASK_B3
#define XINPUT_Y           GAMEPAD_MASK_B4
#define XINPUT_LS          GAMEPAD_MASK_L1
#define XINPUT_RS          GAMEPAD_MASK_R1
#define XINPUT_LB          GAMEPAD_MASK_L3
#define XINPUT_RB          GAMEPAD_MASK_R3
#define XINPUT_BACK        GAMEPAD_MASK_S1
#define XINPUT_START       GAMEPAD_MASK_S2
#define XINPUT_GUIDE       GAMEPAD_MASK_A1
#define XINPUT_UNUSED      GAMEPAD_MASK_A2
#define XINPUT_DPAD_UP     GAMEPAD_MASK_UP
#define XINPUT_DPAD_DOWN   GAMEPAD_MASK_DOWN
#define XINPUT_DPAD_LEFT   GAMEPAD_MASK_LEFT
#define XINPUT_DPAD_RIGHT  GAMEPAD_MASK_RIGHT

#define SPECIAL_REBOOT_BOOTSEL (XINPUT_BACK | XINPUT_START)
#define SPECIAL_REBOOT_WEBCONFIG (XINPUT_START | XINPUT_X | XINPUT_Y)
#define SPECIAL_BEGIN_CALIBRATION (XINPUT_START | XINPUT_Y | XINPUT_B)

#define F310_MODE_BUTTON   GAMEPAD_MASK_A2

#define F310_BUTTON_NOT_CONNECTED 0

#define ADC_MAX 4095.0f

#define ADC_MINI 0
#define ADC_MIDI 2047
#define ADC_MAXI 4095


struct RawRange {
    uint16_t min, max;
};

struct CalibrationState {
    RawRange x1 = {ADC_MAXI, 0};
    RawRange y1 = {ADC_MAXI, 0};
    RawRange z1 = {ADC_MAXI, 0};
    RawRange x2 = {ADC_MAXI, 0};
    RawRange y2 = {ADC_MAXI, 0};
    RawRange z2 = {ADC_MAXI, 0};

    void reset() {
        x1 = {ADC_MAXI, 0};
        y1 = {ADC_MAXI, 0};
        z1 = {ADC_MAXI, 0};
        x2 = {ADC_MAXI, 0};
        y2 = {ADC_MAXI, 0};
        z2 = {ADC_MAXI, 0};
    }
};

struct AnalogValues {
    uint16_t x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
};

class F310mod final : public GPAddon {

public:
    virtual void bootProcess() {}
    virtual bool available();
    virtual void setup();
    virtual void preprocess();

    virtual void process();
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return AddonName; }

private:
    bool calibrating = false;
    CalibrationState calibrationState;
    AnalogValues analogValues;

    static void updateButtons(Gamepad *gamepad);
    static uint32_t getMask(uint32_t outPin, uint32_t mask0, uint32_t mask1, uint32_t mask2, uint32_t mask3);

    void updateRawAnalogs();
    void updateAnalogs(Gamepad *gamepad) const;
    static void selectAnalog(uint32_t selector);

    static uint16_t mapJoystickValue(uint16_t adcValue, int axisIndex);
    static uint8_t mapTriggerValue(uint16_t adcValue, int axisIndex);

    void checkSpecialCombinations(const Gamepad *gamepad);

    void enterCalibrationMode();
    void updateCalibrationData();
    void commitCalibration();
};

enum SpecialMode {
    MODE_NONE = 0,
    MODE_REBOOT_BOOTSEL,
    MODE_REBOOT_WEBCONFIG,
    MODE_BEGIN_CALIBRATION
};

struct AnalogRange {
    float minNormalized;
    float midNormalized;
    float maxNormalized;

    AnalogRange() :
        minNormalized(0.0f), midNormalized(0.5f), maxNormalized(1.0f) {}

    AnalogRange(const uint16_t minRaw, const uint16_t maxRaw) :
        minNormalized(static_cast<float>(minRaw) / ADC_MAX),
        midNormalized(0.5f),
        maxNormalized(static_cast<float>(maxRaw) / ADC_MAX) {}

    AnalogRange(const uint16_t minRaw, const uint16_t midRaw, const uint16_t maxRaw) :
        minNormalized(static_cast<float>(minRaw) / ADC_MAX),
        midNormalized(static_cast<float>(midRaw) / ADC_MAX),
        maxNormalized(static_cast<float>(maxRaw) / ADC_MAX) {}

    AnalogRange(const uint32_t minRaw, const uint32_t maxRaw) :
        minNormalized(static_cast<float>(minRaw) / ADC_MAX),
        midNormalized(0.5f),
        maxNormalized(static_cast<float>(maxRaw) / ADC_MAX) {}

    AnalogRange(const uint32_t minRaw, const uint32_t midRaw, const uint32_t maxRaw) :
        minNormalized(static_cast<float>(minRaw) / ADC_MAX),
        midNormalized(static_cast<float>(midRaw) / ADC_MAX),
        maxNormalized(static_cast<float>(maxRaw) / ADC_MAX) {}

};

#endif //F310MOD_H
