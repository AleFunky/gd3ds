#pragma once

#include "../types.hpp"
#include <vector>

/////////////////////////
// Tween ported from Open Dash by StarryDawn72
/////////////////////////

// TODO: ACTUALLY MAKE THE TWEEN WORK (BY STARRY!!!)

namespace gd3ds
{

enum class EasingType {
    Linear,

    EaseIn,
    EaseOut,
    EaseInOut,

    ExponentialIn,
    ExponentialOut,
    ExponentialInOut,

    SineIn,
    SineOut,
    SineInOut,

    ElasticIn,
    ElasticOut,
    ElasticInOut,

    BounceIn,
    BounceOut,
    BounceInOut,

    BackIn,
    BackOut,
    BackInOut,
};

struct TweenOptions {
    EasingType easing = EasingType::Linear;
    Callback<float> onUpdate = nullptr;
    Callback<> onComplete = nullptr;
    bool yoyo = false;
};

struct TweenKeyframe {
    float duration;
    float to;
    std::function<float(float)> easeFunction;
    TweenOptions options;
};

class Tween {
    // friend class Director;
public:
    // static Tween& create(float initialValue);

    Tween& delay(float seconds);

    /*
        Note: onComplete on a keyframe runs the SAME frame that keyframe is done, while
        a `.call()` chained will wait for a new frame to be called. Do not expect
        onComplete and .call() to behave the same for single-keyframed tweens, they
        execute a frame apart.
    */
    Tween& to(float duration, float to, const TweenOptions& options);

    /*
        Chain a callback.
    */
    Tween& call(Callback<> call);

    /*
        Stops a tween from deleting itself upon completion.
    */
    Tween& retain();

    /*
        Sets the repeat count to infinity. The tween will loop until stopped by `stopRepeating()`.
        This overrides `setRepeatCount()` if called after it.
        This is a no-op when duration is 0.
    */
    Tween& repeatForever();

    /*
        Cancels looping entirely.
    */
    Tween& stopRepeating();

    void start();
    void pause();
    void resume();

    /*
        Stops the tween from running and resets progress.
    */
	void stop();

    /*
        Resets the tween's progress to the beginning.
    */
    void restart();
    
private:
    void addKeyframe(const TweenKeyframe& keyframe);
    void step(float dt);
    void keyframeCompleted(const TweenKeyframe& kf);
    void tweenCompleted();
    void reverseFromYoyo(const TweenKeyframe& kf);
private:
    std::vector<TweenKeyframe> keyframes_{};
    int keyframeIndex_ = 0;

    float value_ = 0.0f;
    float initialValue_ = 0.0f;
    float from_ = 0.0f;
    float elapsed_ = 0.0f;

    bool paused_ = false;
    bool running_ = false;
    bool deleteSelf_ = true;
    bool reversed_ = false;
    bool repeatForever_ = false;
    bool hasEverStarted_ = false;
};

}