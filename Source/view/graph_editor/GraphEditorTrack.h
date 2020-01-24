#pragma once

#include <Utilities.h>
#include <Identifiers.h>
#include "JuceHeader.h"
#include "GraphEditorProcessors.h"

class GraphEditorTrack : public Component, public Utilities::ValueTreePropertyChangeListener, public GraphEditorProcessorContainer, private ChangeListener {
public:
    explicit GraphEditorTrack(Project& project, const ValueTree& state, ConnectorDragListener &connectorDragListener, ProcessorGraph& graph)
            : tracksManager(project.getTracksManager()), state(state),
              viewState(project.getViewState()), connectorDragListener(connectorDragListener), graph(graph),
              processors(project, state, connectorDragListener, graph) {
        nameLabel.setJustificationType(Justification::centred);
        updateLabelColour();
        addAndMakeVisible(nameLabel);
        addAndMakeVisible(processors);
        if (!isMasterTrack()) {
            nameLabel.setText(getTrackName(), dontSendNotification);
            nameLabel.setEditable(false, true);
            nameLabel.onTextChange = [this] { this->state.setProperty(IDs::name, nameLabel.getText(false), &this->graph.undoManager); };
        } else {
            masterTrackName.setText(getTrackName());
            masterTrackName.setColour(findColour(TextEditor::textColourId));
            masterTrackName.setJustification(Justification::centred);
            addAndMakeVisible(masterTrackName);
        }
        nameLabel.addMouseListener(this, false);
        this->state.addListener(this);
        viewState.addListener(this);
    }

    ~GraphEditorTrack() override {
        nameLabel.removeMouseListener(this);
        viewState.removeListener(this);
        this->state.removeListener(this);
    }

    void mouseDown(const MouseEvent &e) override {
        if (e.eventComponent == &nameLabel) {
            if (e.mods.isPopupMenu()) {
                auto *colourSelector = new ColourSelector();
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
                colourSelector->setSize(300, 400);

                CallOutBox::launchAsynchronously(colourSelector, getScreenBounds(), nullptr);
            } else {
                setSelected(!(isSelected() && e.mods.isCommandDown()), !e.mods.isCommandDown());
            }
        }
    }

    const ValueTree& getState() const { return state; }

    bool isMasterTrack() const { return TracksStateManager::isMasterTrack(state); }

    int getTrackIndex() const { return state.getParent().indexOf(state); }

    int getTrackViewIndex() const { return tracksManager.getViewIndexForTrack(state); }

    String getTrackName() const { return state[IDs::name].toString(); }

    Colour getColour() const {
        const Colour &trackColour = tracksManager.getTrackColour(state);
        return isSelected() ? trackColour.brighter(0.25) : trackColour;
    }

    void setColour(const Colour& colour) { state.setProperty(IDs::colour, colour.toString(), &graph.undoManager); }

    bool isSelected() const { return state.getProperty(IDs::selected); }

    void setSelected(bool selected, bool deselectOthers=true) {
        tracksManager.setTrackSelected(state, selected, deselectOthers);
    }

    const Label *getNameLabel() const { return &nameLabel; }

    const Component *getDragControlComponent() const { return getNameLabel(); }

    void resized() override {
        auto r = getLocalBounds();
        processors.setBounds(r);
        const auto &nameLabelBounds = isMasterTrack()
                                      ? r.removeFromLeft(TracksStateManager::TRACK_LABEL_HEIGHT)
                                         .withX(tracksManager.getTrackWidth() * processors.getSlotOffset())
                                      : r.removeFromTop(TracksStateManager::TRACK_LABEL_HEIGHT)
                                         .withY(tracksManager.getProcessorHeight() * processors.getSlotOffset());
        nameLabel.setBounds(nameLabelBounds);
        nameLabel.toFront(false);
        if (isMasterTrack()) {
            const auto& labelBoundsFloat = nameLabelBounds.toFloat();
            masterTrackName.setBoundingBox(Parallelogram<float>(labelBoundsFloat.getBottomLeft(), labelBoundsFloat.getTopLeft(), labelBoundsFloat.getBottomRight()));
            masterTrackName.setFontHeight(3 * TracksStateManager::TRACK_LABEL_HEIGHT / 4);
            masterTrackName.toFront(false);
        }
    }

    GraphEditorProcessor *getProcessorForNodeId(const AudioProcessorGraph::NodeID nodeId) const override {
        if (auto *currentlyMovingProcessor = processors.getCurrentlyMovingProcessor()) {
            if (currentlyMovingProcessor->getNodeId() == nodeId) {
                return currentlyMovingProcessor;
            }
        }
        return processors.getProcessorForNodeId(nodeId);
    }

    GraphEditorPin *findPinAt(const MouseEvent &e) const {
        return processors.findPinAt(e);
    }

    int findSlotAt(const MouseEvent &e) {
        return processors.findSlotAt(e);
    }

    void setCurrentlyMovingProcessor(GraphEditorProcessor *currentlyMovingProcessor) {
        processors.setCurrentlyMovingProcessor(currentlyMovingProcessor);
    }

private:
    TracksStateManager& tracksManager;

    ValueTree state, viewState;

    Label nameLabel;
    DrawableText masterTrackName;
    ConnectorDragListener &connectorDragListener;
    ProcessorGraph &graph;
    GraphEditorProcessors processors;

    void updateLabelColour() {
        nameLabel.setColour(Label::backgroundColourId, getColour());
    }

    void valueTreePropertyChanged(ValueTree &tree, const Identifier &i) override {
        if (i == IDs::gridViewSlotOffset || ((i == IDs::gridViewTrackOffset || i == IDs::masterViewSlotOffset) && isMasterTrack())) {
            resized();
        }

        if (tree != state)
            return;

        if (i == IDs::name) {
            nameLabel.setText(tree[IDs::name].toString(), dontSendNotification);
        } else if (i == IDs::colour || i == IDs::selected) {
            updateLabelColour();
        }
    }

    void changeListenerCallback(ChangeBroadcaster *source) override {
        if (auto *cs = dynamic_cast<ColourSelector *> (source)) {
            setColour(cs->getCurrentColour());
        }
    }
};
