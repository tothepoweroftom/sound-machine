#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Command.h"
#include "StatefulCommand.h"
#include <utility>



template<class Top>
class MidiCommandImpl : public StatefulCommandImpl<Top> {
protected:
    using parent = StatefulCommandImpl<Top>;
    friend parent;

public:
    explicit MidiCommandImpl(const MidiMessage message): message(std::move(message)) {}

    void execute() const;

    const MidiMessage& getMessage() const {
        return message;
    }
protected:
    MidiMessage message;
};

struct MidiCommand : MidiCommandImpl<MidiCommand> {
    explicit MidiCommand(const MidiMessage &message) : MidiCommandImpl(message) {}
};

template<class Top> void MidiCommandImpl<Top>::execute() const {
    parent::execute();
    parent::notifyObservers(MidiCommand::observers);
}