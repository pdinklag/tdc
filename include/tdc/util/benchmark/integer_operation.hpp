#pragma once

namespace tdc {
namespace benchmark {

/// \brief An operation code for \em insert operations.
constexpr char OPCODE_INSERT = 'I';

/// \brief An operation code for \em delete operations.
constexpr char OPCODE_DELETE = 'D';

/// \brief An operation code for \em query operations.
constexpr char OPCODE_QUERY = 'Q';

/// \brief Represents an operation on or with an integer.
struct IntegerOperation {
    /// \brief The application-specific operation code.
    char code;
    
    /// \brief The integer key on which to perform the operation.
    uint64_t key;
} __attribute__((packed));

static_assert(sizeof(IntegerOperation) == 9);

}
}
