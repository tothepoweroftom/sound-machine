#pragma once


#include <stack>
#include <command/StatefulCommand.h>
#include "command/Command.h"

class CommandHistoryManager : public CommandObserver<StatefulCommand> {
public:
    void onCommandExecuted(const StatefulCommand& command) override {
        undoStack.push(std::move(std::make_unique<StatefulCommand>(command)));
    }

private:
    std::stack<std::unique_ptr<StatefulCommand> > undoStack;
};
