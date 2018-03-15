#pragma once

#include <command_observer/CommandObserver.h>
#include <typeindex>
#include <list>
#include <unordered_map>

template<class Top> class Command {
public:
    void execute() const {
        notifyObservers(Command::observers);
    }

    static void registerObserver(std::unique_ptr<CommandObserver<Top>> observer) {
        Top::observers.push_back(std::move(observer));
    }

    void notifyObservers(const std::list<std::unique_ptr<CommandObserver<Top>>>& list) const {
        for (auto &observer : Top::observers) {
            observer->onCommandExecuted(cTop());
        }
    }

    static std::list<std::unique_ptr<CommandObserver<Top>> > observers;

protected:
    inline Top& top() const {
        return static_cast<Top&>(*this);
    }

    inline const Top& cTop() const {
        return static_cast<const Top&>(*this);
    }
};

template <class Top> std::list<std::unique_ptr<CommandObserver<Top> > > Command<Top>::observers;
