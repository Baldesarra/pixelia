#include "pixelia/engine/memory.h"

#include <cstdlib>

namespace pixelia {

namespace {
AllocFn s_alloc = std::malloc;
FreeFn s_free = std::free;
}  // namespace

void set_asset_allocator(AllocFn alloc, FreeFn release) {
    s_alloc = alloc ? alloc : std::malloc;
    s_free = release ? release : std::free;
}

void* asset_alloc(size_t bytes) { return s_alloc(bytes); }

void asset_free(void* p) { s_free(p); }

}  // namespace pixelia
