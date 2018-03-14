#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Command.h"
#include "StatefulCommand.h"
#include <utility>

class MidiCommand : public Command {
    typedef Command super;
public:
    explicit MidiCommand(MidiMessage message): message(std::move(message)) {}

    virtual void execute() const {
        super::execute();
        notifyObservers(*this, MidiCommand::getType());
    }

    static std::type_index getType() {
        return typeid(MidiCommand);
    }

    const MidiMessage& getMessage() const {
        return message;
    }
protected:
    MidiMessage message;
};
