#ifndef COINBASE_HPP
#define COINBASE_HPP

#include "Requester/CoinbaseRequester.hpp"
#include "json.hpp"
#include "Requester/trade.hpp"
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>


class Coinbase{
    std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<double,double, double, std::string, double>>> conversionRateMap;
    CoinbaseRequester cbr;
    std::mutex open_orders_mutex;
    std::unordered_map<std::string, Trade> open_orders;
    std::chrono::system_clock::time_point last_update_time = std::chrono::system_clock::now();
    std::thread clearFilledLimitOrdersWorker;
    std::thread populateConversionTableWorker;
    std::mutex conversionRateMapMutex;

    std::atomic<bool> on = true;
    std::mutex tradelog_mutex;
    std::unordered_map<std::string, double> balances;

    std::unordered_map<std::string, double> best_conversion_yet;
    std::vector<std::string> path_vec;
    std::unordered_set<std::string> path_set;
    std::vector<std::string> final_path = {};

    std::string client_order_id = "26";

    std::unordered_map<std::string,std::pair<double, double>> symbol_increment;


public:
    Coinbase();
    ~Coinbase();

    void clearFilledLimitOrders();

    double makeLimitOrder(const std::string& product_id, const std::string& side, const std::string& amount, const std::string& limit_price);

    void makeMarketOrder(const std::string& product_id, const std::string& side, const std::string& amount);

    std::chrono::system_clock::time_point parseIso8601(const std::string& iso_time);

    void appendToTradeLog(const Trade& trade);

    void populateConversionTable();

    void getAccount();

    std::pair<std::vector<std::string>, double> exchangeTraversalWithPrune(const std::string& start_currency);

    void exchangeTraversalWithPruneHelper(const std::string& curr_currency, double curr_amount, std::unordered_map<std::string, double>& best_conversion_yet, std::vector<std::string>& path_vec, 
        std::unordered_set<std::string>& path_set,
        std::vector<std::string>& final_path, const std::string& start_currency);

    double getMaxStartSize(const std::vector<std::string>& path);

    void printPath(double curr_balance, const std::vector<std::string>& path);

    std::string timePointToString(const std::chrono::system_clock::time_point& time_point);

    void buyPath(double curr_balance, const std::vector<std::string>& path);

    std::pair<std::vector<std::string>, double> exchangeTraversalWithPruneIter(const std::string& start_currency);

    // void createPath(double curr_balance, const std::vector<std::string>& path);


};




#endif