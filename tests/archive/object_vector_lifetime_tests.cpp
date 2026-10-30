#include "common/collections/MyVector.h"
#include <cstdio>
#include <memory>
#include <new>
#include <utility>

namespace {
struct Entry {
    std::shared_ptr<int> lease;
    static int copiesBeforeFailure;
    explicit Entry(std::shared_ptr<int> value) : lease(std::move(value)) {}
    Entry(const Entry &other) : lease(other.lease) {
        if (copiesBeforeFailure == 0)
            throw std::bad_alloc();
        if (copiesBeforeFailure > 0)
            --copiesBeforeFailure;
    }
};
int Entry::copiesBeforeFailure = -1;
} // namespace

int main() {
    std::weak_ptr<int> resource;
    bool failed = false;
    {
        auto owner = std::make_shared<int>(42);
        resource = owner;
        CObjectVector<Entry> source;
        source.Add(Entry(owner));
        source.Add(Entry(owner));
        owner.reset();
        // The first copied element owns a resource when the next copy throws.
        Entry::copiesBeforeFailure = 1;
        try {
            CObjectVector<Entry> copy(source);
        } catch (const std::bad_alloc &) {
            failed = true;
        }
        Entry::copiesBeforeFailure = -1;
        // Failure must leave the original usable, including another copy.
        CObjectVector<Entry> retry(source);
        if (*retry[0].lease != 42 || *source[1].lease != 42)
            return 1;
    }
    if (!failed) {
        std::fputs("element-copy exception was not propagated\n", stderr);
        return 1;
    }
    if (!resource.expired()) {
        std::fputs("partial vector copy leaked a retained resource\n", stderr);
        return 1;
    }
    return 0;
}
