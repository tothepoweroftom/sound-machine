#pragma once

#include "JuceHeader.h"
#include "Push2ComponentBase.h"
#include "view/processor_editor/ParametersPanel.h"

class Push2TrackManagingView : public Push2ComponentBase, protected ValueTree::Listener {
public:
    explicit Push2TrackManagingView(Project &project, Push2MidiCommunicator &push2)
            : Push2ComponentBase(project, push2) {
        project.getState().addListener(this);

        for (int i = 0; i < NUM_COLUMNS; i++) {
            addChildComponent(trackLabels.add(new Push2Label(i, false, push2)));
        }
    }

    ~Push2TrackManagingView() override {
        push2.getPush2Colours().removeListener(this);
        project.getState().removeListener(this);
    }

    void resized() override {
        auto r = getLocalBounds();
        auto labelWidth = getWidth() / NUM_COLUMNS;
        auto bottom = r.removeFromBottom(HEADER_FOOTER_HEIGHT);
        for (auto* trackLabel : trackLabels) {
            trackLabel->setBounds(bottom.removeFromLeft(labelWidth));
        }
    }

    void belowScreenButtonPressed(int buttonIndex) override {
        auto track = tracksManager.getTrackWithViewIndex(buttonIndex);
        if (track.isValid() && !TracksStateManager::isMasterTrack(track)) {
            tracksManager.setTrackSelected(track, true);
        }
    }

    void updateEnabledPush2Buttons() override {
        auto focusedTrack = tracksManager.getFocusedTrack();
        for (auto* label : trackLabels) {
            label->setVisible(false);
        }
        if (!isVisible())
            return;
        int labelIndex = 0;
        for (int i = 0; i < jmin(trackLabels.size(), tracksManager.getNumNonMasterTracks()); i++) {
            auto *label = trackLabels.getUnchecked(labelIndex++);
            const auto& track = tracksManager.getTrackWithViewIndex(i);
            // TODO left/right buttons
            label->setVisible(true);
            label->setMainColour(tracksManager.getTrackColour(track));
            label->setText(track[IDs::name], dontSendNotification);
            label->setSelected(track == focusedTrack);
        }
    }

protected:
    virtual void trackAdded(const ValueTree &track) { updateEnabledPush2Buttons(); }
    virtual void trackRemoved(const ValueTree &track) { updateEnabledPush2Buttons(); }
    virtual void trackSelected(const ValueTree &track) { updateEnabledPush2Buttons(); }

    void valueTreeChildAdded(ValueTree &parent, ValueTree& child) override {
        if (child.hasType(IDs::TRACK)) {
            trackAdded(child);
        }
    }

    void valueTreeChildRemoved(ValueTree &exParent, ValueTree& child, int index) override {
        if (child.hasType(IDs::TRACK)) {
            trackRemoved(child);
        }
    }

    void valueTreePropertyChanged(ValueTree &tree, const Identifier &i) override {
        if (tree.hasType(IDs::TRACK)) {
            if (i == IDs::name || i == IDs::colour) {
                int trackIndex = tracksManager.getViewIndexForTrack(tree);
                if (trackIndex < 0 || trackIndex >= trackLabels.size())
                    return;
                if (i == IDs::name && !TracksStateManager::isMasterTrack(tree)) {
                    trackLabels.getUnchecked(trackIndex)->setText(tree[IDs::name], dontSendNotification);
                }
            } else if (i == IDs::selected && tree[IDs::selected]) {
                trackSelected(tree);
            }
        } else if (i == IDs::gridViewTrackOffset) {
            updateEnabledPush2Buttons();
        }
    }

    void valueTreeChildOrderChanged(ValueTree &tree, int, int) override {
        if (tree.hasType(IDs::TRACKS)) {
            updateEnabledPush2Buttons();
        }
    }

    void valueTreeParentChanged(ValueTree &) override {}

    void valueTreeRedirected(ValueTree &) override {}

protected:
    void trackColourChanged(const String &trackUuid, const Colour &colour) override {
        auto track = tracksManager.findTrackWithUuid(trackUuid);
        if (!TracksStateManager::isMasterTrack(track)) {
            if (auto *trackLabel = trackLabels[tracksManager.getViewIndexForTrack(track)]) {
                trackLabel->setMainColour(colour);
            }
        }
    }

private:
    OwnedArray<Push2Label> trackLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Push2TrackManagingView)
};
