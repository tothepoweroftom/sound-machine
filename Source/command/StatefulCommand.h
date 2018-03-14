#pragma once

#include <typeindex>
#include "Command.h"

class StatefulCommand : public Command {
    typedef Command super;

public:
    virtual void execute() const {
        super::execute();
        notifyObservers(*this, StatefulCommand::getType());
    }

    virtual void revert() const {};

    static std::type_index getType() {
        return typeid(StatefulCommand);
    }
};
