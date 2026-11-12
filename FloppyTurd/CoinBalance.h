#pragma once
#include <algorithm>
namespace CoinBalance {
// Total already includes session coins; session is a subset, never an extra balance.
inline bool Spend(int& total, int& session, int cost, bool sessionOnly = false) {
    if (cost < 0 || total < cost || (sessionOnly && session < cost)) return false;
    total -= cost;
    session -= std::min(session, cost);
    return true;
}
}
