/* SPDX-License-Identifier: GPL-2.0-only */
// Local initialization boundary; portable SDK AES remains C.
#include "C/Aes.h"
#include "crypto/primitives/sdkaesblocks.h"

namespace {
struct AesTables {
    AesTables() { AesGenTables(); }
};
} // namespace

extern "C" void sevenzip_gpl_aes_init(void) {
    static const AesTables tables;
    (void)tables;
}
