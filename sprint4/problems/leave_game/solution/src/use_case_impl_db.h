#pragma once
#include "use_cases.h"

#include "postgres.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    UseCasesImpl() = delete;
    explicit UseCasesImpl(postgres::Database & database) : database_(database) {}

    void AddPlayerRetired(const RetiredPlayerInfo & player) override;
    players_list_t GetPlayersRetired(int offset, int limit) override;

private:
    postgres::Database & database_;
};

}  // namespace app