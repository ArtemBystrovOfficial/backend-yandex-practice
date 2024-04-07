#pragma once
#include <string>
#include <vector>
#include <optional>

namespace domain {

class RetiredPlayer {
public:
    RetiredPlayer(const std::string & name, int score, int play_time_ms, std::optional<std::string> uuid = std::nullopt)
        : name_(name)
        , score_(score)
        , play_time_ms_(play_time_ms)
        , uuid_(uuid ? *uuid : GenerateUuid()) {

    }

    const std::string & GetId() const noexcept {
        return uuid_;
    }

    const std::string & GetName() const noexcept {
        return name_;
    }

    int GetScore() const noexcept {
        return score_;
    }

    int GetPlayTimeMs() const noexcept {
        return play_time_ms_;
    }

private:
    std::string GenerateUuid();

    std::string uuid_;
    std::string name_;
    int score_;
    int play_time_ms_;
};

class RetiredPlayerRepository {
public: 
    using retired_players_t = std::vector<RetiredPlayer>; 

    virtual retired_players_t GetSortedRetiredPlayersList(int offset, int limit) = 0;
    virtual void AddRetriedPlayer(const RetiredPlayer &) = 0;

protected:
    ~RetiredPlayerRepository() = default;
};

}  // namespace domain