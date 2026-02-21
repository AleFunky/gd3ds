#pragma once

typedef enum {
    EASE_LINEAR,

    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,

    SINE_IN,
    SINE_OUT,
    SINE_IN_OUT,

    QUAD_IN,
    QUAD_OUT,
    QUAD_INT_OUT,

    CUBIC_IN,
    CUBIC_OUT,
    CUBIC_IN_OUT,

    QUART_IN,
    QUART_OUT,
    QUART_IN_OUT,
    
    QUINT_IN,
    QUINT_OUT,
    QUINT_IN_OUT,

    EXPO_IN,
    EXPO_OUT,
    EXPO_IN_OUT,
    
    CIRC_IN,
    CIRC_OUT,
    CIRC_IN_OUT,
    
    ELASTIC_IN,
    ELASTIC_OUT,
    ELASTIC_IN_OUT,
    
    BACK_IN,
    BACK_OUT,
    BACK_IN_OUT,
    
    BOUNCE_IN,
    BOUNCE_OUT,
    BOUNCE_IN_OUT,
    
    QUADRATIC_IN,
    QUADRATIC_OUT,
    QUADRATIC_IN_OUT,

    EASING_COUNT,
} EaseTypes;

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