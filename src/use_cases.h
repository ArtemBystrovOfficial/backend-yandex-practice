#pragma once

#include <vector>
#include <string>
#include <optional>

namespace app {

struct RetiredPlayerInfo {
    std::string name_;
    int score_;
    int play_time_ms_;
};

class UseCases {
public:
    using players_list_t = std::vector<RetiredPlayerInfo>;

    virtual void AddPlayerRetired(const RetiredPlayerInfo & player) = 0;
    virtual players_list_t GetPlayersRetired(int offset, int limit) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app