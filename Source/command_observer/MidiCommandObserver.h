#pragma once

#include <deque>
#include "command/MidiCommand.h"
#include "CommandObserver.h"

class MidiCommandObserver: public CommandObserver {
public:
    MidiCommandObserver() = delete;
    explicit MidiCommandObserver(Label *status) : status(status) {};

    void onCommandExecuted(const Command& command) override;
private:
    std::deque<std::string> midiMessageStack;
    Label* status;
};
