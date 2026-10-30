// SPDX-License-Identifier: GPL-2.0-or-later
module;
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <expected>
#include <functional>
#include <memory>
// clang-format off
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
// clang-format on
#include <span>
#include <type_traits>

export module sevenzip.dependencies.smb2;

export {
#include "dependencies/smb2/facade.h"
}
