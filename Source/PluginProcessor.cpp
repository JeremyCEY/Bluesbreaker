/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BluesbreakerAudioProcessor::BluesbreakerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

BluesbreakerAudioProcessor::~BluesbreakerAudioProcessor()
{
}

//==============================================================================
const juce::String BluesbreakerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BluesbreakerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BluesbreakerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BluesbreakerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BluesbreakerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BluesbreakerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BluesbreakerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BluesbreakerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BluesbreakerAudioProcessor::getProgramName (int index)
{
    return {};
}

void BluesbreakerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BluesbreakerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    
    spec.numChannels = 2;
    
    spec.sampleRate = sampleRate;
    
    chain.prepare(spec);
    
//    *chain.get<hpf30>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(sampleRate, 30.0f);
//    *chain.get<lpf6_3k>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass  (sampleRate, 6.3e3f);

    updateParameters();

}

void BluesbreakerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BluesbreakerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BluesbreakerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    if (apvts.getRawParameterValue("Bypass")->load() == 1.0f)
    {
        return;
    }

    updateParameters();
    
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto monoBlock = block.getSingleChannelBlock(channel);
        juce::dsp::ProcessContextReplacing<float> context(monoBlock);
        chain.process(context);
    }
}

//==============================================================================
bool BluesbreakerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BluesbreakerAudioProcessor::createEditor()
{
//    return new BluesbreakerAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BluesbreakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void BluesbreakerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if( tree.isValid() )
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.Gain = apvts.getRawParameterValue("Gain")->load();
    settings.Tone = apvts.getRawParameterValue("Tone")->load();
    settings.Volume = apvts.getRawParameterValue("Volume")->load();
    
    return settings;
}


void BluesbreakerAudioProcessor::updateFilters()
{
    
    
}

void BluesbreakerAudioProcessor::updateParameters()
{
    auto chainSettings = getChainSettings(apvts);

    //first gain stage non inverting
    float R1 = 3300.0f;
    float R2 = 4700.0f;
    float normalizedGainKnob = chainSettings.Gain / 10.0f; // Normalize to [0, 1]
    float minGain = 0.0f;
    float maxGain = 100000.0f;

    float Rgain = minGain + normalizedGainKnob * (maxGain - minGain);
    
    float gainValue = 1 + (R1 + Rgain) / R2;
    
    chain.get<firstgainstage>().setGainLinear(gainValue);
    
    
    float capacitance1 = 47e-12f; // 47 pF in farads
            float capacitance2 = 10e-9f;  // 10 nF in farads
            float capacitance3 = 10e-9f;  // 10 nF in farads
            float resistance1 = 3.3e3f;   // 3.3k ohms
            float resistance2 = 4.7e3f;   // 4.7k ohms

            // Calculate the total capacitance in parallel
            float totalCapacitance = capacitance1 + (capacitance2 * capacitance3) / (capacitance2 + capacitance3);

            // Calculate the equivalent resistance
            float equivalentResistance = resistance1 + resistance2;

            // Calculate the cutoff frequency
            float cutoff = 1.0f / (2.0f * juce::MathConstants<float>::pi * equivalentResistance * totalCapacitance);
    
    //Post First Stage Low Pass
    *chain.get<postfirststagelowpass>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(getSampleRate(), cutoff);

    
    
    //Second Gain Stage Inverting
    chain.get<secondgainstage>().setGainLinear(-220000.0 / (220000.0 + 6800.0));

    
    //Soft Clipping with diodes
    chain.get<softClip>().functionToUse = [](float x) {
        // Adjust the gain factor for the desired clipping threshold
        float gainFactor = 0.1f; // Decrease this value for less gain

        // Simulate diode clipping behavior with two diodes facing one direction
        float clippedValuePositive = x - gainFactor * std::atan(x);

        // Simulate diode clipping behavior with two diodes facing the other direction
        float clippedValueNegative = -gainFactor * std::atan(-x);

        // Combine the positive and negative clipping
        float clippedValue = (x >= 0) ? clippedValuePositive : clippedValueNegative;

        return clippedValue;
    };
    
    
    
    //Tone control
    float normalizedToneKnob = chainSettings.Tone / 10.0f; // Normalize to [0, 1]
    float minCutoff = 612.0f; // Set your desired minimum cutoff frequency
    float maxCutoff = 15915.0f; // Set your desired maximum cutoff frequency

    float cutoffFrequency = minCutoff + normalizedToneKnob * (maxCutoff - minCutoff);

    *chain.get<tone>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(getSampleRate(), cutoffFrequency);

    
    //Post Tone Control Low Pass
    *chain.get<postToneLowPass>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(getSampleRate(), 2340.0f);

    
    //Volume Control

    float volumeKnob = chainSettings.Volume;

    float min = 0.0f;
    float max = 1.0f;
    
    float Volume = min + volumeKnob * (max - min);
    
    chain.get<volume>().setGainLinear(Volume);
}

juce::AudioProcessorValueTreeState::ParameterLayout BluesbreakerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 5.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Tone", "Tone", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 5.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Volume", "Volume", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 5.f));
    
    layout.add(std::make_unique<juce::AudioParameterBool>("Bypass", "Bypass", false));

    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BluesbreakerAudioProcessor();
}
