//
// Created by frozenform on 6/6/25.
//

#include "addons/F310mod.h"

#include "storagemanager.h"
#include "config.pb.h"
#include "hardware/adc.h"

#include "pico/time.h"
#include "hardware/gpio.h"

using System::BootMode;

AnalogRange ranges[AXIS_COUNT];

auto selectingBootMode = MODE_NONE;
uint64_t holdStartTime = 0;

bool F310mod::available() {
    return true;
}

void readAxisRanges() {
    const auto opts = Storage::getInstance().getAddonOptions().f310Options;

    ranges[AXIS_X1] = {opts.x1AdcMin, opts.x1AdcMid, opts.x1AdcMax};
    ranges[AXIS_Y1] = {opts.y1AdcMin, opts.y1AdcMid, opts.y1AdcMax};
    ranges[AXIS_Z1] = {opts.z1AdcMin, opts.z1AdcMax};
    ranges[AXIS_X2] = {opts.x2AdcMin, opts.x2AdcMid, opts.x2AdcMax};
    ranges[AXIS_Y2] = {opts.y2AdcMin, opts.y2AdcMid, opts.y2AdcMax};
    ranges[AXIS_Z2] = {opts.z2AdcMin, opts.z2AdcMax};
}

void F310mod::setup() {
    Gamepad *gamepad = Storage::getInstance().GetGamepad();
    gamepad->hasAnalogTriggers = true;
    gamepad->hasLeftAnalogStick = true;
    gamepad->hasRightAnalogStick = true;

    readAxisRanges();

    gpio_init(PIN_M_OUT_0);
    gpio_init(PIN_M_OUT_1);
    gpio_init(PIN_M_OUT_2);
    gpio_init(PIN_M_OUT_3);

    gpio_init(PIN_M_IN_0);
    gpio_init(PIN_M_IN_1);
    gpio_init(PIN_M_IN_2);
    gpio_init(PIN_M_IN_3);

    gpio_set_dir(PIN_M_OUT_0, GPIO_OUT);
    gpio_set_dir(PIN_M_OUT_1, GPIO_OUT);
    gpio_set_dir(PIN_M_OUT_2, GPIO_OUT);
    gpio_set_dir(PIN_M_OUT_3, GPIO_OUT);

    gpio_set_dir(PIN_M_IN_0, GPIO_IN);
    gpio_set_dir(PIN_M_IN_1, GPIO_IN);
    gpio_set_dir(PIN_M_IN_2, GPIO_IN);
    gpio_set_dir(PIN_M_IN_3, GPIO_IN);

    gpio_put(PIN_M_OUT_0, false);
    gpio_put(PIN_M_OUT_1, false);
    gpio_put(PIN_M_OUT_2, false);
    gpio_put(PIN_M_OUT_3, false);

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    gpio_put(PIN_STATUS_LED, false);

    adc_init();
    adc_gpio_init(PIN_ANALOG_IN);
    adc_select_input(0);
}

void F310mod::preprocess() {
}

void F310mod::process() {
    Gamepad *gamepad = Storage::getInstance().GetGamepad();

    gpio_set_dir(PIN_M_IN_0, GPIO_IN);
    gpio_set_dir(PIN_M_IN_1, GPIO_IN);
    gpio_set_dir(PIN_M_IN_2, GPIO_IN);
    gpio_set_dir(PIN_M_IN_3, GPIO_IN);

    updateButtons(gamepad);

    gpio_set_dir(S0, GPIO_OUT);
    gpio_set_dir(S1, GPIO_OUT);
    gpio_set_dir(S2, GPIO_OUT);
    gpio_set_dir(S3, GPIO_OUT);

    updateRawAnalogs();

    if (calibrating) {
        if (
            const auto buttons = gamepad->state.buttons;
            buttons == XINPUT_X
        ) {
            commitCalibration();
        } else if (buttons == XINPUT_B) {
            calibrating = false;
        }

        updateCalibrationData();

    } else {
        updateAnalogs(gamepad);
        checkSpecialCombinations(gamepad);
        checkModeChange(gamepad);
    }
}

void F310mod::updateButtons(Gamepad *gamepad) {
    uint32_t buttons = gamepad->state.buttons;
    uint8_t dpad = 0;

    buttons |= getMask(PIN_M_OUT_0,  XINPUT_A, XINPUT_B, XINPUT_X, XINPUT_Y);
    dpad |= getMask(PIN_M_OUT_1,  XINPUT_DPAD_LEFT, XINPUT_DPAD_UP, XINPUT_DPAD_RIGHT, XINPUT_DPAD_DOWN);
    buttons |= getMask(PIN_M_OUT_2,  XINPUT_RB, XINPUT_LB, XINPUT_RS, XINPUT_LS);
    buttons |= getMask(PIN_M_OUT_3,  F310_BUTTON_NOT_CONNECTED, XINPUT_BACK, XINPUT_GUIDE, F310_MODE_BUTTON);

    gamepad->state.buttons = buttons;
    gamepad->state.dpad = dpad;
}

uint32_t F310mod::getMask(const uint32_t outPin, const uint32_t mask0, const uint32_t mask1, const uint32_t mask2, const uint32_t mask3)  {
    uint32_t finalMask = 0;

    gpio_put(outPin, true);
    sleep_us(20); //stabilize out voltage TODO: check for better way or optimize wait time

    if (gpio_get(PIN_M_IN_0)) {
        finalMask |= mask0;
    }
    if (gpio_get(PIN_M_IN_1)) {
        finalMask |= mask1;
    }
    if (gpio_get(PIN_M_IN_2)) {
        finalMask |= mask2;
    }
    if (gpio_get(PIN_M_IN_3)) {
        finalMask |= mask3;
    }

    gpio_put(outPin, false);

    return finalMask;
}

uint16_t invertAdcValue(const uint16_t adcValue) {
    return static_cast<uint16_t>(ADC_MAX) - adcValue;
}

void F310mod::updateRawAnalogs() {
    selectAnalog(ANALOG_SELECT_X1);
    analogValues.x1 = adc_read();

    selectAnalog(ANALOG_SELECT_Y1);
    analogValues.y1 = invertAdcValue(adc_read());

    selectAnalog(ANALOG_SELECT_Z1);
    analogValues.z1 = adc_read();

    selectAnalog(ANALOG_SELECT_X2);
    analogValues.x2 = invertAdcValue(adc_read());

    selectAnalog(ANALOG_SELECT_Y2);
    analogValues.y2 = adc_read();

    selectAnalog(ANALOG_SELECT_Z2);
    analogValues.z2 = invertAdcValue(adc_read());
}

uint8_t getDigitalTriggerValue(const uint8_t analogValue, const float thresholdPercent) {
    const auto thresholdValue = static_cast<uint8_t>(thresholdPercent / 100.0f * GAMEPAD_TRIGGER_MAX);
    return analogValue >= thresholdValue ? GAMEPAD_TRIGGER_MAX : GAMEPAD_TRIGGER_MIN;
}

void F310mod::updateAnalogs(Gamepad *gamepad) const {
    auto &state = gamepad->state;

    state.lx = mapJoystickValue(analogValues.x1, AXIS_X1);
    state.ly = mapJoystickValue(analogValues.y1, AXIS_Y1);
    state.rx = mapJoystickValue(analogValues.x2, AXIS_X2);
    state.ry = mapJoystickValue(analogValues.y2, AXIS_Y2);

    const uint8_t z1Mapped = mapTriggerValue(analogValues.z1, AXIS_Z1);
    const uint8_t z2Mapped = mapTriggerValue(analogValues.z2, AXIS_Z2);

    if (triggerMode == TRIGGER_MODE_ANALOG) {
        state.lt = z1Mapped;
        state.rt = z2Mapped;
    } else {
        const auto opts = Storage::getInstance().getAddonOptions().f310Options;
        state.lt = getDigitalTriggerValue(z1Mapped, opts.digitalLeftTriggerThresholdPercent);
        state.rt = getDigitalTriggerValue(z2Mapped, opts.digitalRightTriggerThresholdPercent);
    }
}

static bool modeButtonPrevState = false;

void F310mod::checkModeChange(const Gamepad *gamepad) {
    const auto buttons = gamepad->state.buttons;
    const bool modeButtonState = buttons & F310_MODE_BUTTON;
    if (modeButtonState && modeButtonState != modeButtonPrevState) {
        triggerMode = triggerMode == TRIGGER_MODE_ANALOG
            ? TRIGGER_MODE_DIGITAL
            : TRIGGER_MODE_ANALOG;
        gpio_put(PIN_STATUS_LED, triggerMode == TRIGGER_MODE_DIGITAL);
    }
    modeButtonPrevState = modeButtonState;
}


void F310mod::checkSpecialCombinations(const Gamepad *gamepad) {
    const auto buttons = gamepad->state.buttons;
    const auto dpad = gamepad->state.dpad;

    const auto currentSpecialMode = selectingBootMode;

    if (buttons == SPECIAL_REBOOT_BOOTSEL && dpad == XINPUT_DPAD_UP) {
        if (currentSpecialMode != MODE_REBOOT_BOOTSEL) {
            selectingBootMode = MODE_REBOOT_BOOTSEL;
            holdStartTime = to_us_since_boot(get_absolute_time());
        }
    } else if (buttons == SPECIAL_REBOOT_WEBCONFIG) {
        if (currentSpecialMode != MODE_REBOOT_WEBCONFIG) {
            selectingBootMode = MODE_REBOOT_WEBCONFIG;
            holdStartTime = to_us_since_boot(get_absolute_time());
        }
    } else if (buttons == SPECIAL_BEGIN_CALIBRATION) {
        if (currentSpecialMode != MODE_BEGIN_CALIBRATION) {
            selectingBootMode = MODE_BEGIN_CALIBRATION;
            holdStartTime = to_us_since_boot(get_absolute_time());
        }
    } else {
        selectingBootMode = MODE_NONE;
        return;
    }

    if (
        const auto now = to_us_since_boot(get_absolute_time());
        now - holdStartTime > 5000000
    ) {
        switch (currentSpecialMode) {
            case MODE_REBOOT_BOOTSEL:
                System::reboot(BootMode::USB);
                break;
            case MODE_REBOOT_WEBCONFIG:
                System::reboot(BootMode::WEBCONFIG);
                break;
            case MODE_BEGIN_CALIBRATION:
                enterCalibrationMode();
                break;
            default:
                return;
        }
    }
}


void F310mod::selectAnalog(const uint32_t selector) {
    gpio_put_masked(ANALOG_SELECT_MASK, selector);
}

float mapRange(const float value, const float inMin, const float inMax, const float outMin, const float outMax) {
    return (value - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}

uint16_t F310mod::mapJoystickValue(const uint16_t adcValue, const int axisIndex) {
    const auto range = ranges[axisIndex];
    const auto normalizedAdcValue = static_cast<float>(adcValue) / ADC_MAX;
    const float normalizedValue = adcValue < ADC_MIDI
        ? mapRange(normalizedAdcValue, range.minNormalized, range.midNormalized, 0.0f, 0.5f)
        : mapRange(normalizedAdcValue, range.midNormalized, range.maxNormalized, 0.5f, 1.0f);
    const float clampedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
    return static_cast<uint16_t>(clampedValue * GAMEPAD_JOYSTICK_MAX);
}

uint8_t F310mod::mapTriggerValue(const uint16_t adcValue, const int axisIndex) {
    const auto range = ranges[axisIndex];
    const auto normalizedAdcValue = static_cast<float>(adcValue) / ADC_MAX;
    const auto normalizedValue = mapRange(normalizedAdcValue, range.minNormalized, range.maxNormalized, 0.0f, 1.0f);
    const float clampedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
    return static_cast<uint16_t>(clampedValue * GAMEPAD_TRIGGER_MAX);

}

void F310mod::enterCalibrationMode() {
    calibrating = true;
    calibrationState.reset();
}


void F310mod::updateCalibrationData() {
    calibrationState.x1.min = std::min(calibrationState.x1.min, analogValues.x1);
    calibrationState.x1.max = std::max(calibrationState.x1.max, analogValues.x1);
    calibrationState.y1.min = std::min(calibrationState.y1.min, analogValues.y1);
    calibrationState.y1.max = std::max(calibrationState.y1.max, analogValues.y1);
    calibrationState.z1.min = std::min(calibrationState.z1.min, analogValues.z1);
    calibrationState.z1.max = std::max(calibrationState.z1.max, analogValues.z1);
    calibrationState.x2.min = std::min(calibrationState.x2.min, analogValues.x2);
    calibrationState.x2.max = std::max(calibrationState.x2.max, analogValues.x2);
    calibrationState.y2.min = std::min(calibrationState.y2.min, analogValues.y2);
    calibrationState.y2.max = std::max(calibrationState.y2.max, analogValues.y2);
    calibrationState.z2.min = std::min(calibrationState.z2.min, analogValues.z2);
    calibrationState.z2.max = std::max(calibrationState.z2.max, analogValues.z2);
}

void F310mod::commitCalibration() {
    // Get the storage instance
    Storage& storage = Storage::getInstance();

    // Get the current addon options (this should return a mutable reference)
    auto& addonOptions = storage.getAddonOptions();
    auto& f310Options = addonOptions.f310Options;

    // Update the calibration values
    f310Options.x1AdcMin = static_cast<uint32_t>(calibrationState.x1.min);
    f310Options.x1AdcMax = static_cast<uint32_t>(calibrationState.x1.max);
    f310Options.y1AdcMin = static_cast<uint32_t>(calibrationState.y1.min);
    f310Options.y1AdcMax = static_cast<uint32_t>(calibrationState.y1.max);
    f310Options.z1AdcMin = static_cast<uint32_t>(calibrationState.z1.min);
    f310Options.z1AdcMax = static_cast<uint32_t>(calibrationState.z1.max);
    f310Options.x2AdcMin = static_cast<uint32_t>(calibrationState.x2.min);
    f310Options.x2AdcMax = static_cast<uint32_t>(calibrationState.x2.max);
    f310Options.y2AdcMin = static_cast<uint32_t>(calibrationState.y2.min);
    f310Options.y2AdcMax = static_cast<uint32_t>(calibrationState.y2.max);
    f310Options.z2AdcMin = static_cast<uint32_t>(calibrationState.z2.min);
    f310Options.z2AdcMax = static_cast<uint32_t>(calibrationState.z2.max);

    // Save the updated options to persistent storage
    storage.save();

    // Update the local ranges with the new values
    readAxisRanges();

    // Exit calibration mode
    calibrating = false;
}
