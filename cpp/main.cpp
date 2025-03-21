#include "json.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Coinbase.hpp"
#include "Requester/CoinbaseRequester.hpp"
#include <thread>
#include <chrono>
/*

TODO:

*/

int main() {

    // double MAXSIZERATIO = 0.80;

    Coinbase coinbase;

    auto pair = coinbase.exchangeTraversalWithPrune("USD");
    auto path = pair.first;
    if (path.size() == 0){
        std::cout << "No Profitabe Conversion" << std::endl;
        return 0;
    }
    auto yield = pair.second;
    double max_quantity = coinbase.getMaxStartSize(path);

    coinbase.printPath(max_quantity, path);


    return 0;
}