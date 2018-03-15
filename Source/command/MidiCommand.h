#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Command.h"
#include "StatefulCommand.h"
#include <utility>

class MidiCommand : public Command<MidiCommand> {
    typedef Command super;
public:
    explicit MidiCommand(MidiMessage message): message(std::move(message)) {}

    virtual void execute() const {
        super::execute();
        notifyObservers();
    }

    const MidiMessage& getMessage() const {
        return message;
    }
protected:
    MidiMessage message;
};
