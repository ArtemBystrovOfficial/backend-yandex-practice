#pragma once

#include <chrono>
#include <memory>
#include <vector>

namespace model {

// Наследумые обьекты получают сигнал обновления времени
class TimeObject {
   public:
    virtual void Tick(const std::chrono::milliseconds& ms) = 0;
};

class TimeManager {
   public:
    TimeManager() = default;

    void GlobalTick(const std::chrono::milliseconds& ms);
    void AddSubscribers(std::shared_ptr<TimeObject> object);

   private:
    std::vector<std::weak_ptr<TimeObject>> subscribers_;
};

}  // namespace model