#pragma once

#include <processors/StatefulAudioProcessor.h>
#include <ValueTreeItems.h>
#include <processors/ProcessorIds.h>
#include "push2/Push2DisplayBridge.h"
#include "AudioGraphBuilder.h"
#include "Push2ProcessorView.h"
#include "Push2ProcessorSelector.h"

class Push2Component :
        public Timer,
        public Component,
        private ProjectChangeListener {
public:
    explicit Push2Component(Project &project, AudioGraphBuilder &audioGraphBuilder)
            : project(project), audioGraphBuilder(audioGraphBuilder), processorSelector(processorIds) {
        startTimer(60);

        addChildComponent(processorView);
        addChildComponent(processorSelector);

        project.addChangeListener(this);
        setSize(Push2Display::WIDTH, Push2Display::HEIGHT);
        processorView.setBounds(getBounds());
        processorSelector.setBounds(getBounds());
    }

    ~Push2Component() override {
        project.removeChangeListener(this);
    }

    void openProcessorSelector() {
        for (auto *c : getChildren())
            c->setVisible(false);
        processorSelector.setVisible(true);
    }

    void aboveScreenButtonPressed(int buttonIndex) {
        if (processorSelector.isVisible()) {
            const String &processorId = processorSelector.selectProcessor(buttonIndex);
            if (!processorId.isEmpty()) {
                project.createAndAddProcessor(processorId);
            }
        }
    }

private:
    /*!
     *  Render a frame and send it to the Push 2 display
     */
    void inline drawFrame() {
        static const juce::Colour CLEAR_COLOR = juce::Colour(0xff000000);

        auto &g = displayBridge.getGraphics();
        g.fillAll(CLEAR_COLOR);
        paintEntireComponent(g, true);
        displayBridge.writeFrameToDisplay();
    }

    void timerCallback() override {
        //auto t1 = std::chrono::high_resolution_clock::now();
        drawFrame();
        //std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t1).count() << '\n';
    }

    void itemSelected(ValueTree item) override {
        for (auto *c : getChildren())
            c->setVisible(false);

        if (!item.isValid()) {
        } else if (item.hasType(IDs::PROCESSOR)) {
            String uuid = item[IDs::uuid];
            StatefulAudioProcessor *processor = audioGraphBuilder.getAudioProcessor(uuid);
            if (processor != nullptr) {
                processorView.setProcessor(processor);
                processorView.setVisible(true);
            }
        }
    }

    void itemRemoved(ValueTree item) override {
        if (item == project.getSelectedProcessor()) {
            itemSelected(ValueTree());
        }
    }

private:
    Push2DisplayBridge displayBridge;

    Project &project;
    AudioGraphBuilder &audioGraphBuilder;

    Push2ProcessorView processorView;
    Push2ProcessorSelector processorSelector;
};