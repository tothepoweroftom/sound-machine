#include "MidiCommandObserver.h"
#include <sstream>
#include <iomanip>

void MidiCommandObserver::onCommandExecuted(const MidiCommand &command) {
    const MidiMessage& message = command.getMessage();
    if (message.isAftertouch()) {
        midiMessageStack.emplace_back("Aftertouch");
    } else if (message.isChannelPressure()) {
        midiMessageStack.emplace_back("Channel pressure");
    }
    if (message.getRawDataSize() == 3) {
        auto data = message.getRawData();
        std::ostringstream oss;
        oss << "Midi ("
            << message.getTimeStamp() << ") :"
            << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << int(data[0]) << " - "
            << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << int(data[1]) << " - "
            << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << int(data[2]);

        midiMessageStack.push_back(oss.str());
    }
    while (midiMessageStack.size() > 4) {
        midiMessageStack.pop_front();
    }

    std::string statusStr;
    for (const auto &s : midiMessageStack) {
        statusStr += s + "\n";
    }
    const MessageManagerLock lock;
    status->setText(statusStr, juce::dontSendNotification);
}
