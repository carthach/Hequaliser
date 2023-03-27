#pragma once

using namespace juce;

class DarkLookAndFeel : public LookAndFeel_V4
{
public:
    DarkLookAndFeel()
    {
        setColour(Slider::thumbColourId, Colours::white);
        setColour(Slider::trackColourId, Colours::darkgrey);
        setColour(Slider::rotarySliderFillColourId, Colours::darkgrey);
        setColour(Slider::rotarySliderOutlineColourId, Colours::white);
        setColour(TextButton::buttonColourId, Colours::darkgrey);
        setColour(TextButton::textColourOnId, Colours::white);
        setColour(TextButton::textColourOffId, Colours::white);
        setColour(ComboBox::backgroundColourId, Colours::darkgrey);
        setColour(ComboBox::textColourId, Colours::white);
        setColour(ComboBox::outlineColourId, Colours::white);
        setColour(ComboBox::arrowColourId, Colours::white);
    }
    
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto cornerSize = 4.0f;
        auto outlineColour = button.findColour(button.getToggleState() ? TextButton::textColourOnId : TextButton::textColourOffId);
        auto background = backgroundColour.brighter(0.1f);
        
        if (isButtonDown || isMouseOverButton)
        {
            background = background.darker(0.1f);
        }
        
        g.setColour(background);
        g.fillRoundedRectangle(bounds, cornerSize);
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
    
    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) override
    {
        auto cornerSize = 4.0f;
        auto bounds = Rectangle<int>{ width, height }.toFloat().reduced(0.5f);
        auto outlineColour = findColour(ComboBox::outlineColourId);
        auto textColour = findColour(ComboBox::textColourId);
        auto arrowColour = findColour(ComboBox::arrowColourId);
        auto background = findColour(ComboBox::backgroundColourId);
        
        if (box.isEnabled())
        {
            g.setColour(background);
            g.fillRoundedRectangle(bounds, cornerSize);
            
            g.setColour(outlineColour);
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            
            auto arrowX = buttonX + buttonW * 0.5f;
            auto arrowY = buttonY + buttonH * 0.5f;
            
            Path path;
            path.addTriangle(-4.0f, -2.0f, 4.0f, -2.0f, 0.0f, 3.0f);
            AffineTransform transform;
            transform = transform.translated(arrowX, arrowY);
            g.setColour(arrowColour);
            g.fillPath(path, transform);
        }
        else
        {
            g.setColour(background.brighter(0.5f));
            g.fillRoundedRectangle(bounds, cornerSize);
            
            g.setColour(textColour.withAlpha(0.5f));
            g.drawFittedText("Not Available", bounds.toNearestInt(), Justification::centred, 1);
        }
    }
};
   

