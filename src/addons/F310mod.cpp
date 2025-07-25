//
// Created by frozenform on 6/6/25.
//

#include "addons/F310mod.h"

#include "storagemanager.h"
#include "config.pb.h"
#include "hardware/adc.h"

#include "pico/time.h"
#include "hardware/gpio.h"

bool F310mod::available() {
    // Storage::getInstance().getAddonOptions()
    return true;
}

void F310mod::setup() {
    Gamepad *gamepad = Storage::getInstance().GetGamepad();
    gamepad->hasAnalogTriggers = true;
    gamepad->hasLeftAnalogStick = true;
    gamepad->hasRightAnalogStick = true;

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

    gpio_init(D2);
    gpio_set_dir(D2, GPIO_IN);
    gpio_pull_down(D2);

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

    updateAnalogs(gamepad);

    if (gpio_get(D2)) {
        System::reboot(System::BootMode::WEBCONFIG);
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

    // if (buttons == (GAMEPAD_MASK_B1 & GAMEPAD_MASK_R1 & GAMEPAD_MASK_S1)) {
    //     System::reboot(System::BootMode::WEBCONFIG);
    // }
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


void F310mod::updateAnalogs(Gamepad *gamepad) {
    auto &state = gamepad->state;

    selectAnalog(ANALOG_SELECT_X1);  
    state.lx = mapJoystickValue(adc_read());

    selectAnalog(ANALOG_SELECT_Y1);  
    state.ly = mapJoystickValue(adc_read());

    selectAnalog(ANALOG_SELECT_Z1);  
    state.lt = mapTriggerValue(adc_read());

    selectAnalog(ANALOG_SELECT_X2);  
    state.rx = mapJoystickValue(adc_read());

    selectAnalog(ANALOG_SELECT_Y2);  
    state.ry = mapJoystickValue(adc_read());

    selectAnalog(ANALOG_SELECT_Z2);  
    state.rt = mapTriggerValue(adc_read());
}

void F310mod::selectAnalog(const uint32_t selector) {
    gpio_put_masked(ANALOG_SELECT_MASK, selector);
}

uint16_t F310mod::mapJoystickValue(const uint16_t adcValue) {
    const float normalizedValue = static_cast<float>(adcValue) / ADC_MAX;
    return static_cast<uint16_t>(normalizedValue * GAMEPAD_JOYSTICK_MAX);
}

uint8_t F310mod::mapTriggerValue(const uint16_t adcValue) {
    const float normalizedValue = static_cast<float>(adcValue) / ADC_MAX;
    return static_cast<uint8_t>(normalizedValue * GAMEPAD_TRIGGER_MAX);
}
