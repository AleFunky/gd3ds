#include <math.h>
#include "easing.h"

#define M_PI 3.14159265358979323846
#define M_PI_X2 (M_PI * 2)

#ifndef M_PI_2
    #define M_PI_2 (M_PI / 2)
#endif


float executeEase(EaseTypes ease, float time, float period) {
    switch (ease) {
        case EASE_LINEAR:
            return linear(time);

        case EASE_IN:
            return easeIn(time, period);
        case EASE_OUT:
            return easeOut(time, period);
        case EASE_IN_OUT:
            return easeInOut(time, period);

        case SINE_IN:
            return sineEaseIn(time);
        case SINE_OUT:
            return sineEaseOut(time);
        case SINE_IN_OUT:
            return sineEaseInOut(time);

        case QUAD_IN:
            return quadEaseIn(time);
        case QUAD_OUT:
            return quadEaseOut(time);
        case QUAD_INT_OUT:
            return quadEaseInOut(time);

        case CUBIC_IN:
            return cubicEaseIn(time);
        case CUBIC_OUT:
            return cubicEaseOut(time);
        case CUBIC_IN_OUT:
            return cubicEaseInOut(time);

        case QUART_IN:
            return quartEaseIn(time);
        case QUART_OUT:
            return quartEaseOut(time);
        case QUART_IN_OUT:
            return quartEaseInOut(time);

        case QUINT_IN:
            return quintEaseIn(time);
        case QUINT_OUT:
            return quintEaseOut(time);
        case QUINT_IN_OUT:
            return quintEaseInOut(time);

        case EXPO_IN:
            return expoEaseIn(time);
        case EXPO_OUT:
            return expoEaseOut(time);
        case EXPO_IN_OUT:
            return expoEaseInOut(time);

        case CIRC_IN:
            return circEaseIn(time);
        case CIRC_OUT:
            return circEaseOut(time);
        case CIRC_IN_OUT:
            return circEaseInOut(time);

        case ELASTIC_IN:
            return elasticEaseIn(time, period);
        case ELASTIC_OUT:
            return elasticEaseOut(time, period);
        case ELASTIC_IN_OUT:
            return elasticEaseInOut(time, period);

        case BACK_IN:
            return backEaseIn(time);
        case BACK_OUT:
            return backEaseOut(time);
        case BACK_IN_OUT:
            return backEaseInOut(time);

        case BOUNCE_IN:
            return bounceEaseIn(time);
        case BOUNCE_OUT:
            return bounceEaseOut(time);
        case BOUNCE_IN_OUT:
            return bounceEaseInOut(time);

        case QUADRATIC_IN:
            return quadraticIn(time);
        case QUADRATIC_OUT:
            return quadraticOut(time);
        case QUADRATIC_IN_OUT:
            return quadraticInOut(time);
        
        default:
            return sineEaseInOut(time);
    }
}

float easeValue(EaseTypes ease, float start, float end, float elapsed, float duration, float period) {
    if (duration <= 0.0f) return end;

    float t = elapsed / duration;    
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float easedT = executeEase(ease, t, period);

    return start + (end - start) * easedT;
}

float easeTime(EaseTypes ease, float elapsed, float duration, float period) {
    if (duration <= 0.0f) return 1.f;

    float t = elapsed / duration;    
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float easedT = executeEase(ease, t, period);

    return easedT;
}

float easeIn(float time, float rate)
{
    return powf(time, rate);
}

float easeOut(float time, float rate)
{
    return powf(time, 1 / rate);
}

float easeInOut(float time, float rate)
{
    time *= 2;
    if (time < 1)
    {
        return 0.5f * powf(time, rate);
    }
    else
    {
        return (1.0f - 0.5f * powf(2 - time, rate));
    }
}


// Linear
float linear(float time)
{
    return time;
}

// Sine Ease
float sineEaseIn(float time)
{
    return -1 * cosf(time * (float)M_PI_2) + 1;
}

float sineEaseOut(float time)
{
    return sinf(time * (float)M_PI_2);
}

float sineEaseInOut(float time)
{
    return -0.5f * (cosf((float)M_PI * time) - 1);
}

// Quad Ease
float quadEaseIn(float time)
{
    return time * time;
}

float quadEaseOut(float time)
{
    return -1 * time * (time - 2);
}

float quadEaseInOut(float time)
{
    time = time * 2;
    if (time < 1)
        return 0.5f * time * time;
    --time;
    return -0.5f * (time * (time - 2) - 1);
}

// Cubic Ease
float cubicEaseIn(float time)
{
    return time * time * time;
}
float cubicEaseOut(float time)
{
    time -= 1;
    return (time * time * time + 1);
}
float cubicEaseInOut(float time)
{
    time = time * 2;
    if (time < 1)
        return 0.5f * time * time * time;
    time -= 2;
    return 0.5f * (time * time * time + 2);
}

// Quart Ease
float quartEaseIn(float time)
{
    return time * time * time * time;
}

float quartEaseOut(float time)
{
    time -= 1;
    return -(time * time * time * time - 1);
}

float quartEaseInOut(float time)
{
    time = time * 2;
    if (time < 1)
        return 0.5f * time * time * time * time;
    time -= 2;
    return -0.5f * (time * time * time * time - 2);
}

// Quint Ease
float quintEaseIn(float time)
{
    return time * time * time * time * time;
}

float quintEaseOut(float time)
{
    time -= 1;
    return (time * time * time * time * time + 1);
}

float quintEaseInOut(float time)
{
    time = time * 2;
    if (time < 1)
        return 0.5f * time * time * time * time * time;
    time -= 2;
    return 0.5f * (time * time * time * time * time + 2);
}

// Expo Ease
float expoEaseIn(float time)
{
    return time == 0 ? 0 : powf(2, 10 * (time / 1 - 1)) - 1 * 0.001f;
}
float expoEaseOut(float time)
{
    return time == 1 ? 1 : (-powf(2, -10 * time / 1) + 1);
}
float expoEaseInOut(float time)
{
    if (time == 0 || time == 1)
        return time;

    if (time < 0.5f)
        return 0.5f * powf(2, 10 * (time * 2 - 1));

    return 0.5f * (-powf(2, -10 * (time * 2 - 1)) + 2);
}

// Circ Ease
float circEaseIn(float time)
{
    return -1 * (sqrtf(1 - time * time) - 1);
}
float circEaseOut(float time)
{
    time = time - 1;
    return sqrtf(1 - time * time);
}
float circEaseInOut(float time)
{
    time = time * 2;
    if (time < 1)
        return -0.5f * (sqrtf(1 - time * time) - 1);
    time -= 2;
    return 0.5f * (sqrtf(1 - time * time) + 1);
}

// Elastic Ease
float elasticEaseIn(float time, float period)
{

    float newT = 0;
    if (time == 0 || time == 1)
    {
        newT = time;
    }
    else
    {
        float s = period / 4;
        time = time - 1;
        newT = -powf(2, 10 * time) * sinf((time - s) * M_PI_X2 / period);
    }

    return newT;
}
float elasticEaseOut(float time, float period)
{

    float newT = 0;
    if (time == 0 || time == 1)
    {
        newT = time;
    }
    else
    {
        float s = period / 4;
        newT = powf(2, -10 * time) * sinf((time - s) * M_PI_X2 / period) + 1;
    }

    return newT;
}
float elasticEaseInOut(float time, float period)
{

    float newT = 0;
    if (time == 0 || time == 1)
    {
        newT = time;
    }
    else
    {
        time = time * 2;
        if (!period)
        {
            period = 0.3f * 1.5f;
        }

        float s = period / 4;

        time = time - 1;
        if (time < 0)
        {
            newT = -0.5f * powf(2, 10 * time) * sinf((time - s) * M_PI_X2 / period);
        }
        else
        {
            newT = powf(2, -10 * time) * sinf((time - s) * M_PI_X2 / period) * 0.5f + 1;
        }
    }
    return newT;
}

// Back Ease
float backEaseIn(float time)
{
    float overshoot = 1.70158f;
    return time * time * ((overshoot + 1) * time - overshoot);
}
float backEaseOut(float time)
{
    float overshoot = 1.70158f;

    time = time - 1;
    return time * time * ((overshoot + 1) * time + overshoot) + 1;
}
float backEaseInOut(float time)
{
    float overshoot = 1.70158f * 1.525f;

    time = time * 2;
    if (time < 1)
    {
        return (time * time * ((overshoot + 1) * time - overshoot)) / 2;
    }
    else
    {
        time = time - 2;
        return (time * time * ((overshoot + 1) * time + overshoot)) / 2 + 1;
    }
}

// Bounce Ease
float bounceTime(float time)
{
    if (time < 1 / 2.75f)
    {
        return 7.5625f * time * time;
    }
    else if (time < 2 / 2.75f)
    {
        time -= 1.5f / 2.75f;
        return 7.5625f * time * time + 0.75f;
    }
    else if (time < 2.5f / 2.75f)
    {
        time -= 2.25f / 2.75f;
        return 7.5625f * time * time + 0.9375f;
    }

    time -= 2.625f / 2.75f;
    return 7.5625f * time * time + 0.984375f;
}
float bounceEaseIn(float time)
{
    return 1 - bounceTime(1 - time);
}

float bounceEaseOut(float time)
{
    return bounceTime(time);
}

float bounceEaseInOut(float time)
{
    float newT = 0;
    if (time < 0.5f)
    {
        time = time * 2;
        newT = (1 - bounceTime(1 - time)) * 0.5f;
    }
    else
    {
        newT = bounceTime(time * 2 - 1) * 0.5f + 0.5f;
    }

    return newT;
}

float quadraticIn(float time)
{
    return powf(time, 2);
}

float quadraticOut(float time)
{
    return -time * (time - 2);
}

float quadraticInOut(float time)
{

    float resultTime = time;
    time = time * 2;
    if (time < 1)
    {
        resultTime = time * time * 0.5f;
    }
    else
    {
        --time;
        resultTime = -0.5f * (time * (time - 2) - 1);
    }
    return resultTime;
}