#include "time.h"

void model::TimeManager::GlobalTick(const std::chrono::milliseconds& ms) {
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
        if (auto timeObjPtr = it->second.lock()) {
            timeObjPtr->Tick(ms);
            ++it;
        } else 
            it = subscribers_.erase(it);
    }
}

void model::TimeManager::AddSubscribers(std::shared_ptr<TimeObject> object, int priority) {
    subscribers_.push_back({priority, object});
    std::sort(subscribers_.rbegin(),subscribers_.rend(),[](auto & p1, auto & p2){
        return p1.first < p2.first;
    });
}
