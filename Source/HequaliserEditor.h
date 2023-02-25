/*
  ==============================================================================

 This is the Frequalizer UI editor

  ==============================================================================
*/

#pragma once

#include "HequaliserProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

using namespace juce;

class MyTask  : public juce::ThreadWithProgressWindow
{
public:
    MyTask()    : juce::ThreadWithProgressWindow ("busy...", true, true)
    {
        
    }
    
    String getResultText (const URL& url)
    {
        juce::StringPairArray responseHeaders;
        int statusCode = 0;

        if (auto stream = url.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                                                                                 .withConnectionTimeoutMs(0)
                                                                                 .withResponseHeaders (&responseHeaders)
                                                                                 .withStatusCode (&statusCode)
                                                                                 .withExtraHeaders("Authorization: Bearer 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")
                                                 ))
        {
//            return (statusCode != 0 ? "Status code: " + String (statusCode) + newLine : String())
//                    + "Response headers: " + newLine
//                    + responseHeaders.getDescription() + newLine
//                    + "----------------------------------------------------" + newLine
//                    + stream->readEntireStreamAsString();
            return stream->readEntireStreamAsString();
        }

        if (statusCode != 0)
            return "Failed to connect, status code = " + String (statusCode);

        return "Failed to connect!";
    }
    
    var getHeadphoneSettings(URL url, const StringPairArray & fileIDs)
    {                
        auto json = getResultText(url);
                
        var inputJSON;
        
        auto parsedObject = new DynamicObject();
                    
        if (JSON::parse (json, inputJSON).wasOk())
            if (auto * obj = inputJSON.getDynamicObject())
                if(auto * data = obj->getProperty("data").getDynamicObject())
                    if(auto * repo = data->getProperty("repository").getDynamicObject())
                        for(auto & file : repo->getProperties())
                            parsedObject->setProperty(fileIDs[file.name], file.value);
        
        return var(parsedObject);
        
        String result;
                                                                                            
        auto lineTokens = StringArray::fromTokens(result, ":,", "");
        auto contentResult = lineTokens[lineTokens.indexOf("\"content\"")+1];
        contentResult = contentResult.unquoted();
        contentResult = contentResult.removeCharacters("\\n");
                    
        MemoryOutputStream decodedStream;
        
        auto resultList = StringArray::fromLines(decodedStream.toString());
        
        auto headphoneSetting = new DynamicObject();

        for(auto line : resultList)
        {
            auto lineTokens = StringArray::fromTokens(line, " :", "");

            if(lineTokens[0] == "Preamp")
                headphoneSetting->setProperty("Preamp", lineTokens[2]);

            if(lineTokens[0] == "Filter")
            {
                DynamicObject* filterSetting = new DynamicObject();

                filterSetting->setProperty("Type", lineTokens[lineTokens.indexOf("ON")+1]);
                filterSetting->setProperty("Fc", lineTokens[lineTokens.indexOf("Fc")+1]);
                filterSetting->setProperty("Gain", lineTokens[lineTokens.indexOf("Gain")+1]);
                filterSetting->setProperty("Q", lineTokens[lineTokens.indexOf("Q")+1]);
                                                                                    
                headphoneSetting->setProperty(lineTokens[1], var(filterSetting));
            }
        }
                        
        return headphoneSetting;
    }
     
    void run()
    {
        URL rootURL("https://raw.githubusercontent.com/jaakkopasanen/AutoEq/master/results/oratory1990/");
                        
        auto result = getResultText(rootURL.getChildURL("README.md"));
        
        //TODO: Some error handling
        if(result.startsWith("Failed"))
            return;
        
        result = result.fromLastOccurrenceOf("----------------------------------------------------", false, false);
        auto resultList = StringArray::fromLines(result);
                
        if(resultList[0].startsWith("Failed"))
           return;
            
        auto postData = String::formatted(R"({"query": "query {repository(owner: \"jaakkopasanen\", name: \"AutoEq\") {)");
        
        StringPairArray fileIDs;
                                                                    
        for (int i = 0; i < resultList.size();++i)
        {
            // must check this as often as possible, because this is
            // how we know if the user's pressed 'cancel'
            if (threadShouldExit())
                break;
            
            auto & line = resultList[i];
                                
            if(!line.startsWith("- "))
                continue;
            
            auto lineTokens = StringArray::fromTokens(line, "[]/", "");
            
            auto headphoneName = lineTokens[1];
            auto headphoneLink = lineTokens[3];
            
            String fileID("h");
            fileID += String(i);
                                                        
            if(headphoneName.isEmpty())
                continue;
            
            fileIDs.set(fileID, headphoneName);
                                                                      
            postData += String::formatted(R"(%s: object(expression: \"master:results/oratory1990/%s/%s/%s FixedBandEQ.txt\") { ... on Blob { text } } )",fileID.toRawUTF8(), headphoneLink.toRawUTF8(), headphoneName.toRawUTF8(), headphoneName.toRawUTF8());
                                                                                                                    
            // this will update the progress bar on the dialog box
            setProgress (i / (double) resultList.size());
  
            //   ... do the business here...
        }
        
        postData += R"(}}"})";
                            
        URL headphoneSettingURL("https://api.github.com/graphql"), headphone;
        headphoneSettingURL = headphoneSettingURL.withPOSTData(postData);
                
        auto headphoneSettings = getHeadphoneSettings(headphoneSettingURL, fileIDs);
        
        auto file = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
        file = file.getChildFile("Hequaliser/headphoneNames.txt");
        file.replaceWithText("");
        FileOutputStream fileOutputStream(file);
        
        JSON::writeToStream(fileOutputStream, headphoneSettings);
    }
};

//==============================================================================
/**
*/
class FrequalizerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         public juce::ChangeListener,
                                         public juce::Timer
{
public:
    FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor&);
    ~FrequalizerAudioProcessorEditor() override;

    //==============================================================================

    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* sender) override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseDoubleClick (const juce::MouseEvent& e) override;

    //==============================================================================

    class BandEditor : public juce::Component,
                       public juce::Button::Listener
    {
    public:
        BandEditor (size_t i, FrequalizerAudioProcessor& processor);

        void resized () override;

        void updateControls (FrequalizerAudioProcessor::FilterType type);

        void updateSoloState (bool isSolo);

        void setFrequency (float frequency);

        void setGain (float gain);

        void setType (int type);

        void buttonClicked (juce::Button* b) override;

        juce::Path frequencyResponse;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandEditor)

        size_t index;
        FrequalizerAudioProcessor& processor;

        juce::GroupComponent      frame;
        juce::ComboBox            filterType;
        juce::Slider              frequency { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Slider              quality   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Slider              gain      { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::TextButton          solo      { TRANS ("S") };
        juce::TextButton          activate  { TRANS ("A") };
        juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachments;
        juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
        juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    };

private:

    void updateFrequencyResponses ();

    static float getPositionForFrequency (float freq);

    static float getFrequencyForPosition (float pos);

    static float getPositionForGain (float gain, float top, float bottom);

    static float getGainForPosition (float pos, float top, float bottom);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FrequalizerAudioProcessor& freqProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessorEditor)

#ifdef JUCE_OPENGL
    juce::OpenGLContext     openGLContext;
#endif

    juce::OwnedArray<BandEditor>  bandEditors;

    juce::Rectangle<int>          plotFrame;
    juce::Rectangle<int>          brandingFrame;

    juce::Path                    frequencyResponse;
    juce::Path                    analyserPath;

    juce::GroupComponent          frame;
    juce::Slider                  output { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::ComboBox                headphoneType;

    SocialButtons                 socialButtons;

    int                           draggingBand = -1;
    bool                          draggingGain = false;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment* headphoneTypeAttachment;
    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;

    juce::PopupMenu               contextMenu;
};
