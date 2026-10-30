// SPDX-License-Identifier: GPL-2.0-only
/*
 * Helpers for formatting and printing strings
 *
 * Copyright 31 August 2008 James Bottomley
 * Copyright (C) 2013, Intel Corporation
 */
#include "crypto/primitives/userspace.h"

/**
 * match_string - matches given string in an array
 * @array:	array of strings
 * @n:		number of strings in the array or -1 for NULL terminated arrays
 * @string:	string to match with
 *
 * This routine will look for a string in an array of strings up to the
 * n-th element in the array or until the first NULL element.
 *
 * Historically the value of -1 for @n, was used to search in arrays that
 * are NULL terminated. However, the function does not make a distinction
 * when finishing the search: either @n elements have been compared OR
 * the first NULL element was found.
 *
 * Return:
 * index of a @string in the @array if matches, or %-EINVAL otherwise.
 */
int match_string(const char * const *array, size_t n, const char *string)
{
	int index;
	const char *item;

	for (index = 0; index < n; index++) {
		item = array[index];
		if (!item)
			break;
		if (!strcmp(item, string))
			return index;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(match_string);
