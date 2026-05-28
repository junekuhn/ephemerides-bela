#include <Bela.h>
#include <cmath>
#include <vector>

class Limiter {
public:

	float iThreshold;
	float sampleRate;
	
    void setup(float sampleRate, float threshold, float lookahead, float release) {
        this->sampleRate = sampleRate;

        setLookaheadMs(lookahead);   // ~2 ms
        setReleaseMs(release);    // smooth release
        iThreshold = threshold;
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
        if (envelope > iThreshold) {
			//proportially scaling down the gain to smooth the input signal
            gain = iThreshold / envelope;
        }

        return delayed * gain;
    }

private:

    float envelope = 0.0f;
    float releaseCoeff = 0.999f;

    std::vector<float> delayBuffer;
    int writeIndex = 0;
};
