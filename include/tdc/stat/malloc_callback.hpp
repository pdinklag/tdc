#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdlib>

namespace tdc {
namespace stat {
    /// \brief Called when memory is allocated.
    /// \param bytes the number of bytes being allocated
    void on_alloc(size_t bytes);

    /// \brief Called when memory is freed.
    /// \param bytes the number of bytes being freed
    void on_free(size_t bytes);
}}

/// \cond INTERNAL
extern "C" void* __libc_malloc(size_t);
extern "C" void  __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);
/// \endcond
