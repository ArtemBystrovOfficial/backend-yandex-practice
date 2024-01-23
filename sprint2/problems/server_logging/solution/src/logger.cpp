#include "logger.h"

#include <boost/date_time.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "json_loader.h"

namespace {
namespace sinks = boost::log::sinks;

const std::string EMPTY = "";
const std::string TIMESTAMP = "timestamp";
const std::string DATA = "data";
const std::string MESSAGE = "message";

void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    boost::property_tree::ptree tree;

    if (auto ts = rec[timestamp]; ts)
        tree.put(TIMESTAMP, to_iso_extended_string(*ts));
    else
        tree.put(TIMESTAMP, EMPTY);

    if (auto add_data = rec[additional_data]; add_data)
        tree.add_child(DATA, *add_data);
    else
        tree.add_child(DATA, {});

    if (auto message = rec[logging::expressions::smessage]; message)
        tree.put(MESSAGE, *message);
    else
        tree.put(MESSAGE, EMPTY);

    strm << json_loader::JsonObject::GetJson(tree, false);
}

}  // namespace

void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_console_log(std::clog, keywords::format = &LogFormatter, keywords::auto_flush = true);
}
