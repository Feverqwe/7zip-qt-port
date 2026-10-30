// SPDX-License-Identifier: GPL-2.0-only
module;
#include "formats/erofs/erofsreader.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>

export module sevenzip.dependencies.linux_erofs;

export {
#include "dependencies/linux_erofs/linux_erofs.h"
}
