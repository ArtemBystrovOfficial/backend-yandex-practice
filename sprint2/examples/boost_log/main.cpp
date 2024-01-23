#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>

using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_file_log(
        keywords::file_name = "sample.log",
        keywords::format = "[%ProcessID%] [%TimeStamp%] [%ThreadID%]: %Message%",
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
    );
}

int main() {
    InitBoostLogFilter();
    BOOST_LOG_TRIVIAL(info) << "test";
}