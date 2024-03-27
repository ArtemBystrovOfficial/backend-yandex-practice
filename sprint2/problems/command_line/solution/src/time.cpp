#include "time.h"

void model::TimeManager::GlobalTick(const std::chrono::milliseconds& ms) {
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
        if (auto timeObjPtr = it->lock()) {
            timeObjPtr->Tick(ms);
            ++it;
        } else 
            it = subscribers_.erase(it);
    }
}

void model::TimeManager::AddSubscribers(std::shared_ptr<TimeObject> object) {
    subscribers_.emplace_back(object);
}
