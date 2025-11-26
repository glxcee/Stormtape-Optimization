// io_utils.hpp - small helper: parallel_map / parallel_for_each with bounded concurrency
// Optimized implementation:
// 1. Pre-allocation (no resize overhead).
// 2. Worker Pool pattern (no std::async overhead, no batching barriers).
// 3. Atomic indexing for perfect load balancing without locks (for Random Access Iterators).
// 4. Use std::optional to handle types that are not DefaultConstructible or CopyAssignable.

#pragma once

#include <vector>
#include <thread>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <atomic>
#include <mutex>
#include <optional> // Necessary for handling non-default-constructible result types

namespace storm::io_utils {

// Helper to verify iterator category
template<typename Iter>
using is_random_access = std::is_same<
    typename std::iterator_traits<Iter>::iterator_category, 
    std::random_access_iterator_tag
>;

template <typename Iter, typename Fn>
using invoke_result_t_iter = std::invoke_result_t<Fn, typename std::iterator_traits<Iter>::reference>;

// parallel_map: Optimized Worker Pool implementation
template <typename Iter, typename Fn>
auto parallel_map(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
    -> std::vector<invoke_result_t_iter<Iter, Fn>>
{
    using result_t = invoke_result_t_iter<Iter, Fn>;

    auto dist = std::distance(begin, end);
    if (dist <= 0) return {};

    std::size_t size = static_cast<std::size_t>(dist);
    
    // FIX FOR COMPILATION ERROR:
    // We use vector<optional<T>> instead of vector<T>.
    // This allows pre-allocation (resize) even if T has no default constructor
    // or no assignment operator (like std::pair with const members).
    std::vector<std::optional<result_t>> temp_results;
    temp_results.resize(size);

    // Default concurrency logic
    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }
    // Don't create more threads than items
    std::size_t num_threads = std::min(size, concurrency);

    // Atomic index for lock-free load balancing
    std::atomic<std::size_t> next_idx{0};
    std::mutex iterator_mtx; 

    auto worker = [&]() {
        while (true) {
            // Atomic fetch-and-add: fastest way to distribute work
            std::size_t idx = next_idx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= size) return; // Work finished

            // Get the element to process
            // If vector (RandomAccess), we jump directly: O(1)
            // If list (ForwardIterator), we must lock and advance: O(N) but safe
            Iter current_it = begin;
            if constexpr (is_random_access<Iter>::value) {
                std::advance(current_it, idx);
            } else {
                // Fallback for lists/maps: protect the shared iterator advance
                std::lock_guard<std::mutex> lock(iterator_mtx);
                current_it = begin;
                std::advance(current_it, idx);
            }

            // Execute task and construct result in-place
            // .emplace() bypasses the need for operator= assignment
            temp_results[idx].emplace(fn(*current_it));
        }
    };

    // Thread Pool
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    // Wait for all workers
    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }

    // Move results to the final vector
    // This is a linear pass, but moving is cheap and necessary if T isn't default-constructible.
    std::vector<result_t> final_results;
    final_results.reserve(size);
    for (auto& opt : temp_results) {
        if (opt.has_value()) {
            final_results.push_back(std::move(*opt));
        }
    }

    return final_results;
}

// parallel_for_each: Same logic, just void return
template <typename Iter, typename Fn>
void parallel_for_each(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
{
    auto dist = std::distance(begin, end);
    if (dist <= 0) return;
    
    std::size_t size = static_cast<std::size_t>(dist);

    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }
    std::size_t num_threads = std::min(size, concurrency);

    std::atomic<std::size_t> next_idx{0};
    std::mutex iterator_mtx;

    auto worker = [&]() {
        while (true) {
            std::size_t idx = next_idx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= size) return;

            Iter current_it = begin;
            if constexpr (is_random_access<Iter>::value) {
                std::advance(current_it, idx);
            } else {
                std::lock_guard<std::mutex> lock(iterator_mtx);
                current_it = begin;
                std::advance(current_it, idx);
            }

            fn(*current_it);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }
}

} // namespace storm::io_utils