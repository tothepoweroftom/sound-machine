#pragma once

#include <midi/MidiCommunicator.h>
#include "view/push2/Push2Listener.h"
#include "view/push2/Push2Colours.h"
#include "Project.h"

class Push2MidiCommunicator : public MidiCommunicator, private Push2Colours::Listener, private Timer {
public:
    explicit Push2MidiCommunicator(Project& project, Push2Colours& push2Colours) :
            project(project), viewManager(project.getViewStateManager()), push2Colours(push2Colours) {};

    void initialize() override {
        MidiCommunicator::initialize();
        push2Colours.addListener(this);
        registerAllIndexedColours();

        /*
         * https://github.com/Ableton/push-interface/blob/master/doc/AbletonPush2MIDIDisplayInterface.asc#Aftertouch
         * In channel pressure mode (default), the pad with the highest pressure determines the value sent.
         * The pressure range that produces aftertouch is given by the aftertouch threshold pad parameters.
         * The value curve is linear to the pressure and in range 0 to 127. See Pad Parameters.
         * In polyphonic key pressure mode, aftertouch for each pressed key is sent individually.
         * The value is defined by the pad velocity curve and in range 1…​127. See Velocity Curve.
         */
        static const uint8 setAftertouchModePolyphonic[] { 0x00, 0x21, 0x1D, 0x01, 0x01, 0x1E, 0x01 };
        auto polyphonicAftertouchSysExMessage = MidiMessage::createSysExMessage(setAftertouchModePolyphonic, 7);
        sendMessageChecked(polyphonicAftertouchSysExMessage);

        push2Listener->deviceConnected();
    }

    ~Push2MidiCommunicator() override {
        push2Colours.removeListener(this);
    }

    Push2Colours& getPush2Colours() { return push2Colours; }

    void setPush2Listener(Push2Listener *push2Listener) {
        this->push2Listener = push2Listener;
    }

    void handleIncomingMidiMessage(MidiInput *source, const MidiMessage &message) override {
        // Only pass non-sysex note messages to listeners if we're in a non-control mode.
        // (Allow note-off messages through in case switch to control mode happened during note events.)
        if (message.isNoteOnOrOff() && isPadNoteNumber(message.getNoteNumber()) && (viewManager.isInNoteMode() || message.isNoteOff())) {
            MidiCommunicator::handleIncomingMidiMessage(source, message);
        }

        MessageManager::callAsync([this, source, message]() {
            if (push2Listener == nullptr)
                return;

            if (message.isController()) {
                const auto ccNumber = message.getControllerNumber();

                if (isEncoderCcNumber(ccNumber)) {
                    float changeAmount = encoderCcMessageToRotationChange(message);
                    if (ccNumber == masterKnob) {
                        return push2Listener->masterEncoderRotated(changeAmount / 2.0f);
                    } else if (isAboveScreenEncoderCcNumber(ccNumber)) {
                        return push2Listener->encoderRotated(ccNumber - ccNumberForTopKnobIndex(0), changeAmount / 2.0f);
                    }
                    return;
                }

                if (isButtonPressControlMessage(message)) {
                    static const Array<int> repeatableButtonCcNumbers { undo, up, down, left, right };
                    if (repeatableButtonCcNumbers.contains(ccNumber)) {
                        buttonHoldStopped();
                        currentlyHeldRepeatableButtonCcNumber = ccNumber;
                        startTimer(BUTTON_HOLD_WAIT_FOR_REPEAT_MS);
                    }
                    return handleButtonPressMidiCcNumber(ccNumber);
                }
                else if (isButtonReleaseControlMessage(message)) {
                    buttonHoldStopped();
                    return handleButtonReleaseMidiCcNumber(ccNumber);
                }
            } else {
                push2Listener->handleIncomingMidiMessage(source, message);
            }
        });
    }

    void handleButtonPressMidiCcNumber(int ccNumber) {
        if (isAboveScreenButtonCcNumber(ccNumber))
            return push2Listener->aboveScreenButtonPressed(ccNumber - topDisplayButton1);
        else if (isBelowScreenButtonCcNumber(ccNumber))
            return push2Listener->belowScreenButtonPressed(ccNumber - bottomDisplayButton1);
        else if (isArrowButtonCcNumber(ccNumber))
            return push2Listener->arrowPressed(directionForArrowButtonCcNumber(ccNumber));
        switch (ccNumber) {
            case shift:
                push2Listener->shiftPressed();
                return;
            case undo:
                return push2Listener->undoButtonPressed();
            case delete_:
                return push2Listener->deleteButtonPressed();
            case duplicate:
                return push2Listener->duplicateButtonPressed();
            case addTrack:
                return push2Listener->addTrackButtonPressed();
            case addDevice:
                return push2Listener->addDeviceButtonPressed();
            case mix:
                return push2Listener->mixButtonPressed();
            case master:
                return push2Listener->masterButtonPressed();
            case note:
                return push2Listener->noteButtonPressed();
            case session:
                return push2Listener->sessionButtonPressed();
            default:
                return;
        }
    }

    void handleButtonReleaseMidiCcNumber(int ccNumber) {
        switch (ccNumber) {
            case shift:
                push2Listener->shiftReleased();
                return;
            default:
                return;
        }
    }

    static const uint8
            topKnob1 = 14, topKnob2 = 15, topKnob3 = 71, topKnob4 = 72, topKnob5 = 73, topKnob6 = 74, topKnob7 = 75,
            topKnob8 = 76, topKnob9 = 77, topKnob10 = 78, masterKnob = 79, tapTempo = 3, metronome = 9, setup = 30, user = 59,
            topDisplayButton1 = 102, topDisplayButton2 = 103, topDisplayButton3 = 104, topDisplayButton4 = 105,
            topDisplayButton5 = 106, topDisplayButton6 = 107, topDisplayButton7 = 108, topDisplayButton8 = 109,
            bottomDisplayButton1 = 20, bottomDisplayButton2 = 21, bottomDisplayButton3 = 22, bottomDisplayButton4 = 23,
            bottomDisplayButton5 = 24, bottomDisplayButton6 = 25, bottomDisplayButton7 = 26, bottomDisplayButton8 = 27,
            delete_ = 118, undo = 119, addDevice = 52, addTrack = 53, device = 110, mix = 112, browse = 111, clip = 113,
            mute = 60, solo = 61, stopClip = 29, master = 28, convert = 35, doubleLoop = 117, quantize = 116, duplicate = 88,
            new_ = 87, fixedLength = 90, automate = 89, record = 86, play = 85,
            divide_1_32t = 43, divide_1_32 = 42, divide_1_16_t = 41, divide_1_16 = 40,
            divide_1_8_t = 39, divide_1_8 = 38, divide_1_4t = 37, divide_1_4 = 36,
            left = 44, right = 45, up = 46, down = 47, repeat = 56, accent = 57, scale = 58, layout = 31, note = 50,
            session = 51, octaveUp = 55, octaveDown = 54, pageLeft = 62, pageRight = 63, shift = 49, select = 48;

    static const int upArrowDirection = 0, downArrowDirection = 1, leftArrowDirection = 2, rightArrowDirection = 3;

    static const int lowestPadNoteNumber = 36, highestPadNoteNumber = 99;

    // From https://github.com/Ableton/push-interface/blob/master/doc/AbletonPush2MIDIDisplayInterface.asc#Encoders:
    //
    // The value 0xxxxxx or 1yyyyyy gives the amount of accumulated movement since the last message. The faster you move, the higher the value.
    // The value is given as a 7 bit relative value encoded in two’s complement. 0xxxxxx indicates a movement to the right,
    // with decimal values from 1 to 63 (in practice, values above 20 are unlikely). 1yyyyyy means movement to the left, with decimal values from 127 to 64.
    // The total step count sent for a 360° turn is approx. 210, except for the detented tempo encoder, where one turn is 18 steps.
    //
    // This function returns a value between -1 and 1 normalized so that (roughly) the magnitude of a full rotation would sum to 1.
    // i.e. Turning 210 'steps' to the left would total to ~-1, and turning 210 steps to the right would total ~1.
    static float encoderCcMessageToRotationChange(const MidiMessage& message) {
        auto value = float(message.getControllerValue());
        if (value <= 63)
            return value / 210.0f;
        else
            return (value - 128.0f) / 210.0f;
    }

    static uint8 ccNumberForTopKnobIndex(int topKnobIndex) {
        switch (topKnobIndex) {
            case 0: return topKnob3;
            case 1: return topKnob4;
            case 2: return topKnob5;
            case 3: return topKnob6;
            case 4: return topKnob7;
            case 5: return topKnob8;
            case 6: return topKnob9;
            case 7: return topKnob10;
            default: return 0;
        }
    }

    static uint8 ccNumberForArrowButton(int direction) {
        switch(direction) {
            case upArrowDirection: return up;
            case downArrowDirection: return down;
            case leftArrowDirection: return left;
            case rightArrowDirection: return right;
            default: return 0;
        }
    }

    static int directionForArrowButtonCcNumber(int ccNumber) {
        jassert(isArrowButtonCcNumber(ccNumber));

        switch (ccNumber) {
            case left: return leftArrowDirection;
            case right: return rightArrowDirection;
            case up: return upArrowDirection;
            default: return downArrowDirection;
        }
    }

    static bool isEncoderCcNumber(int ccNumber) {
        return ccNumber == topKnob1 || ccNumber == topKnob2 || ccNumber == masterKnob || isAboveScreenEncoderCcNumber(ccNumber);
    }

    static bool isAboveScreenEncoderCcNumber(int ccNumber) {
        return ccNumber >= topKnob3 && ccNumber <= topKnob10;
    }

    static bool isArrowButtonCcNumber(int ccNumber) {
        return ccNumber >= left && ccNumber <= down;
    }

    static bool isAboveScreenButtonCcNumber(int ccNumber) {
        return ccNumber >= topDisplayButton1 && ccNumber <= topDisplayButton8;
    }

    static bool isBelowScreenButtonCcNumber(int ccNumber) {
        return ccNumber >= bottomDisplayButton1 && ccNumber <= bottomDisplayButton8;
    }

    static bool isButtonPressControlMessage(const MidiMessage &message) {
        return message.getControllerValue() == 127;
    }

    static bool isButtonReleaseControlMessage(const MidiMessage &message) {
        return message.getControllerValue() == 0;
    }

    static bool isPadNoteNumber(int noteNumber) {
        return noteNumber >= lowestPadNoteNumber && noteNumber <= highestPadNoteNumber;
    }

    void setAboveScreenButtonColour(int buttonIndex, const Colour &colour) {
        setButtonColour(topDisplayButton1 + buttonIndex, colour);
    }

    void setBelowScreenButtonColour(int buttonIndex, const Colour &colour) {
        setButtonColour(bottomDisplayButton1 + buttonIndex, colour);
    }

    void setAboveScreenButtonEnabled(int buttonIndex, bool enabled) {
        setColourButtonEnabled(topDisplayButton1 + buttonIndex, enabled);
    }

    void setBelowScreenButtonEnabled(int buttonIndex, bool enabled) {
        setColourButtonEnabled(bottomDisplayButton1 + buttonIndex, enabled);
    }

    void enableWhiteLedButton(int buttonCcNumber) const {
        sendMessageChecked(MidiMessage::controllerEvent(NO_ANIMATION_LED_CHANNEL, buttonCcNumber, 14));
    }

    void disableWhiteLedButton(int buttonCcNumber) const {
        sendMessageChecked(MidiMessage::controllerEvent(NO_ANIMATION_LED_CHANNEL, buttonCcNumber, 0));
    }

    void activateWhiteLedButton(int buttonCcNumber) const {
        sendMessageChecked(MidiMessage::controllerEvent(NO_ANIMATION_LED_CHANNEL, buttonCcNumber, 127));
    }

    void setColourButtonEnabled(int buttonCcNumber, bool enabled) {
        if (enabled)
            setButtonColour(buttonCcNumber, Colours::white);
        else
            disableWhiteLedButton(buttonCcNumber);
    }

    void setButtonColour(int buttonCcNumber, const Colour &colour) {
        auto colourIndex = push2Colours.findIndexForColourAddingIfNeeded(colour);
        sendMessageChecked(MidiMessage::controllerEvent(NO_ANIMATION_LED_CHANNEL, buttonCcNumber, colourIndex));
    }

    void disablePad(int noteNumber) {
        if (!isPadNoteNumber(noteNumber))
            return;

        sendMessageChecked(MidiMessage::noteOn(NO_ANIMATION_LED_CHANNEL, noteNumber, uint8(0)));
    }

    void setPadColour(int noteNumber, const Colour &colour) {
        if (!isPadNoteNumber(noteNumber))
            return;

        auto colourIndex = push2Colours.findIndexForColourAddingIfNeeded(colour);
        sendMessageChecked(MidiMessage::noteOn(NO_ANIMATION_LED_CHANNEL, noteNumber, colourIndex));
    }

private:
    static constexpr int NO_ANIMATION_LED_CHANNEL = 1;
    static constexpr int BUTTON_HOLD_REPEAT_HZ = 10; // how often to repeat a repeatable button press when it is held
    static constexpr int BUTTON_HOLD_WAIT_FOR_REPEAT_MS = 500; // how long to wait before starting held button message repeats

    Project& project;
    ViewStateManager& viewManager;
    Push2Colours& push2Colours;
    Push2Listener *push2Listener {};

    int currentlyHeldRepeatableButtonCcNumber {0};

    bool holdRepeatIsHappeningNow { false };

    void sendMessageChecked(const MidiMessage& message) const {
        if (isOutputConnected()) {
            midiOutput->sendMessageNow(message);
        }
    }

    void registerAllIndexedColours() {
        for (std::pair<String, uint8> pair : push2Colours.indexForColour) {
            colourAdded(Colour::fromString(pair.first), pair.second);
        }
    }

    void colourAdded(const Colour& colour, uint8 colourIndex) override {
        jassert(colourIndex > 0 && colourIndex < CHAR_MAX - 1);

        uint32 argb = colour.getARGB();
        // 8 bytes: 2 for each of R, G, B, W. First byte contains the 7 LSBs; Second byte contains the 1 MSB.
        uint8 bgra[8];

        for (int i = 0; i < 4; i++) {
            auto c = static_cast<uint8>(argb >> (i * CHAR_BIT));
            bgra[i * 2] = static_cast<uint8>(c & 0x7F);
            bgra[i * 2 + 1] = static_cast<uint8>(c & 0x80) >> 7;
        }

        const uint8 setLedColourPaletteEntryCommand[] { 0x00, 0x21, 0x1D, 0x01, 0x01, 0x03, colourIndex,
                                                        bgra[4], bgra[5], bgra[2], bgra[3], bgra[0], bgra[1], bgra[6], bgra[7] };
        auto setLedColourPaletteEntryMessage = MidiMessage::createSysExMessage(setLedColourPaletteEntryCommand, 15);
        sendMessageChecked(setLedColourPaletteEntryMessage);
        static const uint8 reapplyColorPaletteCommand[] { 0x00, 0x21, 0x1D, 0x01, 0x01, 0x05 };
        sendMessageChecked(MidiMessage::createSysExMessage(reapplyColorPaletteCommand, 6));
    }

    void trackColourChanged(const String &trackUuid, const Colour &colour) override {}

    void buttonHoldStopped() {
        currentlyHeldRepeatableButtonCcNumber = 0;
        holdRepeatIsHappeningNow = false;
        stopTimer();
    }

    void timerCallback() override {
        if (currentlyHeldRepeatableButtonCcNumber != 0) {
            handleButtonPressMidiCcNumber(currentlyHeldRepeatableButtonCcNumber);
            if (!holdRepeatIsHappeningNow) {
                holdRepeatIsHappeningNow = true;
                // after the initial wait for a long hold, start sending the message repeatedly during hold
                startTimerHz(BUTTON_HOLD_REPEAT_HZ);
            }
        } else {
            buttonHoldStopped();
        }
    }
};
