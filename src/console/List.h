// Native property projection of Console/List.cpp; no console/process transport.
// Copyright (c) Igor Pavlov. LGPL-2.1-or-later.
#pragma once
#include "7zip/PropID.h"
#include "Common/MyString.h"
#include "Windows/PropVariant.h"

UString GetListingPropertyName(PROPID id, const wchar_t *name);
UString GetListingPropertyValue(PROPID id, const PROPVARIANT &value);
UString GetListingItemPropertyValue(PROPID id, const PROPVARIANT &value, bool isDir);
