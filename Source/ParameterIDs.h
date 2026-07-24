#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace desire::param {
inline const juce::ParameterID input { "input", 1 };
inline const juce::ParameterID body { "body", 1 };
inline const juce::ParameterID heat { "heat", 1 };
inline const juce::ParameterID silk { "silk", 1 };
inline const juce::ParameterID desire { "desire", 1 };
inline const juce::ParameterID motion { "motion", 1 };
inline const juce::ParameterID width { "width", 1 };
inline const juce::ParameterID mix { "mix", 1 };
inline const juce::ParameterID output { "output", 1 };
inline const juce::ParameterID mode { "mode", 1 };
inline const juce::ParameterID bypass { "bypass", 1 };
} // namespace desire::param
