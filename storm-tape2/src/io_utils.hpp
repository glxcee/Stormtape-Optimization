// io_utils.hpp - small helper: parallel_map / parallel_for_each with bounded concurrency
// Conservative implementation using std::async + batching to limit concurrent tasks.
#pragma once

#include <future>
#include <vector>
#include <thread>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <cstddef>

namespace storm::io_utils {

// Determine result type for invoking Fn on reference to iterator value
template <typename Iter, typename Fn>
using invoke_result_t_iter = std::invoke_result_t<Fn, typename std::iterator_traits<Iter>::reference>;

// parallel_map: apply fn to each element in [begin, end), return vector of results.
// Concurrency: at most 'concurrency' tasks are scheduled concurrently.
template <typename Iter, typename Fn>
auto parallel_map(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
    -> std::vector<invoke_result_t_iter<Iter, Fn>>
{
    using result_t = invoke_result_t_iter<Iter, Fn>;

    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }

    std::vector<result_t> results;
    auto dist = std::distance(begin, end);
    results.reserve(static_cast<std::size_t>(dist));

    std::vector<std::future<result_t>> futures;
    futures.reserve(std::min<std::size_t>(static_cast<std::size_t>(dist), concurrency));

    for (auto it = begin; it != end; ++it) {
        futures.push_back(std::async(std::launch::async, fn, std::ref(*it)));
        if (futures.size() >= concurrency) {
           for (auto &f : futures) {
               results.push_back(f.get());
            }
            futures.clear();
        }
    }
    for (auto &f : futures) {
        results.push_back(f.get());
    }

    return results;
}

// parallel_for_each: run fn for side-effects, no results collected
template <typename Iter, typename Fn>
void parallel_for_each(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
{
    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }

    std::vector<std::future<void>> futures;
    futures.reserve(std::min<std::size_t>(static_cast<std::size_t>(std::distance(begin, end)), concurrency));

    for (auto it = begin; it != end; ++it) {
        futures.push_back(std::async(std::launch::async, fn, std::ref(*it)));
        if (futures.size() >= concurrency) {
            for (auto &f : futures) f.get();
            futures.clear();
        }
    }
    for (auto &f : futures) f.get();
}

} // namespace storm::io_utils