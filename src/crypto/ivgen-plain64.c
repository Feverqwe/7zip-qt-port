/*
 * QEMU Crypto block IV generator - plain
 * Copyright (c) 2015-2016 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464)
#endif
#include "../C/Precomp.h"

#include "../C/CpuArch.h"
#include "block-qcow.h"
#include <string.h>

void qcrypto_ivgen_plain_calculate(uint64_t sector, unsigned char *iv, size_t niv) {
    unsigned char little[8];
    size_t ivprefix = sizeof(sector);
    SetUi64(little, sector);
    if (ivprefix > niv)
        ivprefix = niv;
    memcpy(iv, little, ivprefix);
    if (ivprefix < niv)
        memset(iv + ivprefix, 0, niv - ivprefix);
}
