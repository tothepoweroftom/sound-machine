#pragma once

#include <ValueTreeObjectList.h>
#include <Project.h>
#include "GraphEditorProcessor.h"
#include "GraphEditorProcessorContainer.h"
#include "ConnectorDragListener.h"
#include "ProcessorGraph.h"
#include "GraphEditorPin.h"

class GraphEditorProcessors : public Component,
                              public Utilities::ValueTreeObjectList<GraphEditorProcessor>,
                              public GraphEditorProcessorContainer {
public:
    explicit GraphEditorProcessors(Project& project, ValueTree& state, ConnectorDragListener &connectorDragListener, ProcessorGraph& graph)
            : Utilities::ValueTreeObjectList<GraphEditorProcessor>(state),
              project(project), connectorDragListener(connectorDragListener), graph(graph) {
        rebuildObjects();
    }

    ~GraphEditorProcessors() override {
        freeObjects();
    }

    bool isMasterTrack() const {
        return parent.hasType(IDs::MASTER_TRACK);
    }

    void mouseDown(const MouseEvent &e) override {
        select();
        if (e.mods.isPopupMenu()) {
            showPopupMenu(e.position.toInt());
        }
    }

    void resized() override {
        auto r = getLocalBounds();
        int totalSize = isMasterTrack() ? getWidth() : getHeight();

        int mixerChannelSize = isMasterTrack() ? getHeight() : int(totalSize * MIXER_CHANNEL_SLOT_RATIO);
        for (int slot = 0; slot < Project::NUM_AVAILABLE_PROCESSOR_SLOTS; slot++) {
            cellSizes[slot] = (slot != Project::NUM_AVAILABLE_PROCESSOR_SLOTS - 1)
                              ? (totalSize - mixerChannelSize) / (Project::NUM_AVAILABLE_PROCESSOR_SLOTS - 1)
                              : mixerChannelSize;
        }

        for (int slot = 0; slot < Project::NUM_AVAILABLE_PROCESSOR_SLOTS; slot++) {
            auto processorBounds = isMasterTrack() ? r.removeFromLeft(cellSizes[slot]) : r.removeFromTop(cellSizes[slot]);
            if (auto *processor = findProcessorAtSlot(slot)) {
                processor->setBounds(processorBounds);
            }
        }
    }

    void paint(Graphics &g) override {
        auto r = getLocalBounds();
        g.setColour(findColour(ResizableWindow::backgroundColourId).brighter(0.15));
        for (int cellSize : cellSizes)
            g.drawRect(isMasterTrack() ? r.removeFromLeft(cellSize) : r.removeFromTop(cellSize));
    }

    bool isSuitableType(const ValueTree &v) const override {
        return v.hasType(IDs::PROCESSOR);
    }

    GraphEditorProcessor *createNewObject(const ValueTree &v) override {
        GraphEditorProcessor *processor = currentlyMovingProcessor != nullptr ? currentlyMovingProcessor : new GraphEditorProcessor(v, connectorDragListener, graph);
        addAndMakeVisible(processor);
        processor->update();
        return processor;
    }

    void deleteObject(GraphEditorProcessor *processor) override {
        if (currentlyMovingProcessor == nullptr) {
            delete processor;
        } else {
            removeChildComponent(processor);
        }
    }

    void newObjectAdded(GraphEditorProcessor *) override { resized(); }

    void objectRemoved(GraphEditorProcessor *object) override {
        resized();
        if (object == mostRecentlySelectedProcessor) {
            mostRecentlySelectedProcessor = nullptr;
            select();
        }
    }

    void objectOrderChanged() override { resized(); }

    GraphEditorProcessor *getProcessorForNodeId(AudioProcessorGraph::NodeID nodeId) const override {
        for (auto *processor : objects) {
            if (processor->getNodeId() == nodeId) {
                return processor;
            }
        }
        return nullptr;
    }

    GraphEditorPin *findPinAt(const MouseEvent &e) {
        for (auto *processor : objects) {
            if (auto* pin = processor->findPinAt(e)) {
                return pin;
            }
        }
        return nullptr;
    }

    int findSlotAt(const MouseEvent &e) {
        const MouseEvent &relative = e.getEventRelativeTo(this);
        return findSlotAt(relative.getPosition());
    }
    
    void update() {
        for (auto *processor : objects) {
            processor->update();
        }
    }

    GraphEditorProcessor *getCurrentlyMovingProcessor() const {
        return currentlyMovingProcessor;
    }

    void setCurrentlyMovingProcessor(GraphEditorProcessor *currentlyMovingProcessor) {
        this->currentlyMovingProcessor = currentlyMovingProcessor;
    }

    bool anySelected() const {
        for (auto *processor : objects) {
            if (processor->isSelected())
                return true;
        }
        return false;
    }

    void select() {
        if (mostRecentlySelectedProcessor != nullptr) {
            mostRecentlySelectedProcessor->setSelected(true);
        } else if (auto* firstProcessor = objects.getFirst()) {
            firstProcessor->setSelected(true);
        }
    }

private:
    const float MIXER_CHANNEL_SLOT_RATIO = 0.2f;

    Project &project;
    ConnectorDragListener &connectorDragListener;
    ProcessorGraph &graph;
    std::unique_ptr<PopupMenu> menu;
    GraphEditorProcessor *currentlyMovingProcessor {};
    GraphEditorProcessor* mostRecentlySelectedProcessor {};

    int cellSizes[Project::NUM_AVAILABLE_PROCESSOR_SLOTS];

    void valueTreePropertyChanged(ValueTree &v, const Identifier &i) override {
        if (isSuitableType(v)) {
            if (i == IDs::processorSlot)
                resized();
            else if (i == IDs::selected && v[IDs::selected])
                mostRecentlySelectedProcessor = findObjectWithState(v);
        }

        Utilities::ValueTreeObjectList<GraphEditorProcessor>::valueTreePropertyChanged(v, i);
    }
    
    GraphEditorProcessor* findProcessorAtSlot(int slot) const {
        for (auto* processor : objects) {
            if (processor->getSlot() == slot) {
                return processor;
            }
        }
        return nullptr;
    }

    void showPopupMenu(const Point<int> &mousePos) {
        int slot = findSlotAt(mousePos);
        menu = std::make_unique<PopupMenu>();
        project.addPluginsToMenu(*menu, parent);
        menu->showMenuAsync({}, ModalCallbackFunction::create([this, slot](int r) {
            if (auto *description = project.getChosenType(r)) {
                project.createAndAddProcessor(*description, parent, slot);
            }
        }));
    }

    int getCellSize() const {
        return (isMasterTrack() ? getWidth() : getHeight()) / Project::NUM_AVAILABLE_PROCESSOR_SLOTS;
    }

    int findSlotAt(const Point<int> relativePosition) {
        int slot = isMasterTrack() ? relativePosition.x / getCellSize() : relativePosition.y / getCellSize();
        return jlimit(0, Project::NUM_AVAILABLE_PROCESSOR_SLOTS - 1, slot);
    }
};
