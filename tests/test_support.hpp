#pragma once
#include <stdexcept>
#include <string>
inline void check(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class Operation> void rejects(Operation operation, const char *message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, message);
}
