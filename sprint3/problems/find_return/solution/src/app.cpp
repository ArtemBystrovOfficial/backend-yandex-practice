#include "app.h"

namespace app {

App::App(const std::filesystem::path& settings_json) : game_(settings_json), players_(game_), tick_edit_access_(false) {};

}