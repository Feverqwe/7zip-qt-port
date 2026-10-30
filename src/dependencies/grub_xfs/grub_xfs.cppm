// SPDX-License-Identifier: GPL-3.0-or-later
module;
#include "formats/xfs/xfsreader.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>

export module sevenzip.dependencies.grub_xfs;

export {
#include "dependencies/grub_xfs/grub_xfs.h"
}
