#pragma once

#include <memory>

#include "hotdog.h"
#include "result.h"

using namespace std::literals;
using namespace std::chrono_literals;
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order> {
   public:
    Order(net::io_context& io, int id, HotDogHandler handler)
        : io_{io},
          id_{id},
          sausage_{Store::Instance().GetSausage()},
          bread_{Store::Instance().GetBread()},
          strand_(net::make_strand(io)),
          handler_{std::move(handler)} {}

    Result<HotDog> TryToMakeHotDog() {
        if (is_already_cooked_)
            return std::make_exception_ptr(
                std::runtime_error{"Already cooked"});
        if (sausage_->IsCooked() && bread_->IsCooked()) {
            is_already_cooked_ = true;
            return HotDog{sausage_->GetId() * 100 + bread_->GetId(), sausage_,
                          bread_};
        }
        return std::make_exception_ptr(std::runtime_error{
            "Not all ingredients has cocked: "s +
            (sausage_ ? "[sausage] "s : ""s) + (bread_ ? "[bread] "s : ""s)});
    }

    void Execute(std::shared_ptr<GasCooker> cooker) {
        // TODO Все индигриенты сделать списком базового класса и обрабатывать
        // соотвественно
        sausage_->StartFry(*cooker, [self = shared_from_this(), this]() {
            sausage_timer_.reset(new net::steady_timer(
                strand_, HotDog::MIN_SAUSAGE_COOK_DURATION));
            // TODO продумать запуск таймера не по его созданию
            sausage_timer_->async_wait([self, this](sys::error_code ec) {
                sausage_->StopFry();
                if (sausage_->IsCooked() && !ec) {
                    auto result = TryToMakeHotDog();
                    if (result.HasValue()) handler_(result);
                } else
                    handler_(Result<HotDog>(std::make_exception_ptr(
                        std::runtime_error{"Doesn't cooked: "s + ec.what()})));
            });
        });

        bread_->StartFry(*cooker, [self = shared_from_this(), this]() {
            // TODO Все индигриенты сделать списком базового класса и
            // обрабатывать соотвественно
            bread_timer_.reset(new net::steady_timer(
                strand_, HotDog::MIN_BREAD_COOK_DURATION));
            // TODO продумать запуск таймера не по его созданию
            bread_timer_->async_wait([self, this](sys::error_code ec) {
                bread_->StopFry();
                if (bread_->IsCooked() && !ec) {
                    auto result = TryToMakeHotDog();
                    if (result.HasValue()) handler_(result);
                } else
                    handler_(Result<HotDog>(std::make_exception_ptr(
                        std::runtime_error{"Doesn't cooked: "s + ec.what()})));
            });
        });
    }

   private:
    bool is_already_cooked_{false};

    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;

    HotDogHandler handler_;

    int id_;

    net::io_context& io_;
    std::shared_ptr<net::steady_timer> bread_timer_;
    std::shared_ptr<net::steady_timer> sausage_timer_;

    Strand strand_;
};