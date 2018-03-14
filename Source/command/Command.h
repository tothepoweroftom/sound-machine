#pragma once

#include <command_observer/CommandObserver.h>
#include <typeindex>
#include <list>
#include <unordered_map>

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() const {
        notifyObservers(*this, Command::getType());
    }

    static std::type_index getType() {
        return typeid(Command);
    }

    static void registerObserver(std::unique_ptr<CommandObserver> observer, std::type_index commandType) {
        observersForCommandClass[commandType].push_back(std::move(observer));
    }

    static void notifyObservers(const Command &command, const std::type_index type) {
        auto &listeners = observersForCommandClass[type];
        for (auto &listener : listeners) {
            listener->onCommandExecuted(command);
        }
    }

    static std::unordered_map<std::type_index, std::list<std::unique_ptr<CommandObserver> > > observersForCommandClass;
};
