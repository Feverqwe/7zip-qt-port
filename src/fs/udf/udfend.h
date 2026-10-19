/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __UDF_ENDIAN_H
#define __UDF_ENDIAN_H
/* Byte-order services and disk types are supplied by udfdecl.h. */
static inline struct kernel_lb_addr lelb_to_cpu(struct lb_addr in)
{
	struct kernel_lb_addr out;

	out.logicalBlockNum = le32_to_cpu(in.logicalBlockNum);
	out.partitionReferenceNum = le16_to_cpu(in.partitionReferenceNum);

	return out;
}
#endif
