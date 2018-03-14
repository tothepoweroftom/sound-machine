#pragma once


#include <stack>
#include <command/StatefulCommand.h>
#include "command/Command.h"

class CommandHistoryManager : public CommandObserver {
public:
    void onCommandExecuted(const Command& command) override {
        const auto& statefulCommand = dynamic_cast<const StatefulCommand&>(command);
        undoStack.push(std::move(std::make_unique<StatefulCommand>(statefulCommand)));
    }

private:
    std::stack<std::unique_ptr<StatefulCommand> > undoStack;
};
