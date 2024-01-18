#include "my_logger.h"

void Logger::SetTimestamp(std::chrono::system_clock::time_point ts) { CheckAndReopen(std::move(ts)); }