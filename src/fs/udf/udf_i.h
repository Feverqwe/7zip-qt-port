/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UDF_I_H
#define _UDF_I_H
struct extent_position {
    struct buffer_head *bh;
    uint32_t offset;
    struct kernel_lb_addr block;
};
struct udf_ext_cache {
    /* Extent position */
    struct extent_position epos;
    /* Start logical offset in bytes */
    int64_t lstart;
};
#endif
