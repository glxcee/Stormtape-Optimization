// io_utils.hpp - small helper: parallel_map / parallel_for_each with bounded concurrency
// Implementation: std::async with batching (simpler logic).
// Optimization: Result vector pre-allocation + Direct index access (No Mutex/Locks).

#pragma once

#include <future>
#include <vector>
#include <thread>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <cstddef>
#include <optional>

namespace storm::io_utils {

// Determine result type for invoking Fn on reference to iterator value
template <typename Iter, typename Fn>
using invoke_result_t_iter = std::invoke_result_t<Fn, typename std::iterator_traits<Iter>::reference>;

// parallel_map: apply fn to each element in [begin, end), return vector of results.
// Strategy: Batched std::async calls. 
//           Pre-allocates memory to allow lock-free direct writes.
template <typename Iter, typename Fn>
auto parallel_map(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
    -> std::vector<invoke_result_t_iter<Iter, Fn>>
{
    using result_t = invoke_result_t_iter<Iter, Fn>;

    auto dist = std::distance(begin, end);
    if (dist <= 0) return {};
    std::size_t size = static_cast<std::size_t>(dist);

    // FIX 1: Pre-allocate exact size immediately.
    // Using optional allows resizing even if result_t has no default constructor.
    std::vector<std::optional<result_t>> temp_results;
    temp_results.resize(size);

    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }

    // We only need futures to track completion/exceptions, not to return values.
    std::vector<std::future<void>> futures;
    futures.reserve(concurrency);

    std::size_t idx = 0;
    for (auto it = begin; it != end; ++it, ++idx) {
        // FIX 2: Pass 'idx' to the task. 
        // The task writes directly to temp_results[idx]. 
        // No lock is needed because 'idx' is unique for each thread.
        // FIX: Capture iterator 'it' by value instead of '*it' to avoid 
        // creating a const copy of the object, ensuring we pass 
        // the correct reference type (e.g. non-const reference) to 'fn'.
        futures.push_back(std::async(std::launch::async, 
            [&temp_results, idx, &fn, it]() {
                // PROF FIX 3: Direct assignment (using emplace for optional)
                temp_results[idx].emplace(fn(*it));
            }
        ));

        // Simple batching to limit concurrency
        if (futures.size() >= concurrency) {
            for (auto &f : futures) f.get(); // Wait for current batch
            futures.clear();
        }
    }

    // Wait for tail tasks
    for (auto &f : futures) f.get();

    // Unpack results (linear move, very fast)
    std::vector<result_t> final_results;
    final_results.reserve(size);
    for (auto& opt : temp_results) {
        if (opt.has_value()) {
            final_results.push_back(std::move(*opt));
        }
    }

    return final_results;
}

// parallel_for_each: run fn for side-effects, no results collected
// Strategy: Batched std::async calls.
template <typename Iter, typename Fn>
void parallel_for_each(Iter begin, Iter end, Fn fn, std::size_t concurrency = 0)
{
    if (concurrency == 0) {
        auto hc = std::thread::hardware_concurrency();
        concurrency = (hc == 0) ? 2u : std::max(2u, hc);
    }

    std::vector<std::future<void>> futures;
    futures.reserve(concurrency);

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