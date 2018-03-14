#pragma once


#include <memory>

class Command;

class CommandObserver {
public:
    virtual ~CommandObserver() = default;
    virtual void onCommandExecuted(const Command& command) = 0;
};
