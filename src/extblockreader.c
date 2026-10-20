/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive ownership and callbacks for Linux system-zone validation. */
#include "extreaderprivate.h"
#include "fs/ext4/blockread.h"

struct inode *ext4_iget(struct super_block *sb, unsigned long ino, ext4_iget_flags flags)
{
    const struct ext_block_services *services = EXT4_SB(sb)->block_services;
    struct ext_node *node = NULL;
    struct ext_node_info info;
    void *raw;
    int error;
    if (!services || !services->inode || ino > UINT32_MAX) return ERR_PTR(-EIO);
    raw = malloc(EXT4_INODE_SIZE(sb));
    if (!raw) return ERR_PTR(-ENOMEM);
    error = services->inode(services->context, (u32)ino, raw, EXT4_INODE_SIZE(sb));
    if (!error) error = ext_linux_inode_open((struct ext_volume *)sb, (u32)ino, raw, &node, &info);
    free(raw);
    if (!error) error = check_igot_inode(&node->inode, flags, __func__, __LINE__);
    if (error) { ext_linux_inode_close(node); return ERR_PTR(error); }
    return &node->inode;
}
void iput(struct inode *inode) { ext_linux_inode_close((struct ext_node *)inode); }
int ext4_map_blocks(void *handle, struct inode *inode, struct ext4_map_blocks *map, int flags)
{
    const struct ext_block_services *services = EXT4_SB(inode->i_sb)->block_services;
    int result;
    (void)handle; (void)flags;
    if (!services || !services->map) return -EIO;
    result = services->map(services->context, inode->i_ino, map->m_lblk, &map->m_len, &map->m_pblk);
    if (result < 0) return result;
    if (!map->m_len || map->m_len > INT_MAX) return -EIO;
    return map->m_pblk ? (int)map->m_len : 0;
}
int ext_linux_system_zone(struct ext_volume *volume, const void *descriptors, uint32_t groups,
                          const struct ext_block_services *services)
{
    struct super_block *sb = &volume->sb;
    struct ext4_sb_info *sbi = EXT4_SB(sb);
    u64 blocks = ext4_blocks_count(sbi->s_es);
    u32 first = le32_to_cpu(sbi->s_es->s_first_data_block);
    u32 per_group = le32_to_cpu(sbi->s_es->s_blocks_per_group);
    size_t size = (size_t)groups * sbi->s_desc_size;
    int error;
    if (sbi->s_group_desc || !per_group || blocks <= first || !groups ||
        groups != (blocks - first + per_group - 1) / per_group ||
        size / sbi->s_desc_size != groups) return -EIO;
    sbi->s_group_desc = malloc(size);
    if (!sbi->s_group_desc) return -ENOMEM;
    memcpy(sbi->s_group_desc, descriptors, size);
    sbi->s_groups_count = groups;
    sbi->s_gdb_count = (groups + sbi->s_desc_per_block - 1) / sbi->s_desc_per_block;
    sbi->s_itb_per_group = EXT4_INODES_PER_GROUP(sb) / (sb->s_blocksize / sbi->s_inode_size);
    if (!ext4_check_descriptors(sb, sb->s_blocksize == 1024 ? 1 : 0, NULL)) {
        error = -EIO;
        goto fail;
    }
    sbi->block_services = services;
    error = ext4_setup_system_zone(sb);
    sbi->block_services = NULL;
    if (!error) return 0;
fail:
    free(sbi->s_group_desc);
    sbi->s_group_desc = NULL;
    return error;
}
int ext_linux_block_valid(struct ext_volume *volume, uint32_t ino, uint64_t start, uint32_t count)
{
    struct inode inode = { .i_sb = &volume->sb, .i_ino = ino };
    return ext4_inode_block_valid(&inode, start, count);
}
int ext_linux_blockrefs(struct ext_volume *volume, uint32_t ino, const void *refs, uint32_t count)
{
    struct super_block sb = volume->sb;
    struct inode inode = { .i_sb = &sb, .i_ino = ino };
    journal_t journal = { .j_inode = &inode };
    int result;
    if (ext4_has_feature_journal(&volume->sb) && ino == le32_to_cpu(volume->raw.s_journal_inum))
        sb.info.s_journal = &journal;
    result = ext4_check_blockref(__func__, __LINE__, &inode, (__le32 *)refs, count);
    return result;
}
