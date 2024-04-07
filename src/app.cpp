#include "app.h"

namespace app {

App::App(const std::filesystem::path& settings_json, const std::string & db_url) 
: game_(settings_json)
, players_(game_)
, tick_edit_access_(false)
, database_(db_url) {
    game_.request_to_save_retired_player_s.connect(
        [this](const std::string & a1, int a2, int a3){use_case_db.AddPlayerRetired({a1,a2,a3});}
    );
};

}