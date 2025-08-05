#ifndef LC_CONTEXT_H
#define LC_CONTEXT_H

#include <type_traits>

#include "lc_config.h"

LC_NAMESPACE_BEGIN

template <typename Metadata>
    requires std::is_move_constructible_v<Metadata>
struct Context {
    Metadata metadata;
};

LC_NAMESPACE_END

#endif  // LC_CONTEXT_H
