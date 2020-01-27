#pragma once

#include <Identifiers.h>
#include <view/PluginWindow.h>
#include <processors/StatefulAudioProcessorWrapper.h>

#include "JuceHeader.h"
#include "processors/ProcessorManager.h"
#include "Project.h"
#include "state_managers/ConnectionsStateManager.h"
#include "StatefulAudioProcessorContainer.h"
#include "push2/Push2MidiCommunicator.h"

class ProcessorGraph : public AudioProcessorGraph, public StatefulAudioProcessorContainer,
                       private ValueTree::Listener, private Timer {
public:
    explicit ProcessorGraph(Project &project, ConnectionsStateManager& connectionsManager,
                            UndoManager &undoManager, AudioDeviceManager& deviceManager,
                            Push2MidiCommunicator& push2MidiCommunicator)
            : undoManager(undoManager), project(project), viewManager(project.getViewStateManager()),
            tracksManager(project.getTracksManager()), connectionsManager(connectionsManager),
            deviceManager(deviceManager), push2MidiCommunicator(push2MidiCommunicator) {
        enableAllBuses();

        project.getState().addListener(this);
        viewManager.getState().addListener(this);
        project.setStatefulAudioProcessorContainer(this);
    }

    ~ProcessorGraph() override {
        project.setStatefulAudioProcessorContainer(nullptr);
        project.getState().removeListener(this);
    }

    StatefulAudioProcessorWrapper* getProcessorWrapperForNodeId(NodeID nodeId) const override {
        if (!nodeId.isValid())
            return nullptr;

        auto nodeIdAndProcessorWrapper = processorWrapperForNodeId.find(nodeId);
        if (nodeIdAndProcessorWrapper == processorWrapperForNodeId.end())
            return nullptr;

        return nodeIdAndProcessorWrapper->second.get();
    }

    Node* getNodeForState(const ValueTree &processorState) const {
        return AudioProcessorGraph::getNodeForId(getNodeIdForState(processorState));
    }

    StatefulAudioProcessorWrapper *getMasterGainProcessor() {
        const ValueTree &gain = tracksManager.getMixerChannelProcessorForTrack(tracksManager.getMasterTrack());
        return getProcessorWrapperForState(gain);
    }

    ResizableWindow* getOrCreateWindowFor(Node* node, PluginWindow::Type type) {
        jassert(node != nullptr);

        for (auto* w : activePluginWindows)
            if (w->node->nodeID == node->nodeID && w->type == type)
                return w;

        if (auto* processor = node->getProcessor())
            return activePluginWindows.add(new PluginWindow(node, type, activePluginWindows));

        return nullptr;
    }

    bool closeAnyOpenPluginWindows() {
        bool wasEmpty = activePluginWindows.isEmpty();
        activePluginWindows.clear();
        return !wasEmpty;
    }

    AudioProcessorGraph::Node* getNodeForId(AudioProcessorGraph::NodeID nodeId) const override {
        return AudioProcessorGraph::getNodeForId(nodeId);
    }

    bool removeConnection(const Connection& c) override {
        project.removeConnection(c);
    }

    bool addConnection(const Connection& c) override {
        return connectionsManager.checkedAddConnection(c, false, getDragDependentUndoManager());
    }

    bool disconnectNode(AudioProcessorGraph::NodeID nodeId) override {
        return connectionsManager.disconnectNode(nodeId, getDragDependentUndoManager());
    }

    void setDefaultConnectionsAllowed(NodeID nodeId, bool defaultConnectionsAllowed) {
        auto processor = getProcessorStateForNodeId(nodeId);
        processor.setProperty(IDs::allowDefaultConnections, defaultConnectionsAllowed, &undoManager);
    }

    UndoManager &undoManager;
private:
    std::map<NodeID, std::unique_ptr<StatefulAudioProcessorWrapper> > processorWrapperForNodeId;

    Project &project;
    ViewStateManager &viewManager;
    TracksStateManager &tracksManager;

    ConnectionsStateManager &connectionsManager;
    AudioDeviceManager &deviceManager;
    Push2MidiCommunicator &push2MidiCommunicator;
    
    OwnedArray<PluginWindow> activePluginWindows;

    bool isMoving { false };
    bool isDeleting { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorGraph)

    UndoManager* getDragDependentUndoManager() {
        return project.getDragDependentUndoManager();
    }

    void addProcessor(const ValueTree &processorState) {
        static String errorMessage = "Could not create processor";
        auto description = project.getTypeForIdentifier(processorState[IDs::id]);
        auto processor = project.getFormatManager().createPluginInstance(*description, getSampleRate(), getBlockSize(), errorMessage);
        if (processorState.hasProperty(IDs::state)) {
            MemoryBlock memoryBlock;
            memoryBlock.fromBase64Encoding(processorState[IDs::state].toString());
            processor->setStateInformation(memoryBlock.getData(), (int) memoryBlock.getSize());
        }

        const Node::Ptr &newNode = processorState.hasProperty(IDs::nodeId) ?
                                   addNode(std::move(processor), getNodeIdForState(processorState)) :
                                   addNode(std::move(processor));
        processorWrapperForNodeId[newNode->nodeID] = std::make_unique<StatefulAudioProcessorWrapper>
                (dynamic_cast<AudioPluginInstance *>(newNode->getProcessor()), newNode->nodeID, processorState, undoManager, project.getDeviceManager());
        if (processorWrapperForNodeId.size() == 1)
            // Added the first processor. Start the timer that flushes new processor state to their value trees.
            startTimerHz(10);

        if (auto midiInputProcessor = dynamic_cast<MidiInputProcessor *>(newNode->getProcessor())) {
            const String &deviceName = processorState.getProperty(IDs::deviceName);
            midiInputProcessor->setDeviceName(deviceName);
            if (deviceName.containsIgnoreCase(push2MidiDeviceName)) {
                push2MidiCommunicator.addMidiInputCallback(&midiInputProcessor->getMidiMessageCollector());
            } else {
                deviceManager.addMidiInputCallback(deviceName, &midiInputProcessor->getMidiMessageCollector());
            }
        } else if (auto *midiOutputProcessor = dynamic_cast<MidiOutputProcessor *>(newNode->getProcessor())) {
            const String &deviceName = processorState.getProperty(IDs::deviceName);
            if (auto* enabledMidiOutput = deviceManager.getEnabledMidiOutput(deviceName))
                midiOutputProcessor->setMidiOutput(enabledMidiOutput);
        }
        ValueTree mutableProcessor = processorState;
        if (mutableProcessor.hasProperty(IDs::processorInitialized))
            mutableProcessor.sendPropertyChangeMessage(IDs::processorInitialized);
        else
            mutableProcessor.setProperty(IDs::processorInitialized, true, nullptr);

    }

    void removeProcessor(const ValueTree &processor) {
        auto* processorWrapper = getProcessorWrapperForState(processor);
        jassert(processorWrapper);
        const NodeID nodeId = getNodeIdForState(processor);

        // disconnect should have already been called before delete! (to avoid nested undo actions)
        if (processor[IDs::name] == MidiInputProcessor::name()) {
            if (auto *midiInputProcessor = dynamic_cast<MidiInputProcessor *>(processorWrapper->processor)) {
                const String &deviceName = processor.getProperty(IDs::deviceName);
                if (deviceName.containsIgnoreCase(push2MidiDeviceName)) {
                    push2MidiCommunicator.removeMidiInputCallback(&midiInputProcessor->getMidiMessageCollector());
                } else {
                    deviceManager.removeMidiInputCallback(deviceName, &midiInputProcessor->getMidiMessageCollector());
                }
            }
        }
        processorWrapperForNodeId.erase(nodeId);
        nodes.removeObject(AudioProcessorGraph::getNodeForId(nodeId));
        topologyChanged();
    }

    void recursivelyInitializeState(const ValueTree &state) {
        if (state.hasType(IDs::PROCESSOR)) {
            addProcessor(state);
            return;
        }
        for (const ValueTree& child : state) {
            recursivelyInitializeState(child);
        }
    }

    void updateIoChannelEnabled(const ValueTree& parent, const ValueTree& channel, bool enabled) {
        String processorName = parent.getParent()[IDs::name];
        bool isInput;
        if (processorName == "Audio Input" && parent.hasType(IDs::OUTPUT_CHANNELS))
            isInput = true;
        else if (processorName == "Audio Output" && parent.hasType(IDs::INPUT_CHANNELS))
            isInput = false;
        else
            return;
        if (auto* audioDevice = deviceManager.getCurrentAudioDevice()) {
            AudioDeviceManager::AudioDeviceSetup config;
            deviceManager.getAudioDeviceSetup(config);
            auto &channels = isInput ? config.inputChannels : config.outputChannels;
            const auto &channelNames = isInput ? audioDevice->getInputChannelNames() : audioDevice->getOutputChannelNames();
            auto channelIndex = channelNames.indexOf(channel[IDs::name].toString());
            if (channelIndex != -1 && ((enabled && !channels[channelIndex]) || (!enabled && channels[channelIndex]))) {
                channels.setBit(channelIndex, enabled);
                deviceManager.setAudioDeviceSetup(config, true);
            }
        }
    }

    void valueTreePropertyChanged(ValueTree& tree, const Identifier& i) override {
        if (tree.hasType(IDs::PROCESSOR)) {
            if (i == IDs::bypassed) {
                if (auto node = getNodeForState(tree)) {
                    node->setBypassed(tree[IDs::bypassed]);
                }
            } else if (i == IDs::allowDefaultConnections) {
                project.updateDefaultConnectionsForProcessor(tree, true);
            }
        } else if (i == IDs::focusedTrackIndex) {
            if (!isDeleting)
                project.resetDefaultExternalInputs(nullptr);
        }
    }

    void valueTreeChildAdded(ValueTree& parent, ValueTree& child) override {
        if (child.hasType(IDs::PROCESSOR)) {
            if (!project.isCurrentlyDraggingProcessor()) {
                if (getProcessorWrapperForState(child) == nullptr) {
                    addProcessor(child);
                }
            }
        } else if (child.hasType(IDs::CONNECTION)) {
            if (!project.isCurrentlyDraggingProcessor()) {
                const ValueTree &sourceState = child.getChildWithName(IDs::SOURCE);
                const ValueTree &destState = child.getChildWithName(IDs::DESTINATION);

                if (auto *source = getNodeForState(sourceState)) {
                    if (auto *dest = getNodeForState(destState)) {
                        int destChannel = destState[IDs::channel];
                        int sourceChannel = sourceState[IDs::channel];

                        source->outputs.add({dest, destChannel, sourceChannel});
                        dest->inputs.add({source, sourceChannel, destChannel});
                        topologyChanged();
                    }
                }
                if (child.hasProperty(IDs::isCustomConnection)) {
                    const auto& processor = getProcessorStateForNodeId(getNodeIdForState(sourceState));
                    project.updateDefaultConnectionsForProcessor(processor, true);
                }
            }
        } else if (child.hasType(IDs::TRACK)) {
            recursivelyInitializeState(child);
        } else if (child.hasType(IDs::CHANNEL)) {
            updateIoChannelEnabled(parent, child, true);
            removeIllegalConnections();
        }
    }

    void valueTreeChildRemoved(ValueTree& parent, ValueTree& child, int indexFromWhichChildWasRemoved) override {
        if (child.hasType(IDs::PROCESSOR)) {
            if (!isMoving) {
                removeProcessor(child);
            }
            for (int i = activePluginWindows.size(); --i >= 0;) {
                if (!nodes.contains(activePluginWindows.getUnchecked(i)->node)) {
                    activePluginWindows.remove(i);
                }
            }
        } else if (child.hasType(IDs::CONNECTION)) {
            if (!project.isCurrentlyDraggingProcessor()) {
                const ValueTree &sourceState = child.getChildWithName(IDs::SOURCE);
                const ValueTree &destState = child.getChildWithName(IDs::DESTINATION);

                if (auto *source = getNodeForState(sourceState)) {
                    if (auto *dest = getNodeForState(destState)) {
                        int destChannel = destState[IDs::channel];
                        int sourceChannel = sourceState[IDs::channel];

                        source->outputs.removeAllInstancesOf({dest, destChannel, sourceChannel});
                        dest->inputs.removeAllInstancesOf({source, sourceChannel, destChannel});
                        topologyChanged();
                    }
                }
            }
        } else if (child.hasType(IDs::CHANNEL)) {
            updateIoChannelEnabled(parent, child, false);
            removeIllegalConnections();
        }
    }

    void valueTreeChildWillBeMovedToNewParent(ValueTree child, ValueTree& oldParent, int oldIndex, ValueTree& newParent, int newIndex) override {
        if (child.hasType(IDs::PROCESSOR))
            isMoving = true;
    }

    void valueTreeChildHasMovedToNewParent(ValueTree child, ValueTree& oldParent, int oldIndex, ValueTree& newParent, int newIndex) override {
        if (child.hasType(IDs::PROCESSOR))
            isMoving = false;
    }

    void timerCallback() override {
        bool anythingUpdated = false;

        for (auto& nodeIdAndprocessorWrapper : processorWrapperForNodeId) {
            if (nodeIdAndprocessorWrapper.second->flushParameterValuesToValueTree())
                anythingUpdated = true;
        }

        startTimer(anythingUpdated ? 1000 / 50 : jlimit(50, 500, getTimerInterval() + 20));
    }
};
