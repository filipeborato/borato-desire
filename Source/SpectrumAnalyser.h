#pragma once

#include <array>
#include <atomic>

#include <juce_dsp/juce_dsp.h>

namespace desire {

// Mono spectrum analyser fed sample-by-sample from the audio thread. The FFT
// itself, and all allocation, happens on the message thread when a block is
// ready -- the audio thread only ever writes into a pre-allocated ring.
class SpectrumAnalyser {
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numOutputBins = 96;

    SpectrumAnalyser();

    void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate; }

    // Audio thread. Feeds one (already mono-summed) sample into the FFT ring.
    void pushSample(float sample) noexcept;

    bool isNextBlockReady() const noexcept { return nextBlockReady_.load(std::memory_order_acquire); }

    // Message thread only, call after isNextBlockReady(). Fills outBins with
    // normalised (0..1) magnitudes across log-spaced bands from 20 Hz to 20 kHz,
    // and low/mid/high band energy (also 0..1).
    void computeBins(std::array<float, numOutputBins>& outBins,
                      std::array<float, 3>& outBandEnergy) noexcept;

private:
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;

    std::array<float, static_cast<size_t>(fftSize)> fifoBuffer_ {};
    std::array<float, static_cast<size_t>(fftSize) * 2> fftData_ {};
    int fifoIndex_ = 0;
    std::atomic<bool> nextBlockReady_ { false };
    double sampleRate_ = 44100.0;
};

} // namespace desire
