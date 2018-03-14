#include "Command.h"

std::unordered_map<std::type_index, std::list<std::unique_ptr<CommandObserver> > > Command::observersForCommandClass;
