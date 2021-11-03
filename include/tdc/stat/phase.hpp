#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "phase_extension.hpp"
#include "json.hpp"

namespace tdc {
namespace stat {

/// \brief Represents a runtime statistics phase.
///
/// Phases are used to track statistics like runtime and memory allocations over the course of the application.
/// The measured data is held as JSON objects and can be exported as such, e.g., for
/// use in the tdc charter for visualization, or in other formats for any third party applications.
class Phase {
public:
    /// \brief The stat key used for a phase's title.
    static const std::string STAT_TITLE;
    /// \brief The stat key used for a phase's running time.
    static const std::string STAT_TIME;
    /// \brief The stat key used for a phase's memory offset.
    static const std::string STAT_MEM_OFF;
    /// \brief The stat key used for a phase's memory peak.
    static const std::string STAT_MEM_PEAK;
    /// \brief The stat key used for a phase's final memory counter.
    static const std::string STAT_MEM_FINAL;
    /// \brief The stat key used for a phase's number of allocations.
    static const std::string STAT_NUM_ALLOC;
    /// \brief The stat key used for a phase's number of frees.
    static const std::string STAT_NUM_FREE;
    
    /// \brief Contains information about the phase time measurement.
    struct TimeInfo {
        /// \brief The timestamp of the start of the phase.
        double start;

        /// \brief The timestamp at which this information was polled.
        double current;

        /// \brief The number of milliseconds the phase was paused.
        double paused;

        /// \brief Computes the time elapsed between polling and the start of the phase, excluding any pause times.
        inline double elapsed() { return (current - start) - paused; }
    };

    /// \brief Contains information about the phase memory measurements.
    struct MemoryInfo {
        /// \brief The number of bytes allocated at the start of the phase.
        ssize_t offset;
        
        /// \brief The number of bytes currently allocated.
        ///
        /// If the phase is finished, this is the number of bytes allocated at the end of the phase.
        ssize_t current;
        
        /// \brief The peak number of bytes allocated during the phase.
        ssize_t peak;
        
        /// \brief The number of allocations during the phase.
        size_t num_allocs;
        
        /// \brief The number of frees during the phase.
        size_t num_frees;
    };

private:
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
    /// \brief Registers a \ref PhaseExtension.
    /// \tparam E the extension class type, which must inherit from \ref PhaseExtension.
    ///
    /// After registration, all phases will include data from the extension.
    template<std::derived_from<PhaseExtension> E>
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

    double time_run() const;

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
    /// In case the given lambda accepts a \ref Phase reference parameter,
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
    /// In case the given lambda accepts a \ref Phase reference parameter,
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
    inline static auto suppress() {
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
    inline static auto suppress(F func) ->
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

    Phase(Phase&& other);
    Phase(const Phase&) = delete;
    Phase& operator=(Phase&& other);
    Phase& operator=(const Phase& other) = delete;

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

    /// \brief Gets the current \ref TimeInfo for the phase.
    TimeInfo time_info() const;

    /// \brief Gets the current \ref MemoryInfo for the phase.
    MemoryInfo memory_info() const;

    /// \brief Constructs the JSON representation of the measured data.
    ///
    /// It also contains a subtree of sub-phase data.
    json to_json() const;

    /// \brief Constructs a key=value style string of the measured data.
    ///
    /// Note that this does not contain any sub-phase data.
    std::string to_keyval() const;

    /// \brief Constructs a key=value style strings for phase's subphases.
    ///
    /// The exact format will be <tt>[value_stat]_[title]=[value]</tt>.
    ///
    /// \param value_stat the statistic used as values in the pairs
    /// \param key_stat the statistic used as keys in the pairs, typically \ref STAT_TITLE, must support casting to \c std::string
    std::string subphases_keyval(const std::string& value_stat = STAT_TIME, const std::string& key_stat = STAT_TITLE) const;
};

}} // namespace tdc::stat
