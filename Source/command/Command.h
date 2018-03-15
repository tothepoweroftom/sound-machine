#pragma once

#include <command_observer/CommandObserver.h>
#include <typeindex>
#include <list>
#include <unordered_map>

template<class T> class Command {
public:
    virtual ~Command() = default;
    virtual void execute() const {
        notifyObservers();
    }

    static void registerObserver(std::unique_ptr<CommandObserver<T>> observer) {
        T::observers.push_back(std::move(observer));
    }

    void notifyObservers() const {
        for (auto &observer : T::observers) {
            observer->onCommandExecuted(static_cast<const T&>(*this));
        }
    }

    static std::list<std::unique_ptr<CommandObserver<T>> > observers;
};

template <class T> std::list<std::unique_ptr<CommandObserver<T> > > Command<T>::observers;
