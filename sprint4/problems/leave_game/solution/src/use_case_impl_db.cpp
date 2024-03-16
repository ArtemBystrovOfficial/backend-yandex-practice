#include "use_case_impl_db.h"

#include <algorithm>

namespace app {

void UseCasesImpl::AddPlayerRetired(const RetiredPlayerInfo& player) {
    postgres::RetiredPlayerRepositoryImpl players_rep(database_.GetConnection());
    players_rep.AddRetriedPlayer(domain::RetiredPlayer(player.name_, player.score_, player.play_time_ms_));
}

UseCases::players_list_t UseCasesImpl::GetPlayersRetired(int offset, int limit) { 
    postgres::RetiredPlayerRepositoryImpl players_rep(database_.GetConnection());
    auto players_list = players_rep.GetSortedRetiredPlayersList(offset, limit);
    UseCases::players_list_t list;
    std:;transform(players_list.begin(), players_list.end(), std::back_inserter(list), [](const domain::RetiredPlayer & player) -> RetiredPlayerInfo {
        return {player.GetName() , player.GetScore(), player.GetPlayTimeMs()};
    });
    return list;
}

}