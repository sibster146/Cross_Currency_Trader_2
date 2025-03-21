#ifndef TRADE_HPP
#define TRADE_HPP


#include <string>
#include <iostream>

enum OrderStatus {
    unfulfilled = 0,
    fulfilled = 1,
    partial = 2
};

struct Trade{
    std::string quote_size = "-1";
    std::string base_size = "-1";
    std::string limit_price = "-1";
    std::string order_id = "-1";
    std::string side = "-1";
    std::string product_id = "-1";
    std::string client_order_id = "-1";
    std::string order_type = "-1";
    std::string trade_created_time = "-1";
    std::string last_fill_time = "-1";
    std::string exchange = "-1";
    OrderStatus fulfilled = unfulfilled;
    Trade(){}
    Trade(const std::string& client_order_id, const std::string& order_type, const std::string& side, 
        const std::string& base_size, const std::string& quote_size, const std::string& limit_price,
        const std::string& product_id, const std::string& exchange, const std::string trade_created_time) : 
        quote_size(quote_size), base_size(base_size), limit_price(limit_price), side(side),
        product_id(product_id), client_order_id(client_order_id), order_type(order_type), trade_created_time(trade_created_time), exchange(exchange) {}

    // Equality operator based on order_id
    bool operator==(const Trade& other) const {
        return order_id == other.order_id;
    }

    std::string printTrade() const {
        std::string status;
        switch (fulfilled) {
            case OrderStatus::unfulfilled: status = "unfulfilled"; break;
            case OrderStatus::fulfilled: status = "fulfilled"; break;
            case OrderStatus::partial: status = "partial"; break;
        }
        std::string res = exchange+","
                            +order_id+","
                            +order_type+","
                            +product_id+","
                            +quote_size+","
                            +base_size+","
                            +limit_price+","
                            +trade_created_time+","
                            +last_fill_time+","
                            +status;
        return res;
    }

};

#endif 