#pragma once

#include <typeindex>
#include "Command.h"

template<class Top>
class StatefulCommandImpl : public Command<Top> {
protected:
    using parent = Command<Top>;
    friend parent;

public:
    void execute() const;

    virtual void revert() const {};
};

struct StatefulCommand : StatefulCommandImpl<StatefulCommand> {};

template<class Top> void StatefulCommandImpl<Top>::execute() const {
    parent::execute();
    parent::notifyObservers(StatefulCommand::observers);
}