#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    //  block descriptor
    // add block_size since it is allocated after super block
    int block_size = 1024 << sb->s_log_block_size;
    // create a struct to hold the block descriptor
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + block_size);
    // print out information held in the block descriptor
    printf("Block group:\n");
    printf("    block bitmap: %d\n", bg->bg_block_bitmap);
    printf("    inode bitmap: %d\n", bg->bg_inode_bitmap);
    printf("    inode table: %d\n", bg->bg_inode_table);
    printf("    free blocks: %d\n", bg->bg_free_blocks_count);
    printf("    free inodes: %d\n", bg->bg_free_inodes_count);
    printf("    used_dirs: %d\n", bg->bg_used_dirs_count);


    // the block bitmap is located at the first 16 bytes of the block (1024 bytes)
    // every time shift to the right 1 bit and compare with 1
    printf("Block bitmap: ");
    int i, j;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            printf("%d", *(disk+1024+( bg->bg_block_bitmap-1)*block_size+i)>>j & 1);
        }
        printf(" ");
    }
    printf("\n");

    // the inode bitmap is located at the first 4 bytes of the block
    printf("Inode bitmap: ");
    // store all the inode bitmap into an array
    int inode_bitmap[sb->s_inodes_count];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            // right shift 1 bit and store into inode_bitmap
            inode_bitmap[8*i+j] = *(disk+1024+(bg->bg_inode_bitmap-1)*block_size+i)>>j & 1;
            printf("%d", inode_bitmap[8*i+j]);
        }
        printf(" ");
    }
    printf("\n\n");

    // Inodes
    printf("Inodes:"); 
    int dir_inode[sb->s_inodes_count];
    int dir_block[sb->s_inodes_count];
    int dir_cnt = 0;
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*block_size);
    char type;
    for (i = 0; i < sb->s_inodes_count; i++) {
        // need to check if inode bitmap is 1
        if (inode_bitmap[i] == 1) {
            // we only consider inode 2 and inode >= 12
            if (i == 1 || i >= 11) {
                // determine file type
                if ((inode+i)->i_mode & EXT2_S_IFREG) {
                    type = 'f';
                }
                else if ((inode+i)->i_mode & EXT2_S_IFDIR) {
                    type = 'd';
                }
                else {
                    perror("error on file type");
                    exit(1);
                }
                // print inode information
                printf("\n[%d] type: %c size: %d links: %d blocks: %d", 
                    (i+1), type, (inode+i)->i_size, (inode+i)->i_links_count, (inode+i)->i_blocks);
                if ((inode+i)->i_blocks != 0) {
                    printf("\n[%d] Blocks:  ", (i+1));
                    // store num of blocks count in SECTORs
                    int i_block_num = (inode+i)->i_blocks / 2;
                    // if we only have direct blocks
                    if (i_block_num <= 12) {
                        for (j = 0; j < i_block_num; j++) {
                            printf(" %d", (inode+i)->i_block[j]);
                        }
                    }
                    // if we have singly-indirect block
                    else {
                        for (j = 0; j < 12; j++) {
                            printf(" %d", (inode+i)->i_block[j]);
                        }
                        // rest_block_num = 1 
                        int rest_block_num = (int)((inode+i)->i_size / EXT2_BLOCK_SIZE - 12);
                        int singly_indirect = (inode+i)->i_block[12];
                        for (j = 0; j < 128; j++) {
                            if (*(disk + singly_indirect*block_size + j) != 0) {
                                printf(" %d", *(disk + singly_indirect*block_size + j));
                            }
                        }
                    }
                    // add this block # and inode # to array
                    if (type == 'd') {
                        dir_inode[dir_cnt] = i+1;
                        dir_block[dir_cnt] = (inode+i)->i_block[0];
                        dir_cnt++;
                    }
                }
            }
        }
        
    }

    // ex19
    printf("\n\nDirectory Blocks:");

    int sum_len;
    int block_inode, rec_len, name_len;
    char file_type;
    char *name;

    struct ext2_dir_entry_2 *dir_entry;

    for (i = 0; i < dir_cnt; i++) {
        // print out dir block # and dir inode #
        printf("\n   DIR BLOCK NUM: %d (for inode %d)", dir_block[i], dir_inode[i]);
        // get to the directory block
        dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * dir_block[i]);
        // define sum of length
        sum_len = 0;
        rec_len = 0;

        while (sum_len < block_size) {
            // pointer moves to the index 
            dir_entry = (void *)dir_entry + rec_len;
            // get information
            block_inode = dir_entry->inode;
            rec_len = dir_entry->rec_len;
            name_len = dir_entry->name_len;
            if ((dir_entry->file_type & EXT2_FT_DIR) != 0) {
                file_type = 'd';
            } else {
                file_type = 'f';
            }
            name = dir_entry->name;

            printf("\nInode: %d rec_len: %d name_len: %d type= %c", block_inode, rec_len, name_len, file_type);
            printf(" name=%.*s", name_len, name);

            // update sum_len
            sum_len += rec_len;
        }

    }

    
    return 0;
}
