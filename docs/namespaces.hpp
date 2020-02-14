namespace tdc {

/// \brief Callback functions for memory allocation tracking.
///
/// These can be defined to manually track memory allocations in case
/// \c tdc::stat is not used.
namespace malloc_callback {}

/// \brief Energy measurement using the Intel RAPL interface.
namespace rapl {}

/// \brief Runtime statistics tracking.
namespace stat {}

}
