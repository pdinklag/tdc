#pragma once

#include <chrono>

namespace tdc {
namespace stat {

/// \brief Gets the current system timestamp (in full milliseconds) since the clock's epoch.
inline uint64_t time() {
    using namespace std::chrono;
    
    return uint64_t(duration_cast<milliseconds>(
        high_resolution_clock::now().time_since_epoch()).count());
}

/// \brief Gets the current system timestamp (in full nanoseconds) since the clock's epoch.
inline uint64_t time_nanos() {
    using namespace std::chrono;
    
    return uint64_t(duration_cast<nanoseconds>(
        high_resolution_clock::now().time_since_epoch()).count());
}

/// \brief Gets the current system timestamp (in milliseconds with high resolution) since the clock's epoch.
inline double time_millis() {
    using namespace std::chrono;
    duration<double, std::milli> t = high_resolution_clock::now().time_since_epoch();
    return t.count();
}

/// \brief Gets the current system timestamp (in seconds with high resolution) since the clock's epoch.
inline double time_seconds() {
    using namespace std::chrono;
    duration<double> t = high_resolution_clock::now().time_since_epoch();
    return t.count();
}

}} // namespace tdc::stat
