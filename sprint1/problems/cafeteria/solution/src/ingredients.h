#pragma once
#include <atomic>
#include <functional>
#include <iostream>
#include <optional>

#include "clock.h"
#include "gascooker.h"

/*
Класс "Сосиска".
Позволяет себя обжаривать на газовой плите
*/

class IngredientUsingGas
    : public std::enable_shared_from_this<IngredientUsingGas> {
   public:
    using Handler = std::function<void()>;

    explicit IngredientUsingGas(int id) : id_{id} {}

    int GetId() const { return id_; }

    // Асинхронно начинает приготовление. Вызывает handler, как только началось
    // приготовление
    void StartFry(GasCooker& cooker, Handler handler) {
        // Метод StartFry можно вызвать только один раз
        if (frying_start_time_) {
            throw std::logic_error("Frying already started");
        }

        // Запрещаем повторный вызов StartFry
        frying_start_time_ = Clock::now();

        // Готовимся занять газовую плиту
        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку для начала обжаривания.
        // Чтобы продлить жизнь текущего объекта, захватываем shared_ptr в
        // лямбде
        cooker.UseBurner(
            [self = shared_from_this(), handler = std::move(handler)] {
                // Запоминаем время фактического начала обжаривания
                self->frying_start_time_ = Clock::now();
                handler();
            });
    }

    // Завершает приготовление и освобождает горелку
    void StopFry() {
        if (!frying_start_time_) {
            throw std::logic_error("Frying has not started");
        }
        if (frying_end_time_) {
            throw std::logic_error("Frying has already stopped");
        }
        frying_end_time_ = Clock::now();

        gas_cooker_lock_.Unlock();
    }

    bool IsCooked() const noexcept {
        return frying_start_time_.has_value() && frying_end_time_.has_value();
    }

    Clock::duration GetCookDuration() const {
        if (!frying_start_time_ || !frying_end_time_) {
            throw std::logic_error("Ingredient has not been cooked");
        }
        auto time = *frying_end_time_ - *frying_start_time_;
        // auto cnt =
        //     std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
        return time;
    }

   private:
    int id_;
    GasCookerLock gas_cooker_lock_;
    std::optional<Clock::time_point> frying_start_time_;
    std::optional<Clock::time_point> frying_end_time_;
};

class Sausage : public IngredientUsingGas {
   public:
    Sausage(int id) : IngredientUsingGas(id) {}
};

class Bread : public IngredientUsingGas {
   public:
    Bread(int id) : IngredientUsingGas(id) {}
};

// Склад ингредиентов (возвращает ингредиенты с уникальным id)
class Store {
   public:
    static Store& Instance() {
        static Store store;
        return store;
    }

    std::shared_ptr<Bread> GetBread() & {
        return std::make_shared<Bread>(++next_id_);
    }

    std::shared_ptr<Sausage> GetSausage() & {
        return std::make_shared<Sausage>(++next_id_);
    }

   private:
    Store(){};
    std::atomic_int next_id_ = 0;
};
