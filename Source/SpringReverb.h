#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

// Spring tank model: two parallel springs, each a feedback delay line with a
// chain of allpass filters inside the loop. The allpass chain delays low
// frequencies more than highs, which gives the "boing"/chirp of a real spring
// every time the signal goes round the loop.
class SpringReverb
{
public:
    void prepare (double sampleRate, float delayMs1, float delayMs2)
    {
        const float delaysMs[numSprings] = { delayMs1, delayMs2 };

        for (int s = 0; s < numSprings; ++s)
        {
            auto& sp = springs[(size_t) s];
            sp.length = std::max (16, (int) std::lround (delaysMs[s] * 0.001 * sampleRate));
            sp.buffer.assign ((size_t) sp.length, 0.0f);
            sp.pos = 0;
            sp.lp = 0.0f;
            sp.ap.fill (0.0f);
        }

        const double twoPi = 6.283185307179586;
        loopLpCoeff = (float) (1.0 - std::exp (-twoPi * 3000.0 / sampleRate));
        inLpCoeff   = (float) (1.0 - std::exp (-twoPi * 4500.0 / sampleRate));
        inLp = 0.0f;
    }

    void setFeedback (float fb) { feedback = std::min (fb, 0.9f); }

    float process (float in)
    {
        inLp += inLpCoeff * (in - inLp);
        const float drive = std::tanh (inLp * 1.5f) * 0.5f;

        float out = 0.0f;
        for (auto& sp : springs)
        {
            const float delayed = sp.buffer[(size_t) sp.pos];
            sp.lp += loopLpCoeff * (delayed - sp.lp);

            float sig = drive + feedback * sp.lp;

            for (auto& z : sp.ap)
            {
                const float y = apCoeff * sig + z;
                z = sig - apCoeff * y;
                sig = y;
            }

            sp.buffer[(size_t) sp.pos] = sig;
            if (++sp.pos >= sp.length)
                sp.pos = 0;

            out += delayed;
        }
        return out * 0.5f;
    }

private:
    static constexpr int numSprings = 2;
    static constexpr int numAllpass = 30;
    static constexpr float apCoeff = -0.65f; // negative: low freqs are delayed most

    struct Spring
    {
        std::vector<float> buffer;
        int length = 16, pos = 0;
        float lp = 0.0f;
        std::array<float, numAllpass> ap {};
    };

    std::array<Spring, numSprings> springs;
    float feedback = 0.7f, loopLpCoeff = 0.2f, inLpCoeff = 0.3f, inLp = 0.0f;
};
