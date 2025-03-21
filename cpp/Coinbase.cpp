#include "Requester/CoinbaseRequester.hpp"
#include <mutex>
#include <vector>
#include "json.hpp"
#include "Requester/trade.hpp"
#include "Coinbase.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <algorithm>
#include <stack>
#include <cmath>

Coinbase::Coinbase(){
    //getAccount();
    // populateConversionTable();
    // clearFilledLimitOrdersWorker = std::thread([this](){
    //     while (on){
    //         clearFilledLimitOrders();
    //     }
    // });
    populateConversionTable();
    // populateConversionTableWorker = std::thread([this](){
    //     while (on){
    //         populateConversionTable();
    //     }
    // });
}

Coinbase::~Coinbase(){
    on = false;
    // populateConversionTableWorker.join();
    // clearFilledLimitOrdersWorker.join();
}


double Coinbase::getMaxStartSize(const std::vector<std::string>& path){

    double max_quantity = std::numeric_limits<double>::infinity();
    for (int i = path.size()-2; i >= 0; i--){
        std::string from = path[i];
        std::string to = path[i+1];
        auto [price, size, spread, side, increment] = conversionRateMap[from][to];
        if (side == "sell"){
            max_quantity = std::min(price*size, max_quantity);
            max_quantity = max_quantity/price;
        } else {
            max_quantity = std::min(max_quantity, size);
            max_quantity = max_quantity / price;
        }
    }

    return max_quantity;
}


void Coinbase::printPath(double curr_balance, const std::vector<std::string>& path){
    curr_balance = std::round(curr_balance*1e2) / 1e2;
    double intial_balance = curr_balance;
    // curr_balance = std::min(curr_balance, 50.00);
    std::cout << "Initial Balance: " << curr_balance << std::endl;
    std::vector<std::tuple<double, std::string, std::string>> res_list;

    for (int i = 0; i < path.size()-1; i++){
        std::string from = path[i];
        std::string to = path[i+1];
        auto [price, size, spread, side, increment] = conversionRateMap[from][to];
        curr_balance = (std::floor(curr_balance / increment) * increment);
        std::string product_id;
        if (side == "sell"){
            product_id = from+"-"+to;
            res_list.push_back({curr_balance, product_id, "sell"});
            // std::cout << "  " << from <<"-"<<to<<" "<<price<<" "<<size<< " (SELL) Curr Balance: " << curr_balance << std::endl;
        } else {
            product_id = to+"-"+from;
            res_list.push_back({curr_balance, product_id, "buy"});
            // std::cout << "  " << to <<"-"<<from<<" "<<(1/price)<<" "<<size<<" (BUY) Curr Balance: " << curr_balance << std::endl;
        }
        curr_balance = curr_balance * price;


    }
    
    std::cout << "Final Balance: " << curr_balance << std::endl;
    for (int i = 0; i < res_list.size(); i++){
        auto [num, curr, side] = res_list[i];
        std::cout << num << " " << curr << " " << side << std::endl;
    }
    if (intial_balance > curr_balance){
        std::cout << "NOT PROFITABLE BECAUSE OF TICK SIZE ROUNDING" << std::endl;
    }
}

void Coinbase::buyPath(double curr_balance, const std::vector<std::string>& path){
    curr_balance = std::round(curr_balance*1e2) / 1e2;
    curr_balance = std::min(curr_balance, 50.00);
    std::cout << "Initial Balance: " << curr_balance << std::endl;
    double SPREADRATIO = 0.75;
    for (int i = 0; i < 2; i++){
        std::string from = path[i];
        std::string to = path[i+1];
        auto [limit_price, size, spread, side, increment] = conversionRateMap[from][to];
        if (side == "sell"){
            std::string product_id = from + "-" + to;
            limit_price = limit_price + (spread * SPREADRATIO);
            curr_balance = makeLimitOrder(product_id, "SELL", std::to_string(curr_balance), std::to_string(limit_price));
            std::cout << "  " << from <<"-"<<to<<" "<<limit_price<<" "<<size<< " (SELL) Curr Balance: " << curr_balance << std::endl;
        } else {
            std::string product_id = to + "-" + from;
            limit_price = 1/limit_price;
            limit_price = limit_price - (spread * SPREADRATIO);
            curr_balance = makeLimitOrder(product_id, "BUY", std::to_string(curr_balance), std::to_string(limit_price));
            std::cout << "  " << to <<"-"<<from<<" "<<(limit_price)<<" "<<size<<" (BUY) Curr Balance: " << curr_balance << std::endl;
        }
    }
    std::cout << "Final balance: " << curr_balance << std::endl;

}






std::pair<std::vector<std::string>, double> Coinbase::exchangeTraversalWithPrune(const std::string& start_currency){
    std::unordered_map<std::string, double> best_conversion_yet;
    std::vector<std::string> path_vec;
    std::unordered_set<std::string> path_set;
    std::vector<std::string> final_path;

    exchangeTraversalWithPruneHelper(
        start_currency, 100, best_conversion_yet, 
        path_vec, path_set, final_path, start_currency
    );
    return {final_path,((best_conversion_yet[start_currency] - 100 )/ 100)*100};

}


void Coinbase::exchangeTraversalWithPruneHelper(const std::string& curr_currency, double curr_amount, std::unordered_map<std::string, double>& best_conversion_yet,std::vector<std::string>& path_vec,std::unordered_set<std::string>& path_set,
    std::vector<std::string>& final_path, const std::string& start_currency) {
    if (curr_currency == start_currency && path_vec.size() > 0) {
        best_conversion_yet[curr_currency] = curr_amount;
        path_vec.push_back(curr_currency);
        final_path = path_vec;
        path_vec.pop_back();
        return;
    }

    path_vec.push_back(curr_currency);
    path_set.insert(curr_currency);
    best_conversion_yet[curr_currency] = curr_amount;

    for (auto to_it = conversionRateMap[curr_currency].begin(); to_it != conversionRateMap[curr_currency].end(); to_it++) {
        if (to_it->first != start_currency && path_set.contains(to_it->first)) {
            continue;
        }

        std::string next_currency = to_it->first;
        double next_amount = curr_amount * std::get<0>(to_it->second);
        if (best_conversion_yet.contains(next_currency) && best_conversion_yet[next_currency] >= next_amount)continue;
        
        exchangeTraversalWithPruneHelper(next_currency, next_amount, best_conversion_yet, path_vec, path_set, final_path, start_currency);
    }

    path_vec.pop_back();
    path_set.erase(curr_currency);

}

std::pair<std::vector<std::string>, double> Coinbase::exchangeTraversalWithPruneIter(const std::string& start_currency){

    std::stack<std::tuple<std::string, std::string, double>> stack;
    std::unordered_map<std::string, double> parent_count;
    std::vector<std::string> path;
    std::unordered_set<std::string> pathset;
    std::vector<std::string> final_path;
    std::unordered_map<std::string, double> best_conversion_yet;

    stack.push({"-1", start_currency, 100});
    parent_count["-1"] = 1000000000;

    while (!stack.empty()){
        auto [parent, curr_currency, curr_amount] = stack.top();
        if (parent_count.contains(curr_currency)&&parent_count[curr_currency]==0){
            parent_count.erase(curr_currency);
            pathset.erase(path.back());
            path.pop_back();
            stack.pop();
            parent_count[parent]--;
            continue;
        }



        path.push_back(curr_currency);
        pathset.insert(curr_currency);

        if (curr_currency == start_currency && path.size()>1){
            if (best_conversion_yet[start_currency] < curr_amount){
                best_conversion_yet[start_currency] = curr_amount;
                final_path = path;
            }
            pathset.erase(path.back());
            path.pop_back();
            stack.pop();
            parent_count[parent]--;
            continue;
        }

        best_conversion_yet[curr_currency] = curr_amount;

        bool added = false;

        for (auto to_it = conversionRateMap[curr_currency].begin(); to_it != conversionRateMap[curr_currency].end(); to_it++){
            std::string next_currency = to_it->first;
            double next_amount = curr_amount * std::get<0>(to_it->second);
            if (next_currency != start_currency && pathset.contains(next_currency))continue;
            if (best_conversion_yet.contains(next_currency) && best_conversion_yet[next_currency] >= next_amount)continue;
            added = true;
            stack.push({curr_currency, next_currency, next_amount});
            parent_count[curr_currency] = 1 + (parent_count.count(curr_currency) ? parent_count[curr_currency] : 0);
        }

        if (!added){
            pathset.erase(path.back());
            path.pop_back();
            stack.pop();
            parent_count[parent]--;
        }

    }

    return {final_path, ((best_conversion_yet[start_currency] - 100 )/ 100)*100};

}

std::string Coinbase::timePointToString(const std::chrono::system_clock::time_point& time_point) {
    // Convert time_point to std::time_t (calendar time)
    std::time_t time_t_format = std::chrono::system_clock::to_time_t(time_point);

    // Convert to local time
    std::tm tm_format = *std::localtime(&time_t_format);

    // Format the time into a string
    std::ostringstream oss;
    oss << std::put_time(&tm_format, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}






void Coinbase::clearFilledLimitOrders(){
    nlohmann::json orders = cbr.getOrders();

    for (int i = 0; i < orders.size(); i++){
        nlohmann::json order = orders[i];
        std::string status = order["status"];
        if (status == "FILLED"){
            std::string last_fill_time_str = order["last_fill_time"];
            auto last_fill_time = parseIso8601(last_fill_time_str);
            if (last_update_time < last_fill_time){
                std::string order_id = order["order_id"];
                {
                    std::lock_guard<std::mutex> open_orders_lock(open_orders_mutex);
                    if (open_orders.contains(order_id)){
                        open_orders[order_id].fulfilled = fulfilled;
                        open_orders[order_id].last_fill_time = last_fill_time_str;
                        appendToTradeLog(open_orders[order_id]);
                        open_orders.erase(order_id);
                    }
                }
            }
        }
    }
    last_update_time = std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point Coinbase::parseIso8601(const std::string& iso_time) {
    std::istringstream in(iso_time);
    std::tm tm = {};
    char dot; // To skip the '.' before fractional seconds
    in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S"); // Parse the main part of the timestamp
    if (in.fail()) {
        throw std::runtime_error("Failed to parse time");
    }

    // Parse fractional seconds if present
    int milliseconds = 0;
    if (in.peek() == '.') {
        in >> dot;
        in >> milliseconds;
    }

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return tp + std::chrono::milliseconds(milliseconds);
}


void Coinbase::makeMarketOrder(const std::string& product_id, const std::string& side, const std::string& amount){
    nlohmann::json response = cbr.postMarketOrder(client_order_id, product_id, side, amount);
    if (response["success"]){
        nlohmann::json success_response = response["success_response"];
        std::string order_id = success_response["order_id"];
        std::string order_type = "market_market_ioc";
        nlohmann::json order_config = response["order_configuration"][order_type];
        std::string base_size = order_config["base_size"];
        {
            std::lock_guard<std::mutex> open_orders_lock(open_orders_mutex);
            Trade new_trade(client_order_id, order_type, side, base_size, "-1", "-1", product_id,timePointToString(std::chrono::system_clock::now()), "Coinbase");
            open_orders[order_id] = new_trade;
            appendToTradeLog(new_trade);
        }
    }
}

double Coinbase::makeLimitOrder(const std::string& product_id, const std::string& side, const std::string& amount, const std::string& limit_price){
    nlohmann::json response = cbr.postLimitOrder(client_order_id, product_id, side, amount, limit_price);
    if (response["success"]){
        client_order_id = std::to_string(std::stoi(client_order_id) + 1);
        nlohmann::json success_response = response["success_response"];
        std::string order_id = success_response["order_id"];
        std::string order_type = "limit_limit_gtc";
        nlohmann::json order_config = response["order_configuration"][order_type];
        std::string base_size = order_config["base_size"];
        {
            std::lock_guard<std::mutex> open_orders_lock(open_orders_mutex);
            Trade new_trade(client_order_id, order_type, side, base_size, "-1", limit_price, product_id, timePointToString(std::chrono::system_clock::now()), "Coinbase");
            open_orders[order_id] = new_trade;
            appendToTradeLog(new_trade);
        }
        return std::stod(base_size);
    }
    return -1.0;
}

void Coinbase::appendToTradeLog(const Trade& trade){
    std::lock_guard<std::mutex> tradeloglock(tradelog_mutex);
    const std::string filename = "tradelog.csv";

    // Open the file in append mode
    std::ofstream file(filename, std::ios::app);

    // Check if the file is open
    if (!file.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    file << trade.printTrade() << "\n";

    // Close the file
    file.close();
}

int countDigitsPastDecimal(const std::string& decimalStr) {
    // Find the position of the decimal point
    size_t decimalPos = decimalStr.find('.');
    if (decimalPos == std::string::npos) {
        // No decimal point found
        return 0;
    }

    // Count digits after the decimal point
    int digits = decimalStr.length() - decimalPos - 1;

    // Return 10 raised to the power of the number of digits
    return static_cast<int>(std::pow(10, digits));
}

void Coinbase::populateConversionTable(){
    std::unordered_set<std::string> banned = {"USDC","EUR","GBP"};

    std::unordered_set<std::string> allowed = {"CBETH-ETH", "EURC-USDC", "PAX-USD", "PYUSD-USD", "USDT-USDC", "DAI-USD",
                                                "GUSD-USD", "GYEN-USD", "WBTC-BTC", "LSETH-ETH", "PYUSD-USD", "USDT-USD"};

    nlohmann::json products = cbr.getProducts();

    // std::cout << products << std::endl;

    std::vector<std::string> symbols;

    for (int i = 0; i < products.size(); i++){
        nlohmann::json product = products[i];
        std::string display_name = product["display_name"];
        std::string base_increment_str = product["base_increment"];
        std::string quote_increment_str = product["quote_increment"];
        double base_increment = std::stod(base_increment_str);
        double quote_increment = std::stod(quote_increment_str);
        symbol_increment[display_name] = {base_increment, quote_increment};
        symbols.push_back(display_name);
    }

    nlohmann::json pricebook = cbr.getBBO(symbols);

    for (int i = 0; i < pricebook.size(); i++){
        nlohmann::json price = pricebook[i];
        std::string display_name = price["product_id"];


        // if (!allowed.contains(display_name))continue;


        size_t slash_pos = display_name.find("-");
        if (slash_pos==std::string::npos){
            continue;
        }
        std::string to_currency = display_name.substr(0, slash_pos); // BTC-USD ; BTC is to, USD is from
        std::string from_currency = display_name.substr(slash_pos+1);
        if (banned.contains(to_currency)||banned.contains(from_currency)){
            continue;
        }

        try {
            nlohmann::json ask = price["asks"][0];
            std::string ask_price_str = ask["price"];
            std::string ask_size_str = ask["size"];
            nlohmann::json bid = price["bids"][0];
            std::string bid_price_str = bid["price"];
            std::string bid_size_str = bid["size"];
            double new_ask_price = std::stod(ask_price_str);
            double new_bid_price = std::stod(bid_price_str);
            double new_ask_size = std::stod(ask_size_str);
            double new_bid_size = std::stod(bid_size_str);
            double new_spread = abs(new_ask_price - new_bid_price);

            auto& ask_info = conversionRateMap[from_currency][to_currency];
            auto& bid_info = conversionRateMap[to_currency][from_currency];
            if ((std::get<0>(ask_info) != (1 / new_ask_price)) || (std::get<0>(bid_info) != new_bid_price)){
                std::lock_guard<std::mutex> conversionRateMapLock(conversionRateMapMutex); // USD to BTC 1BTC = 100000 USD
                auto [base_increment, quote_increment] = symbol_increment[display_name];
                // if (new_spread > quote_increment){
                //     new_ask_price -= quote_increment;
                //     new_bid_price += quote_increment;
                // }
                conversionRateMap[from_currency][to_currency] = std::make_tuple(1 / (new_ask_price), new_ask_size, new_spread, "buy", quote_increment);
                conversionRateMap[to_currency][from_currency] = std::make_tuple(new_bid_price, new_bid_size, new_spread, "sell", base_increment);
            }
        }
        catch(...){
            continue;
        }
    } 
}

void Coinbase::getAccount(){
    nlohmann::json accounts = cbr.getAccountInfo();
    std::cout << "Coinbase:" << std::endl;
    for (int i = 0; i < accounts.size(); i++){
        nlohmann::json available_balance = accounts[i]["available_balance"];
        std::string currency = available_balance["currency"];
        std::string value_str = available_balance["value"];
        double value = std::stod(value_str);
        if (value == 0){
            continue;
        }
        balances[currency] = value;
        std::cout << "  " << currency << ": " << value << std::endl;
    }
}


































