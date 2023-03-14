/*
  ==============================================================================

 This is the Frequalizer UI editor

  ==============================================================================
*/

#pragma once

#include "HequaliserProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

class MyTask  : public juce::ThreadWithProgressWindow
{
public:
    MyTask()    : juce::ThreadWithProgressWindow ("busy...", true, true)
    {
        
    }
    
    juce::String getResultText (const juce::URL& url)
    {
        juce::StringPairArray responseHeaders;
        int statusCode = 0;
        
        juce::MemoryOutputStream decodedStream;
        juce::Base64::convertFromBase64(decodedStream, "Z2hwX3JsVldkMVhDZW5qSlhjUDN4UmxLSWNzM1FLT2tHQzJiOHdSRw==");
        auto decodedKey = decodedStream.toString();

        if (auto stream = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                                                                 .withConnectionTimeoutMs(0)
                                                                                 .withResponseHeaders (&responseHeaders)
                                                                                 .withStatusCode (&statusCode)
                                                                                 .withExtraHeaders("Authorization: Bearer " + decodedKey)
                                                 ))
        {
//            return (statusCode != 0 ? "Status code: " + juce::String (statusCode) + newLine : juce::String())
//                    + "Response headers: " + newLine
//                    + responseHeaders.getDescription() + newLine
//                    + "----------------------------------------------------" + newLine
//                    + stream->readEntireStreamAsString();
            return stream->readEntireStreamAsString();
        }

        if (statusCode != 0)
            return "Failed to connect, status code = " + juce::String (statusCode);

        return "Failed to connect!";
    }
    
    juce::DynamicObject* getHeadphoneSetting(juce::String headphoneSettingString)
    {
        auto headphoneSetting = new juce::DynamicObject();
        
        for(auto line : juce::StringArray::fromLines(headphoneSettingString))
        {
            auto lineTokens = juce::StringArray::fromTokens(line, " :", "");

            if(lineTokens[0] == "Preamp")
                headphoneSetting->setProperty("Preamp", lineTokens[2]);

            if(lineTokens[0] == "Filter")
            {                                    
                auto filterSetting = new juce::DynamicObject();

                filterSetting->setProperty("Type", lineTokens[lineTokens.indexOf("ON")+1]);
                filterSetting->setProperty("Fc", lineTokens[lineTokens.indexOf("Fc")+1]);
                filterSetting->setProperty("Gain", lineTokens[lineTokens.indexOf("Gain")+1]);
                filterSetting->setProperty("Q", lineTokens[lineTokens.indexOf("Q")+1]);
                                                                                    
                headphoneSetting->setProperty(lineTokens[1], filterSetting);
            }
        }
        

                        
        return headphoneSetting;
    }    
    
    juce::var getHeadphoneSettings(juce::URL url, const juce::StringPairArray & fileIDs)
    {
        auto json = getResultText(url);
            
        juce::var inputJSON;
        
        auto parsedObject = new juce::DynamicObject();
        
        if (juce::JSON::parse (json, inputJSON).wasOk())
            if (auto * obj = inputJSON.getDynamicObject())
                if(auto * data = obj->getProperty("data").getDynamicObject())
                    if(auto * repo = data->getProperty("repository").getDynamicObject())
                        for(auto & file : repo->getProperties())
                            parsedObject->setProperty(fileIDs[file.name], getHeadphoneSetting(file.value["text"].toString()));
        
        return juce::var(parsedObject);
    }
     
    void run()
    {
        juce::URL rootURL("https://raw.githubusercontent.com/jaakkopasanen/AutoEq/master/results/oratory1990/");
                        
        auto result = getResultText(rootURL.getChildURL("README.md"));
        
        //TODO: Some error handling
        if(result.startsWith("Failed"))
            return;
        
        result = result.fromLastOccurrenceOf("----------------------------------------------------", false, false);
        auto resultList = juce::StringArray::fromLines(result);
                
        if(resultList[0].startsWith("Failed"))
           return;
            
        auto postData = juce::String::formatted(R"({"query": "query {repository(owner: \"jaakkopasanen\", name: \"AutoEq\") {)");
        
        juce::StringPairArray fileIDs;
                                                                    
        for (int i = 0; i < resultList.size();++i)
        {
            // must check this as often as possible, because this is
            // how we know if the user's pressed 'cancel'
            if (threadShouldExit())
                break;
            
            auto & line = resultList[i];
                                
            if(!line.startsWith("- "))
                continue;
            
            auto lineTokens = juce::StringArray::fromTokens(line, "[]/", "");
            
            auto headphoneName = lineTokens[1];
            auto headphoneLink = lineTokens[3];
            
            juce::String fileID("h");
            fileID += juce::String(i);
                                                        
            if(headphoneName.isEmpty())
                continue;
            
            fileIDs.set(fileID, headphoneName);
                                                                      
            postData += juce::String::formatted(R"(%s: object(expression: \"master:results/oratory1990/%s/%s/%s ParametricEQ.txt\") { ... on Blob { text } } )",fileID.toRawUTF8(), headphoneLink.toRawUTF8(), headphoneName.toRawUTF8(), headphoneName.toRawUTF8());
                                                                                                                    
            // this will update the progress bar on the dialog box
            setProgress (i / (double) resultList.size());
  
            //   ... do the business here...
        }
        
        postData += R"(}}"})";
                            
        juce::URL headphoneSettingURL("https://api.github.com/graphql"), headphone;
        headphoneSettingURL = headphoneSettingURL.withPOSTData(postData);
                
        auto headphoneSettings = getHeadphoneSettings(headphoneSettingURL, fileIDs);
        
        auto file = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
        file = file.getChildFile("Hequaliser/headphoneNames.txt");
        file.create();
        file.replaceWithText("");
        juce::FileOutputStream fileOutputStream(file);
        
        juce::JSON::writeToStream(fileOutputStream, headphoneSettings);
    }
    
    void threadComplete (bool userPressedCancel) override
        {
            const juce::String messageString (userPressedCancel ? "You pressed cancel!" : "Thread finished ok!");

            juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::InfoIcon)
                                      .withTitle ("Progress window")
                                      .withMessage (messageString)
                                      .withButton ("OK"),
                                    nullptr);

            // ..and clean up by deleting our thread object..
            delete this;
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> headphoneTypeAttachment;
    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;

    juce::PopupMenu               contextMenu;
};
