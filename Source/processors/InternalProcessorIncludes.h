#pragma once

#include "BalanceProcessor.h"
#include "GainProcessor.h"
#include "MidiInputProcessor.h"
#include "MixerChannelProcessor.h"
#include "SineBank.h"
#include "SineSynth.h"

Array<PluginDescription> internalPluginDescriptions {
        BalanceProcessor::getPluginDescription(),
        GainProcessor::getPluginDescription(),
        MidiInputProcessor::getPluginDescription(),
        MixerChannelProcessor::getPluginDescription(),
        SineBank::getPluginDescription(),
        SineSynth::getPluginDescription(),
};