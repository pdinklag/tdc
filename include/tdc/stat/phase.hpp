#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "phase_extension.hpp"
#include "json.hpp"

#include <sys/time.h>

#ifdef __MACH__
    #include <mach/clock.h>
    #include <mach/mach.h>
#endif

namespace tdc {
namespace stat {

/// \brief Represents a runtime statistics phase.
///
/// Phases are used to track statistics like runtime and memory allocations over the course of the application.
/// The measured data is held as JSON objects and can be exported as such, e.g., for
/// use in the tdc charter for visualization, or in other formats for any third party applications.
class Phase {
private:
    //
    // Time measurement statics
    //
    static inline void get_monotonic_time(struct timespec* ts){
    #ifdef __MACH__
        // OS X
        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        ts->tv_sec = mts.tv_sec;
        ts->tv_nsec = mts.tv_nsec;
    #else
        // Linux
        clock_gettime(CLOCK_MONOTONIC, ts);
    #endif
    }

    static inline double current_time_millis() {
        timespec t;
        get_monotonic_time(&t);

        return double(t.tv_sec * 1000L) + double(t.tv_nsec) / double(1000000L);
    }

    //
    // Memory tracking and suppression statics
    //
    static bool s_init;
    static void force_malloc_override_link();
    
    static uint16_t s_suppress_memory_tracking_state;
    static uint16_t s_suppress_tracking_user_state;

    inline static bool is_tracking_memory() {
        return s_suppress_memory_tracking_state == 0 && s_suppress_tracking_user_state == 0;
    }

    struct suppress_memory_tracking {
        inline suppress_memory_tracking(suppress_memory_tracking const&) = delete;
        inline suppress_memory_tracking(suppress_memory_tracking&&) = default;
        
        inline suppress_memory_tracking() {
            ++s_suppress_memory_tracking_state;
        }
        
        inline ~suppress_memory_tracking() {
            --s_suppress_memory_tracking_state;
        }
    };

    struct suppress_tracking_user {
        inline suppress_tracking_user(suppress_tracking_user const&) = delete;
        inline suppress_tracking_user(suppress_tracking_user&&) = default;
        
        inline suppress_tracking_user() {
            if(s_suppress_tracking_user_state++ == 0) {
                if(s_current) s_current->on_pause_tracking();
            }
        }
        
        inline ~suppress_tracking_user() {
            if(s_suppress_tracking_user_state == 1) {
                if(s_current) s_current->on_resume_tracking();
            }
            --s_suppress_tracking_user_state;
        }
    };

    //
    // Extension statics
    //
    using ext_ptr_t = std::unique_ptr<PhaseExtension>;
    static std::vector<std::function<ext_ptr_t()>> s_extension_registry;

public:
    /// \brief Registers a \c PhaseExtension.
    /// \tparam E the extension class type, which must inherit from \c PhaseExtension
    ///
    /// After registration, all phases will include data from the extension.
    template<typename E>
    static void register_extension() {
        if(s_current != nullptr) {
            throw std::runtime_error(
                "Extensions must be registered outside of any "
                "stat measurements!");
        } else {
            s_extension_registry.emplace_back([](){
                return std::make_unique<E>();
            });
        }
    }

private:
    //
    // Current phase
    //
    static Phase* s_current;

    //
    // Members
    //
    std::unique_ptr<std::vector<ext_ptr_t>> m_extensions;
    Phase* m_parent = nullptr;
    double m_pause_time;

    struct {
        double start, end, paused;
    } m_time;

    struct {
        ssize_t off, current, peak;
    } m_mem;

    size_t m_num_allocs;
    size_t m_num_frees;

    std::string m_title;
    std::unique_ptr<json> m_sub;
    std::unique_ptr<json> m_stats;

    bool m_disabled = false;

    // Initialize a phase
    void init(std::string&& title);

    // Finish the current Phase
    void finish();

    // Pause / resume
    void on_pause_tracking();
    void on_resume_tracking();

    // Memory tracking internals
    void track_alloc_internal(size_t bytes);
    void track_free_internal(size_t bytes);

public:
    /// \brief Executes a lambda as a single statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// In case the given lambda accepts a \c StatPhase reference parameter,
    /// the phase object will be passed to it for use during execution.
    ///
    /// \param title the phase title
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto wrap(std::string&& title, F func) ->
        typename std::result_of<F(Phase&)>::type {

        Phase phase(std::move(title));
        return func(phase);
    }

    /// \brief Executes a lambda as a single statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// In case the given lambda accepts a \c StatPhase reference parameter,
    /// the phase object will be passed to it for use during execution.
    ///
    /// \param title the phase title
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto wrap(std::string&& title, F func) ->
        typename std::result_of<F()>::type {

        Phase phase(std::move(title));
        return func();
    }

    /// \brief Tracks a memory allocation of the given size for the current
    ///        phase.
    ///
    /// Use this only if memory is allocated with methods that do not result
    /// in calls of \c malloc but should still be tracked (e.g., when using
    /// direct kernel allocations like memory mappings).
    ///
    /// \param bytes the amount of allocated bytes to track for the current
    ///              phase
    inline static void track_mem_alloc(size_t bytes) {
        if(s_current) s_current->track_alloc_internal(bytes);
    }

    /// \brief Tracks a memory deallocation of the given size for the current
    ///        phase.
    ///
    /// Use this only if memory is allocated with methods that do not result
    /// in calls of \c malloc but should still be tracked (e.g., when using
    /// direct kernel allocations like memory mappings).
    ///
    /// \param bytes the amount of freed bytes to track for the current phase
    inline static void track_mem_free(size_t bytes) {
        if(s_current) s_current->track_free_internal(bytes);
    }

    /// \brief Creates a guard that suppresses tracking as long as it exists.
    ///
    /// This should be used during more complex logging activity in order
    /// for it to not count against memory measures.
    inline static auto suppress_tracking() {
        return suppress_tracking_user();
    }

    /// \brief Suppress tracking while exeucting the lambda.
    ///
    /// This should be used during more complex logging activity in order
    /// for it to not count against memory measures.
    ///
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto suppress_tracking(F func) ->
        typename std::result_of<F()>::type {

        suppress_tracking_user guard;
        return func();
    }

    /// \brief Logs a user statistic for the current phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    inline static void log_current(std::string&& key, const T& value) {
        if(s_current) s_current->log(std::move(key), value);
    }

    /// \brief Creates an inert statistics phase without any effect.
    Phase();

    /// \brief Creates a new statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// \param title the phase title
    Phase(std::string&& title);
    
    /// \brief Destroys and ends the phase.
    ///
    /// The phase's parent phase, if any, will become the current phase.
    ~Phase();

    /// \brief Starts a new phase as a sibling, reusing the same object.
    ///
    /// This function behaves exactly as if the current phase was ended and
    /// a new phases was started immediately after.
    ///
    /// \param new_title the new phase title
    void split(std::string&& new_title);

    /// \brief Logs a user statistic for this phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    void log(std::string&& key, const T& value) {
        if (!m_disabled) {
            suppress_memory_tracking guard;
            (*m_stats)[std::move(key)] = value;
        }
    }

    inline const std::string& title() const {
        return m_title;
    }

    /// \brief Constructs the JSON representation of the measured data.
    ///
    /// It contains the subtree of phases beneath this phase.
    ///
    /// \return the JSON representation of this phase
    json to_json();
};

}} // namespace tdc::stat
