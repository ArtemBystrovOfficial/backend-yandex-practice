#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

struct Date {
    Date(const std::chrono::system_clock::time_point& point) {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(point);
        std::tm* timeInfo = std::gmtime(&currentTime);

        dd = timeInfo->tm_mday;
        mm = timeInfo->tm_mon + 1;
        yy = timeInfo->tm_year + 1900;
    }

    auto operator<=>(const Date&) const = default;

    std::string GetString() const {
        std::ostringstream oss;
        oss << "sample_log_" << std::setfill('0') << std::setw(4) << yy << "_" << std::setw(2) << mm << "_" << std::setw(2) << dd << ".log";
        return oss.str();
    }

    uint32_t dd, mm, yy;
};

class Logger {
    auto GetTimeStamp() const {
        if (!manual_ts_) throw std::runtime_error("set data");
        const auto t_c = std::chrono::system_clock::to_time_t(*manual_ts_);
        return std::put_time(std::gmtime(&t_c), "%F %T");
    }

    Date GetCurrentDate() {
        if (manual_ts_) {
            return {*manual_ts_};
        } else
            throw std::runtime_error("no any date setted");
    }

    void CheckAndReopen(std::chrono::system_clock::time_point&& point) {
        manual_ts_ = std::move(point);
        auto date = GetCurrentDate();
        bool is_new = false;
        if (last_date_) {
            if (*last_date_ != date) is_new = true;
        } else
            is_new = true;

        if (is_new) {
            last_date_ = std::move(date);
            if (logger_.is_open()) logger_.close();
            logger_ = std::ofstream("/var/log/"s + last_date_->GetString(), std::ios_base::app);
            if (!logger_.is_open()) throw std::runtime_error("error open");
        }
    }

    std::string GetFileTimeStamp() const;

    Logger() = default;
    Logger(const Logger&) = delete;

   public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template <class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mt);
        logger_ << GetTimeStamp() << ": "s;
        (logger_ << ... << args);
        logger_ << std::endl;
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts);

   private:
    std::mutex mt;
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream logger_;
    std::optional<Date> last_date_;
};
