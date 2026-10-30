// SPDX-License-Identifier: GPL-2.0-or-later
module;
#include <brotli/decode.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

export module sevenzip.dependencies.brotli;

export {
#include "dependencies/brotli/brotli.h"
}
