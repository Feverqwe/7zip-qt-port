// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/hfsplus/unicode.c
 *
 * Copyright (C) 2001
 * Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * Handler routines for unicode strings
 */

#include "hfscompat.h"

/* Fold the case of a unicode char, given the 16 bit value */
/* Returns folded char, or 0 if ignorable */
static inline u16 case_fold(u16 c)
{
	u16 tmp;

	tmp = hfsplus_case_fold_table[c >> 8];
	if (tmp)
		tmp = hfsplus_case_fold_table[tmp + (c & 0xff)];
	else
		tmp = c;
	return tmp;
}

/* Compare unicode strings, return values like normal strcmp */
int hfsplus_strcasecmp(const struct hfsplus_unistr *s1,
		       const struct hfsplus_unistr *s2)
{
	u16 len1, len2, c1, c2;
	const hfsplus_unichr *p1, *p2;

	len1 = be16_to_cpu(s1->length);
	len2 = be16_to_cpu(s2->length);
	p1 = s1->unicode;
	p2 = s2->unicode;

	if (len1 > HFSPLUS_MAX_STRLEN) {
		len1 = HFSPLUS_MAX_STRLEN;
		pr_err("invalid length %u has been corrected to %d\n",
			be16_to_cpu(s1->length), len1);
	}

	if (len2 > HFSPLUS_MAX_STRLEN) {
		len2 = HFSPLUS_MAX_STRLEN;
		pr_err("invalid length %u has been corrected to %d\n",
			be16_to_cpu(s2->length), len2);
	}

	while (1) {
		c1 = c2 = 0;

		while (len1 && !c1) {
			c1 = case_fold(be16_to_cpu(*p1));
			p1++;
			len1--;
		}
		while (len2 && !c2) {
			c2 = case_fold(be16_to_cpu(*p2));
			p2++;
			len2--;
		}

		if (c1 != c2)
			return (c1 < c2) ? -1 : 1;
		if (!c1 && !c2)
			return 0;
	}
}
EXPORT_SYMBOL_IF_KUNIT(hfsplus_strcasecmp);

/* Compare names as a sequence of 16-bit unsigned integers */
int hfsplus_strcmp(const struct hfsplus_unistr *s1,
		   const struct hfsplus_unistr *s2)
{
	u16 len1, len2, c1, c2;
	const hfsplus_unichr *p1, *p2;
	int len;

	len1 = be16_to_cpu(s1->length);
	len2 = be16_to_cpu(s2->length);
	p1 = s1->unicode;
	p2 = s2->unicode;

	if (len1 > HFSPLUS_MAX_STRLEN) {
		len1 = HFSPLUS_MAX_STRLEN;
		pr_err("invalid length %u has been corrected to %d\n",
			be16_to_cpu(s1->length), len1);
	}

	if (len2 > HFSPLUS_MAX_STRLEN) {
		len2 = HFSPLUS_MAX_STRLEN;
		pr_err("invalid length %u has been corrected to %d\n",
			be16_to_cpu(s2->length), len2);
	}

	for (len = min(len1, len2); len > 0; len--) {
		c1 = be16_to_cpu(*p1);
		c2 = be16_to_cpu(*p2);
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		p1++;
		p2++;
	}

	return len1 < len2 ? -1 :
	       len1 > len2 ? 1 : 0;
}
EXPORT_SYMBOL_IF_KUNIT(hfsplus_strcmp);

#define Hangul_SBase	0xac00
#define Hangul_LBase	0x1100
#define Hangul_VBase	0x1161
#define Hangul_TBase	0x11a7
#define Hangul_SCount	11172
#define Hangul_LCount	19
#define Hangul_VCount	21
#define Hangul_TCount	28
#define Hangul_NCount	(Hangul_VCount * Hangul_TCount)


static u16 *hfsplus_compose_lookup(u16 *p, u16 cc)
{
	int i, s, e;

	s = 1;
	e = p[1];
	if (!e || cc < p[s * 2] || cc > p[e * 2])
		return NULL;
	do {
		i = (s + e) / 2;
		if (cc > p[i * 2])
			s = i + 1;
		else if (cc < p[i * 2])
			e = i - 1;
		else
			return hfsplus_compose_table + p[i * 2 + 1];
	} while (s <= e);
	return NULL;
}

/*
 * In HFS+, a filename can contain / because : is the separator.
 * The slash is a valid filename character on macOS.
 * But on Linux, / is the path separator and
 * it cannot appear in a filename component.
 * There's a parallel mapping for the NUL character (0 -> U+2400).
 * NUL terminates strings in C/POSIX but is valid in HFS+ filenames.
 */
static inline
void hfsplus_mac2linux_compatibility_check(u16 symbol, u16 *conversion,
					   int name_type)
{
	*conversion = symbol;

	switch (name_type) {
	case HFS_XATTR_NAME:
		/* ignore conversion */
		return;

	default:
		/* continue logic */
		break;
	}

	switch (symbol) {
	case 0:
		*conversion = 0x2400;
		break;
	case '/':
		*conversion = ':';
		break;
	}
}

static int hfsplus_uni2asc(struct super_block *sb,
			   const struct hfsplus_unistr *ustr,
			   int max_len, char *astr, int *len_p,
			   int name_type)
{
	const hfsplus_unichr *ip;
	struct nls_table *nls = HFSPLUS_SB(sb)->nls;
	u8 *op;
	u16 cc, c0, c1;
	u16 *ce1, *ce2;
	int i, len, ustrlen, res, compose;

	op = (u8 *)astr;
	ip = ustr->unicode;

	ustrlen = be16_to_cpu(ustr->length);
	if (ustrlen > max_len) {
		ustrlen = max_len;
		pr_err("invalid length %u has been corrected to %d\n",
			be16_to_cpu(ustr->length), ustrlen);
	}

	len = *len_p;
	ce1 = NULL;
	compose = !test_bit(HFSPLUS_SB_NODECOMPOSE, &HFSPLUS_SB(sb)->flags);

	while (ustrlen > 0) {
		c0 = be16_to_cpu(*ip++);
		ustrlen--;
		/* search for single decomposed char */
		if (likely(compose))
			ce1 = hfsplus_compose_lookup(hfsplus_compose_table, c0);
		if (ce1)
			cc = ce1[0];
		else
			cc = 0;
		if (cc) {
			/* start of a possibly decomposed Hangul char */
			if (cc != 0xffff)
				goto done;
			if (!ustrlen)
				goto same;
			c1 = be16_to_cpu(*ip) - Hangul_VBase;
			if (c1 < Hangul_VCount) {
				/* compose the Hangul char */
				cc = (c0 - Hangul_LBase) * Hangul_VCount;
				cc = (cc + c1) * Hangul_TCount;
				cc += Hangul_SBase;
				ip++;
				ustrlen--;
				if (!ustrlen)
					goto done;
				c1 = be16_to_cpu(*ip) - Hangul_TBase;
				if (c1 > 0 && c1 < Hangul_TCount) {
					cc += c1;
					ip++;
					ustrlen--;
				}
				goto done;
			}
		}
		while (1) {
			/* main loop for common case of not composed chars */
			if (!ustrlen)
				goto same;
			c1 = be16_to_cpu(*ip);
			if (likely(compose))
				ce1 = hfsplus_compose_lookup(
					hfsplus_compose_table, c1);
			if (ce1)
				break;
			hfsplus_mac2linux_compatibility_check(c0, &c0,
							      name_type);
			res = nls->uni2char(c0, op, len);
			if (res < 0) {
				if (res == -ENAMETOOLONG)
					goto out;
				*op = '?';
				res = 1;
			}
			op += res;
			len -= res;
			c0 = c1;
			ip++;
			ustrlen--;
		}
		ce2 = hfsplus_compose_lookup(ce1, c0);
		if (ce2) {
			i = 1;
			while (i < ustrlen) {
				ce1 = hfsplus_compose_lookup(ce2,
					be16_to_cpu(ip[i]));
				if (!ce1)
					break;
				i++;
				ce2 = ce1;
			}
			cc = ce2[0];
			if (cc) {
				ip += i;
				ustrlen -= i;
				goto done;
			}
		}
same:
		hfsplus_mac2linux_compatibility_check(c0, &cc,
						      name_type);
done:
		res = nls->uni2char(cc, op, len);
		if (res < 0) {
			if (res == -ENAMETOOLONG)
				goto out;
			*op = '?';
			res = 1;
		}
		op += res;
		len -= res;
	}
	res = 0;
out:
	*len_p = (char *)op - astr;
	return res;
}

int hfsplus_uni2asc_str(struct super_block *sb,
			       const struct hfsplus_unistr *ustr, char *astr,
			       int *len_p)
{
	return hfsplus_uni2asc(sb,
				ustr, HFSPLUS_MAX_STRLEN,
				astr, len_p,
				HFS_REGULAR_NAME);
}
EXPORT_SYMBOL_IF_KUNIT(hfsplus_uni2asc_str);

int hfsplus_uni2asc_xattr_str(struct super_block *sb,
				     const struct hfsplus_attr_unistr *ustr,
				     char *astr, int *len_p)
{
	return hfsplus_uni2asc(sb, (const struct hfsplus_unistr *)ustr,
				HFSPLUS_ATTR_MAX_STRLEN, astr, len_p,
				HFS_XATTR_NAME);
}
EXPORT_SYMBOL_IF_KUNIT(hfsplus_uni2asc_xattr_str);
