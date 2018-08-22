#include <memory>

#pragma once

#include "JuceHeader.h"
#include "processors/StatefulAudioProcessorWrapper.h"
#include "SwitchParameterComponent.h"
#include "ParametersPanel.h"

class ProcessorEditor : public Component {
public:
    explicit ProcessorEditor(int maxRows=2) :
            pageLeftButton("Page left", 0.5, findColour(ResizableWindow::backgroundColourId).brighter(0.75)),
            pageRightButton("Page right", 0.0, findColour(ResizableWindow::backgroundColourId).brighter(0.75)) {
        addAndMakeVisible(titleLabel);
        addAndMakeVisible(pageLeftButton);
        addAndMakeVisible(pageRightButton);
        addAndMakeVisible((parametersPanel = std::make_unique<ParametersPanel>(maxRows)).get());

        parametersPanel->setBackgroundColour(findColour(ResizableWindow::backgroundColourId).brighter(0.1));
        parametersPanel->setOutlineColour(findColour(TextEditor::backgroundColourId));

        titleLabel.setColour(findColour(TextEditor::textColourId));
        titleLabel.setJustification(Justification::verticallyCentred);
        titleLabel.setFontHeight(18);

        pageLeftButton.onClick = [this]() {
            parametersPanel->pageLeft();
            updatePageButtonVisibility();
        };

        pageRightButton.onClick = [this]() {
            parametersPanel->pageRight();
            updatePageButtonVisibility();
        };
    }

    void paint(Graphics& g) override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(40);
        g.setColour(findColour(TextEditor::backgroundColourId));
        g.fillRect(top);
    }

    void resized() override {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(40);
        top.removeFromLeft(8);
        titleLabel.setBoundingBox(top.removeFromLeft(200).reduced(2).toFloat());

        top.reduce(4, 4);
        pageRightButton.setBounds(top.removeFromRight(30).reduced(5));
        pageLeftButton.setBounds(top.removeFromRight(30).reduced(5));

        parametersPanel->setBounds(r);
    }

    void setProcessor(StatefulAudioProcessorWrapper *const processorWrapper) {
        if (processorWrapper != nullptr) {
            titleLabel.setText(processorWrapper->processor->getName());
        }
        parametersPanel->setProcessor(processorWrapper);
        parametersPanel->resized();
        updatePageButtonVisibility();
    }

    void updatePageButtonVisibility() {
        pageLeftButton.setVisible(parametersPanel->canPageLeft());
        pageRightButton.setVisible(parametersPanel->canPageRight());
    }

private:
    std::unique_ptr<ParametersPanel> parametersPanel;
    DrawableText titleLabel;
    ArrowButton pageLeftButton, pageRightButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
};
