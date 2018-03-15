#pragma once


#include <memory>

template<class T> class Command;

template <class T> class CommandObserver {
public:
    virtual ~CommandObserver() = default;
    virtual void onCommandExecuted(const T& command) = 0;
};
