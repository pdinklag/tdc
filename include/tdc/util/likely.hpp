#pragma once

namespace tdc {

/// \brief Shortcut for \c __builtin_expect for expressions that are likely \c true.
#define tdc_likely(x)   __builtin_expect(x, 1)

/// \brief Shortcut for \c __builtin_expect for expressions that are likely \c false.
#define tdc_unlikely(x) __builtin_expect(x, 0)

} // namespace tdc
