#pragma once

#include "JuceHeader.h"
#include "Identifiers.h"

struct Parameter {
    Parameter(ValueTree& state, String paramID, String paramName, const String labelText, NormalisableRange<double> range,
                  float defaultVal, std::function<String (const float)> valueToTextFunction,
                  std::function<float(const String &)> textToValueFunction)
    : state(state), paramId(std::move(paramID)), paramName(std::move(paramName)), labelText(labelText), range(std::move(range)), defaultValue(defaultVal),
      valueToTextFunction(std::move(valueToTextFunction)), textToValueFunction(std::move(textToValueFunction)) {}

    float getValue() const {
        return (float) range.convertTo0to1(getRawValue());
    }

    void setValue (float newValue) {
        float clampedNewValue = newValue < 0 ? 0 : (newValue > 1 ? 1 : newValue);
        state.getChildWithProperty(IDs::id, paramId).setProperty(IDs::value, range.convertFrom0to1(clampedNewValue), nullptr);
    }

    float getRawValue() const {
        return state.getChildWithProperty(IDs::id, paramId).getProperty(IDs::value);
    }

    Value getValueObject() {
        return state.getChildWithProperty(IDs::id, paramId).getPropertyAsValue(IDs::value, nullptr);
    }

    void attachSlider(Slider *slider, Label *label=nullptr) {
        if (label != nullptr) {
            label->setText(paramName, dontSendNotification);
        }
        slider->setNormalisableRange(range);
        slider->textFromValueFunction = valueToTextFunction;
        slider->valueFromTextFunction = textToValueFunction;
        slider->getValueObject().referTo(getValueObject());
    }

    ValueTree &state;
    const String paramId;
    const String paramName;
    const String labelText;
    NormalisableRange<double> range;
    float defaultValue;
    std::function<String(const float)> valueToTextFunction;
    std::function<float(const String &)> textToValueFunction;
};