#pragma once

namespace gd3ds
{

enum class EaseTypes {
    EaseLinear,

    EaseIn,
    EaseOut,
    EaseInOut,

    SineIn,
    SineOut,
    SineInOut,

    QuadIn,
    QuadOut,
    QuadIntOut,

    CubicIn,
    CubicOut,
    CubicInOut,

    QuartIn,
    QuartOut,
    QuartInOut,
    
    QuintIn,
    QuintOut,
    QuintInOut,

    ExpoIn,
    ExpoOut,
    ExpoInOut,
    
    CircIn,
    CircOut,
    CircInOut,
    
    ElasticIn,
    ElasticOut,
    ElasticInOut,
    
    BackIn,
    BackOut,
    BackInOut,
    
    BounceIn,
    BounceOut,
    BounceInOut,
    
    QuadraticIn,
    QuadraticOut,
    QuadraticInOut,

    EasingCount,
};

float easeValue(EaseTypes ease, float start, float end, float elapsed, float duration, float period);
float easeTime(EaseTypes ease, float elapsed, float duration, float period);

float linear(float time);

float easeIn(float time, float rate);
float easeOut(float time, float rate);
float easeInOut(float time, float rate);

float sineEaseIn(float time);
float sineEaseOut(float time);
float sineEaseInOut(float time);

float quadEaseIn(float time);
float quadEaseOut(float time);
float quadEaseInOut(float time);

float cubicEaseIn(float time);
float cubicEaseOut(float time);
float cubicEaseInOut(float time);

float quartEaseIn(float time);
float quartEaseOut(float time);
float quartEaseInOut(float time);

float quintEaseIn(float time);
float quintEaseOut(float time);
float quintEaseInOut(float time);

float expoEaseIn(float time);
float expoEaseOut(float time);
float expoEaseInOut(float time);

float circEaseIn(float time);
float circEaseOut(float time);
float circEaseInOut(float time);

float elasticEaseIn(float time, float period);
float elasticEaseOut(float time, float period);
float elasticEaseInOut(float time, float period);

float backEaseIn(float time);
float backEaseOut(float time);
float backEaseInOut(float time);

float bounceEaseIn(float time);
float bounceEaseOut(float time);
float bounceEaseInOut(float time);

float quadraticIn(float time);
float quadraticOut(float time);
float quadraticInOut(float time);

}
