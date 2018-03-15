#pragma once

#include <typeindex>
#include "Command.h"

class StatefulCommand : public Command<StatefulCommand> {
    typedef Command super;

public:
    virtual void execute() const {
        super::execute();
        notifyObservers();
    }

    virtual void revert() const {};
};
