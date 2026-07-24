#include "SpectrumAnalyser.h"

#include <cmath>

namespace desire {

SpectrumAnalyser::SpectrumAnalyser()
    : fft_(fftOrder),
      window_(static_cast<size_t>(fftSize), juce::dsp::WindowingFunction<float>::hann) {}

void SpectrumAnalyser::pushSample(float sample) noexcept {
    if (fifoIndex_ == fftSize) {
        if (!nextBlockReady_.load(std::memory_order_acquire)) {
            std::fill(fftData_.begin(), fftData_.end(), 0.0f);
            std::copy(fifoBuffer_.begin(), fifoBuffer_.end(), fftData_.begin());
            nextBlockReady_.store(true, std::memory_order_release);
        }
        fifoIndex_ = 0;
    }
    fifoBuffer_[static_cast<size_t>(fifoIndex_++)] = sample;
}

void SpectrumAnalyser::computeBins(std::array<float, numOutputBins>& outBins,
                                    std::array<float, 3>& outBandEnergy) noexcept {
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize);
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());

    const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
    const float minHz = 20.0f;
    const float maxHz = juce::jmin(20000.0f, nyquist);
    const float logRange = std::log(maxHz / minHz);
    const int usableBins = fftSize / 2;

    for (int band = 0; band < numOutputBins; ++band) {
        const float t0 = static_cast<float>(band) / static_cast<float>(numOutputBins);
        const float t1 = static_cast<float>(band + 1) / static_cast<float>(numOutputBins);
        const float hz0 = minHz * std::exp(logRange * t0);
        const float hz1 = minHz * std::exp(logRange * t1);

        int bin0 = juce::jlimit(0, usableBins - 1,
            static_cast<int>(hz0 / nyquist * static_cast<float>(usableBins)));
        int bin1 = juce::jlimit(bin0 + 1, usableBins,
            static_cast<int>(hz1 / nyquist * static_cast<float>(usableBins)));

        float peak = 0.0f;
        for (int bin = bin0; bin < bin1; ++bin)
            peak = juce::jmax(peak, fftData_[static_cast<size_t>(bin)]);

        const float normalised = juce::jlimit(0.0f, 1.0f,
            (juce::Decibels::gainToDecibels(peak, -72.0f) + 72.0f) / 72.0f);
        outBins[static_cast<size_t>(band)] = normalised;
    }

    const auto bandAverage = [&](float loHz, float hiHz) {
        int bin0 = juce::jlimit(0, usableBins - 1, static_cast<int>(loHz / nyquist * usableBins));
        int bin1 = juce::jlimit(bin0 + 1, usableBins, static_cast<int>(hiHz / nyquist * usableBins));
        float sum = 0.0f;
        for (int bin = bin0; bin < bin1; ++bin)
            sum += fftData_[static_cast<size_t>(bin)];
        const float mean = sum / static_cast<float>(bin1 - bin0);
        return juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(mean, -72.0f) + 72.0f) / 72.0f);
    };

    outBandEnergy[0] = bandAverage(minHz, 250.0f);
    outBandEnergy[1] = bandAverage(250.0f, 2000.0f);
    outBandEnergy[2] = bandAverage(2000.0f, maxHz);

    nextBlockReady_.store(false, std::memory_order_release);
}

} // namespace desire
