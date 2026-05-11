#include <Bela.h>
#include <cmath>
#include <vector>

class Limiter {
public:
    void setup(float sampleRate) {
        this->sampleRate = sampleRate;

        setLookaheadMs(1.0f);   // ~2 ms
        setReleaseMs(50.0f);    // smooth release
        threshold = 0.95f;
    }

    void setLookaheadMs(float ms) {
        int samples = (int)(ms * 0.001f * sampleRate);
        if(samples < 1) samples = 1;

        delayBuffer.assign(samples, 0.0f);
        writeIndex = 0;
    }

    void setReleaseMs(float ms) {
		//converts ms to s, then to samples, e^(-1/samples)
        releaseCoeff = expf(-1.0f / (0.001f * ms * sampleRate));
    }

    float processSample(float x) {
        // Write input into delay buffer
        delayBuffer[writeIndex] = x;

        // Read delayed sample
        int readIndex = (writeIndex + 1) % delayBuffer.size();
        float delayed = delayBuffer[readIndex];

        writeIndex = (writeIndex + 1) % delayBuffer.size();

        // Peak detection
        float inputLevel = fabsf(x);

        // Envelope follower
		//exponential decay towards the input level, but envelope staying >= input level
        envelope = fmaxf(inputLevel, envelope * releaseCoeff);

        // Gain computation
        float gain = 1.0f;
        if (envelope > threshold) {
			//proportially scaling down the gain to smooth the input signal
            gain = threshold / envelope;
        }

        return delayed * gain;
    }

private:
    float sampleRate;
    float threshold = 0.95f;

    float envelope = 0.0f;
    float releaseCoeff = 0.999f;

    std::vector<float> delayBuffer;
    int writeIndex = 0;
};
