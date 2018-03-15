#pragma once

#include <deque>
#include "command/MidiCommand.h"
#include "CommandObserver.h"

class MidiCommandObserver: public CommandObserver<MidiCommand> {
public:
    MidiCommandObserver() = delete;
    explicit MidiCommandObserver(Label *status) : status(status) {};

    void onCommandExecuted(const MidiCommand& command) override;
private:
    std::deque<std::string> midiMessageStack;
    Label* status;
};
