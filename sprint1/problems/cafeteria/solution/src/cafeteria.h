#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "order.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
   public:
    explicit Cafeteria(net::io_context& io) : io_{io} {}

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет
    // готов. Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        const int order_id = next_order_id_++;
        // std::osyncstream{std::cout} << "HotDog preparing to order: " <<
        // order_id
        //                             << std::endl;
        std::make_shared<Order>(io_, order_id, std::move(handler))
            ->Execute(gas_cooker_);
    }

   private:
    int next_order_id_{0};
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая
    // плита на 8 горелок Используйте её для приготовления ингредиентов
    // хот-дога. Плита создаётся с помощью make_shared, так как GasCooker
    // унаследован от enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
