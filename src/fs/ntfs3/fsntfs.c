// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "ntfscompat.h"

const __le16 WOF_NAME[17] = {
	cpu_to_le16('W'), cpu_to_le16('o'), cpu_to_le16('f'), cpu_to_le16('C'),
	cpu_to_le16('o'), cpu_to_le16('m'), cpu_to_le16('p'), cpu_to_le16('r'),
	cpu_to_le16('e'), cpu_to_le16('s'), cpu_to_le16('s'), cpu_to_le16('e'),
	cpu_to_le16('d'), cpu_to_le16('D'), cpu_to_le16('a'), cpu_to_le16('t'),
	cpu_to_le16('a'),
};
