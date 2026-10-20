// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "ntfssecuritycompat.h"

const __le16 WOF_NAME[17] = {
	cpu_to_le16('W'), cpu_to_le16('o'), cpu_to_le16('f'), cpu_to_le16('C'),
	cpu_to_le16('o'), cpu_to_le16('m'), cpu_to_le16('p'), cpu_to_le16('r'),
	cpu_to_le16('e'), cpu_to_le16('s'), cpu_to_le16('s'), cpu_to_le16('e'),
	cpu_to_le16('d'), cpu_to_le16('D'), cpu_to_le16('a'), cpu_to_le16('t'),
	cpu_to_le16('a'),
};

const __le16 I30_NAME[4] = {
	cpu_to_le16('$'), cpu_to_le16('I'), cpu_to_le16('3'), cpu_to_le16('0'),
};

const __le16 SII_NAME[4] = {
	cpu_to_le16('$'), cpu_to_le16('S'), cpu_to_le16('I'), cpu_to_le16('I'),
};

const __le16 SDH_NAME[4] = {
	cpu_to_le16('$'), cpu_to_le16('S'), cpu_to_le16('D'), cpu_to_le16('H'),
};

const __le16 SO_NAME[2] = {
	cpu_to_le16('$'), cpu_to_le16('O'),
};

const __le16 SQ_NAME[2] = {
	cpu_to_le16('$'), cpu_to_le16('Q'),
};

const __le16 SR_NAME[2] = {
	cpu_to_le16('$'), cpu_to_le16('R'),
};

int ntfs_fix_post_read(struct NTFS_RECORD_HEADER *rhdr, size_t bytes,
		       bool simple)
{
	int ret;
	u16 *fixup, *ptr;
	u16 sample, fo, fn;

	fo = le16_to_cpu(rhdr->fix_off);
	fn = simple ? ((bytes >> SECTOR_SHIFT) + 1) :
		      le16_to_cpu(rhdr->fix_num);

	/* Check errors. */
	if ((fo & 1) || fo + fn * sizeof(short) > SECTOR_SIZE || !fn-- ||
	    fn * SECTOR_SIZE > bytes) {
		return -E_NTFS_CORRUPT;
	}

	/* Get fixup pointer. */
	fixup = Add2Ptr(rhdr, fo);
	sample = *fixup;
	ptr = Add2Ptr(rhdr, SECTOR_SIZE - sizeof(short));
	ret = 0;

	while (fn--) {
		/* Test current word. */
		if (*ptr != sample) {
			/* Fixup does not match! Is it serious error? */
			ret = -E_NTFS_FIXUP;
		}

		/* Replace fixup. */
		*ptr = *++fixup;
		ptr += SECTOR_SIZE / sizeof(short);
	}

	return ret;
}

static inline u32 sid_length(const struct SID *sid)
{
	return struct_size(sid, SubAuthority, sid->SubAuthorityCount);
}

static bool is_acl_valid(const struct ACL *acl, u32 len)
{
	const struct ACE_HEADER *ace;
	u32 i;
	u16 ace_count, ace_size;

	if (acl->AclRevision != ACL_REVISION &&
	    acl->AclRevision != ACL_REVISION_DS) {
		/*
		 * This value should be ACL_REVISION, unless the ACL contains an
		 * object-specific ACE, in which case this value must be ACL_REVISION_DS.
		 * All ACEs in an ACL must be at the same revision level.
		 */
		return false;
	}

	if (acl->Sbz1)
		return false;

	if (le16_to_cpu(acl->AclSize) > len)
		return false;

	if (acl->Sbz2)
		return false;

	len -= sizeof(struct ACL);
	ace = (struct ACE_HEADER *)&acl[1];
	ace_count = le16_to_cpu(acl->AceCount);

	for (i = 0; i < ace_count; i++) {
		if (len < sizeof(struct ACE_HEADER))
			return false;

		ace_size = le16_to_cpu(ace->AceSize);
		if (len < ace_size)
			return false;

		len -= ace_size;
		ace = Add2Ptr(ace, ace_size);
	}

	return true;
}

bool is_sd_valid(const struct SECURITY_DESCRIPTOR_RELATIVE *sd, u32 len)
{
	u32 sd_owner, sd_group, sd_sacl, sd_dacl;

	if (len < sizeof(struct SECURITY_DESCRIPTOR_RELATIVE))
		return false;

	if (sd->Revision != 1)
		return false;

	if (sd->Sbz1)
		return false;

	if (!(sd->Control & SE_SELF_RELATIVE))
		return false;

	sd_owner = le32_to_cpu(sd->Owner);
	if (sd_owner) {
		const struct SID *owner = Add2Ptr(sd, sd_owner);

		if (sd_owner + offsetof(struct SID, SubAuthority) > len)
			return false;

		if (owner->Revision != 1)
			return false;

		if (sd_owner + sid_length(owner) > len)
			return false;
	}

	sd_group = le32_to_cpu(sd->Group);
	if (sd_group) {
		const struct SID *group = Add2Ptr(sd, sd_group);

		if (sd_group + offsetof(struct SID, SubAuthority) > len)
			return false;

		if (group->Revision != 1)
			return false;

		if (sd_group + sid_length(group) > len)
			return false;
	}

	sd_sacl = le32_to_cpu(sd->Sacl);
	if (sd_sacl) {
		const struct ACL *sacl = Add2Ptr(sd, sd_sacl);

		if (sd_sacl + sizeof(struct ACL) > len)
			return false;

		if (!is_acl_valid(sacl, len - sd_sacl))
			return false;
	}

	sd_dacl = le32_to_cpu(sd->Dacl);
	if (sd_dacl) {
		const struct ACL *dacl = Add2Ptr(sd, sd_dacl);

		if (sd_dacl + sizeof(struct ACL) > len)
			return false;

		if (!is_acl_valid(dacl, len - sd_dacl))
			return false;
	}

	return true;
}

int ntfs_get_security_by_id(struct ntfs_sb_info *sbi, __le32 security_id,
			    struct SECURITY_DESCRIPTOR_RELATIVE **sd,
			    size_t *size)
{
	int err;
	int diff;
	struct ntfs_inode *ni = sbi->security.ni;
	struct ntfs_index *indx = &sbi->security.index_sii;
	void *p = NULL;
	struct NTFS_DE_SII *sii_e;
	struct ntfs_fnd *fnd_sii;
	struct SECURITY_HDR d_security;
	const struct INDEX_ROOT *root_sii;
	u32 t32;

	*sd = NULL;

	mutex_lock_nested(&ni->ni_lock, NTFS_INODE_MUTEX_SECURITY);

	fnd_sii = fnd_get();
	if (!fnd_sii) {
		err = -ENOMEM;
		goto out;
	}

	root_sii = indx_get_root(indx, ni, NULL, NULL);
	if (!root_sii) {
		err = -EINVAL;
		goto out;
	}

	/* Try to find this SECURITY descriptor in SII indexes. */
	err = indx_find(indx, ni, root_sii, &security_id, sizeof(security_id),
			NULL, &diff, (struct NTFS_DE **)&sii_e, fnd_sii);
	if (err)
		goto out;

	if (diff)
		goto out;

	/* The fixed SII view must fit before accessing its SECURITY_HDR. */
	if (le16_to_cpu(sii_e->de.size) < sizeof(*sii_e) +
		((sii_e->de.flags & NTFS_IE_HAS_SUBNODES) ? sizeof(u64) : 0) ||
	    le16_to_cpu(sii_e->de.key_size) < sizeof(security_id)) {
		err = -EINVAL;
		goto out;
	}

	t32 = le32_to_cpu(sii_e->sec_hdr.size);
	if (t32 < sizeof(struct SECURITY_HDR)) {
		err = -EINVAL;
		goto out;
	}

	if (t32 > sizeof(struct SECURITY_HDR) + 0x10000) {
		/* Looks like too big security. 0x10000 - is arbitrary big number. */
		err = -EFBIG;
		goto out;
	}

	*size = t32 - sizeof(struct SECURITY_HDR);

	p = kmalloc(*size, GFP_NOFS);
	if (!p) {
		err = -ENOMEM;
		goto out;
	}

	err = ntfs_read_run_nb(sbi, &ni->file.run,
			       le64_to_cpu(sii_e->sec_hdr.off), &d_security,
			       sizeof(d_security), NULL);
	if (err)
		goto out;

	if (memcmp(&d_security, &sii_e->sec_hdr, sizeof(d_security))) {
		err = -EINVAL;
		goto out;
	}

	err = ntfs_read_run_nb(sbi, &ni->file.run,
			       le64_to_cpu(sii_e->sec_hdr.off) +
				       sizeof(struct SECURITY_HDR),
			       p, *size, NULL);
	if (err)
		goto out;

	*sd = p;
	p = NULL;

out:
	kfree(p);
	fnd_put(fnd_sii);
	ni_unlock(ni);

	return err;
}
