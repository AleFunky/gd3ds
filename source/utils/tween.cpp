#include "Tween.hpp"
#include "../macros.hpp"

namespace gd3ds
{

float bounceTime(float time) {
    if (time < 1 / 2.75) {
        return 7.5625f * time * time;
    }

    else if (time < 2 / 2.75) {
        time -= 1.5f / 2.75f;
        return 7.5625f * time * time + 0.75f;
    }

    else if (time < 2.5 / 2.75) {
        time -= 2.25f / 2.75f;
        return 7.5625f * time * time + 0.9375f;
    }

    time -= 2.625f / 2.75f;
    return 7.5625f * time * time + 0.984375f;
}

/*
    Easing functions ported from cocos2d-x source.
*/
std::function<float(float)> getEasingFunction(const EasingType& easing, float rate = 2.0f, float period = 0.3f) {
    switch (easing) {
        case EasingType::Linear:
            return [](float t) { return t; };
        case EasingType::EaseIn:
            return [rate](float t) { return powf(t, rate); };
        case EasingType::EaseOut:
            return [rate](float t) { return powf(t, 1 / rate); };
        case EasingType::EaseInOut:
            return [rate](float t) {
                t *= 2;
                if (t < 1)
                    return 0.5f * powf(t, rate);
                else
                    return 1.0f - 0.5f * powf(2-t, rate);
            };
        case EasingType::ExponentialIn:
            return [](float t) {
                return t == 0 ? 0 : powf(2, 10 * (t/1 - 1)) - 1 * 0.001f;
            };
        case EasingType::ExponentialOut:
            return [](float t) {
                return t == 1 ? 1 : (-powf(2, -10 * t / 1) + 1);
            };
        case EasingType::ExponentialInOut:
            return [](float t) {
                t /= 0.5f;
                if (t < 1)
                    t = 0.5f * powf(2, 10 * (t - 1));
                else
                    t = 0.5f * (-powf(2, -10 * (t - 1)) + 2);

                return t;
            };
        case EasingType::SineIn:
            return [](float t) {
                return -1 * cosf(t * (float)CC_PI_2) + 1;
            };
        case EasingType::SineOut:
            return [](float t) {
                return sinf(t * (float)CC_PI_2);
            };
        case EasingType::SineInOut:
            return [](float t) {
                return -0.5f * (cosf((float)CC_PI_2 * t) - 1);
            };
        case EasingType::ElasticIn:
            return [period](float t) {
                float newT = 0;
                if (t == 0 || t == 1)
                    newT = t;
                else
                {
                    float s = period / 4;
                    t -= 1;
                    newT = -powf(2, 10 * t) * sinf((t - s) * CC_PI_X_2 / period);
                }

                return newT;
            };
        case EasingType::ElasticOut:
            return [period](float t) {
                float newT = 0;

                if (t == 0 || t == 1)
                    newT = t;
                else
                {
                    float s = period / 4;
                    newT = powf(2, -10 * t) * sinf((t - s) * CC_PI_X_2 / period) + 1;
                }

                return newT;
            };
        case EasingType::ElasticInOut:
            return [period](float t) {
                float newT = 0;

                if (t == 0 || t == 1)
                    newT = t;
                else
                {
                    t *= 2;
                    float p = period == 0.0f ? 0.3f * 1.5f : period;
                    
                    float s = p / 4;

                    t -= 1;
                    if (t < 0)
                        newT = -0.5f * powf(2, 10 * t) * sinf((t -s) * CC_PI_X_2 / p);
                    else
                        newT = powf(2, -10 * t) * sinf((t - s) * CC_PI_X_2 / p) * 0.5f + 1;
                }

                return newT;
            };
        case EasingType::BounceIn:
            return [](float t) {
                return 1 - bounceTime(1 - t);
            };
        case EasingType::BounceOut:
            return [](float t) {
                return bounceTime(t);
            };
        case EasingType::BounceInOut:
            return [](float t) {
                if (t < 0.5f)
                    return (1 - bounceTime(1 - t * 2)) * 0.5f;
                else
                    return bounceTime(t * 2 - 1) * 0.5f + 0.5f;
            };
        case EasingType::BackIn:
            return [](float t) {
                float overshoot = 1.70158f;
                return t * t * ((overshoot + 1) * t - overshoot);
            };
        case EasingType::BackOut:
            return [](float t) {
                float overshoot = 1.70158f;
                t -= 1;
                return t * t * ((overshoot + 1) * t + overshoot) + 1;
            };
        case EasingType::BackInOut:
            return [](float t) {
                float overshoot = 1.70158f * 1.525f;

                t *= 2;
                if (t < 1)
                    return (t * t * ((overshoot + 1) * t - overshoot)) / 2;
                else
                {
                    t -= 2;
                    return (t * t * ((overshoot + 1) * t + overshoot)) / 2 + 1;
                }
            };
        default:
            return nullptr;
    }
}

// Tween& Tween::create(float initialValue) {
//     std::unique_ptr<Tween> t = std::make_unique<Tween>();
//     t->initialValue_ = initialValue;
//     t->from_ = initialValue;
//     t->value_ = initialValue;
//     // return Director::get()->queueTween(std::move(t)); // TODO
// }

Tween& Tween::delay(float seconds) {
    addKeyframe({
        .duration = seconds,
        .to = value_,
        .easeFunction = nullptr,
        .options = {}
    });
    return *this;
}

Tween& Tween::to(float duration, float to, const TweenOptions &options) {
    addKeyframe({
        .duration = duration,
        .to = to,
        .easeFunction = getEasingFunction(options.easing),
        .options = options
    });
    return *this;
}

Tween& Tween::call(Callback<> call) {
    addKeyframe({
        .duration = 0.0f,
        .to = value_,
        .easeFunction = nullptr,
        .options = {
            .onComplete = call
        }
    });
    return *this;
}

Tween& Tween::retain() {
    deleteSelf_ = false;
    return *this;
}

Tween &Tween::repeatForever() {
    repeatForever_ = true;
    return *this;
}

Tween &Tween::stopRepeating() {
    repeatForever_ = true;
    return *this;
}

void Tween::start() {
    if (running_) return;
    restart();
    running_ = true;

    if (!hasEverStarted_) {
        hasEverStarted_ = true;
        // Director::get()->startTween(this); // TODO
    }
}

void Tween::stop() {
    running_ = false;
    restart();
}

void Tween::restart() {
    keyframeIndex_ = 0;
    value_ = initialValue_;
    from_ = initialValue_;
    reversed_ = false;
    elapsed_ = 0.0f;
}

void Tween::addKeyframe(const TweenKeyframe& keyframe) {
    keyframes_.push_back(keyframe);
}

void Tween::pause() {
    paused_ = true;
}

void Tween::resume() {
    paused_ = false;
}

void Tween::step(float dt) {
    if (!running_ || paused_ || keyframes_.size() <= 0) return;

    elapsed_ += dt;
    auto& kf = keyframes_[keyframeIndex_];

    if (kf.duration <= 0.0f) { // Prevent division by zero
        value_ = kf.to;
    }
    else {
        auto& easeFn = kf.easeFunction;
        float t = std::clamp(elapsed_ / kf.duration, 0.0f, 1.0f);

        if (reversed_) t = 1.0f - t;

        float easedT = easeFn ? easeFn(t) : t;

        value_ = std::lerp(from_, kf.to, easedT);        
    }

    if (auto& onUpdate = kf.options.onUpdate)
        onUpdate(value_);

    if (kf.duration <= 0.0f) {
        keyframeCompleted(kf);
        return;
    }

    if (elapsed_ >= kf.duration) {
        if (kf.options.yoyo && !reversed_) {
            reverseFromYoyo(kf);
            return;
        }

        else keyframeCompleted(kf);
    }
    
}

void Tween::keyframeCompleted(const TweenKeyframe& kf) {

    if (auto& onComplete = kf.options.onComplete)
        onComplete();

    from_ = kf.to;
    elapsed_ -= kf.duration;
    keyframeIndex_++;
    reversed_ = false;

    if (keyframeIndex_ >= static_cast<int>(keyframes_.size()))
        tweenCompleted();
}

void Tween::tweenCompleted() {
    if (repeatForever_) {

        // Resetting elapsed time during loops causes drift
        float elapsed = elapsed_;
        restart();
        elapsed_ = elapsed;

        return;
    }

    running_ = false;

    // if (deleteSelf_)
        // Director::get()->removeTween(this); // TODO
}

void Tween::reverseFromYoyo(const TweenKeyframe& kf) {
    reversed_ = true;
    elapsed_ -= kf.duration;
}

}