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

#define F310_MODE_BUTTON   GAMEPAD_MASK_A2

#define F310_BUTTON_NOT_CONNECTED 0

#define ADC_MAX 4095.0f

class F310mod : public GPAddon {

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
    static void updateButtons(Gamepad *gamepad);
    static uint32_t getMask(uint32_t outPin, uint32_t mask0, uint32_t mask1, uint32_t mask2, uint32_t mask3);

    static void updateAnalogs(Gamepad *gamepad);
    static void selectAnalog(uint32_t selector);

    static uint16_t mapJoystickValue(uint16_t adcValue);
    static uint8_t mapTriggerValue(uint16_t adcValue);
};



#endif //F310MOD_H
