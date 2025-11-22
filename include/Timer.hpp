#pragma once

#include <chrono>
#include <iostream>

template <typename F> auto timeFunction(F &&func) {
    std::cout << "Timing..." << std::flush;
    auto start = std::chrono::high_resolution_clock::now();
    if constexpr (std::is_void_v<std::invoke_result_t<F>>) { // detect if func returns void
        func();
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "\rElapsed time: "
                  << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n"
                  << std::flush;
    } else {
        auto result = func();
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "\rElapsed time: "
                  << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n"
                  << std::flush;
        return result;
    }
}