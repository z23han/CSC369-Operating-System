#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/limits.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    // there are 2 args for this command
    if(argc != 3) {
        fprintf(stderr, "ext2_ls <image name> <absolute path on the disk>\n");
        exit(1);
    }
    
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }


    // super block struct pointer
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    int block_size = 1024 << sb->s_log_block_size;
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + block_size);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*block_size);


    // store the inode information in inode_bitmap
    int inode_bitmap[sb->s_inodes_count];
    int i, j;
    // store the information in the inode_bitmap
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            // right shift 1 bit and store into inode_bitmap
            inode_bitmap[8*i+j] = *(disk+1024+(bg->bg_inode_bitmap-1)*block_size+i)>>j & 1;
        }
    }
/*
    // store the information in the block_bitmap
    int block_bitmap[sb->s_blocks_count];
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            block_bitmap[8*i+j] = *(disk+1024+(bg->bg_block_bitmap-1)*block_size+i)>>j & 1;
        }
    }
*/

    // initialize array inode_list storing directories
    int dir_inode_list[sb->s_inodes_count];
    // inode_cnt used for counting the total number of dir_inode
    int dir_inode_cnt = 0;

    for (i = 0; i < sb->s_inodes_count; i++) {
        // check if inode_bitmap is 1
        if (inode_bitmap[i] == 1) {
            // only look at inode 2 and inode >= 12
            if (i == 1 || i >= 11) {
                // if it is directory
                if ((inode+i)->i_mode & EXT2_S_IFDIR) {
                    dir_inode_list[dir_inode_cnt] = i + 1;
                    dir_inode_cnt++;
                }
                // else it is file 
            }
        }
    }


    // get the path and store it into an char array
    char path[sizeof(argv[2])+1];
    strcpy(path, argv[2]);
    char *pch;
    // testing if the first char is '/'
    if (path[0] != '/') {
        fprintf(stderr, "absolute path\n");
        exit(1);
    }

    // assign the starting inode number to be 1, and we track the inode_search in searching
    int inode_search = 2;
    // initialize pointer pointing to dir_entry
    struct ext2_dir_entry_2 *dir_entry;
    struct ext2_dir_entry_2 *last_entry;
    // initialize sum_len, rec_len, inode_flag
    int sum_len, rec_len, file_flag, dir_flag;
    // we need to split path by '/'
    pch = strtok(path, "/");
    // 2 flags indicating block has been found
    file_flag = 0;
    dir_flag = 1;
    // used for recording inode_block, block number
    int inode_block, i_block_num;

    while (1) {
        // if pch = NULL, it means we have reached the end of the path
        if (pch == NULL) {
            break;
        }
        // otherwise we need to justify the inode
        else {
            // we cannot get file_flag = 1 here, since there shouldn't be /afterwards
            if (file_flag == 1) {
                fprintf(stderr, "No such file or diretory\n");
                exit(1);
            }
            // set dir_flag = 0
            dir_flag = 0;
            // we search through all the inode_list
            // search in the dir_inode_list
            for (i = 0; i < dir_inode_cnt; i++) {
                // if we find the inode_search
                if (dir_inode_list[i] == inode_search) {
                    i_block_num = (inode + inode_search - 1)->i_blocks / 2;
                    // we here think directory only have direct block, no indirect
                    for (j = 0; j < i_block_num; j++) {
                        // diretory block need dir_entry
                        inode_block = (inode + inode_search - 1)->i_block[j];
                        dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * inode_block);
                        // check the name inside the block
                        sum_len = 0;
                        rec_len = 0;
                        // loop until sum_len = 1024
                        while (sum_len < 1024) {
                            dir_entry = (void *)dir_entry + rec_len;
                            rec_len = dir_entry->rec_len;
                            // then we compare the name
                            if (strncmp(dir_entry->name, pch, dir_entry->name_len) == 0 
                            && strlen(pch) == dir_entry->name_len) {
                                // we need to print error for checking, since we find the name but len not match 
                                if (strlen(pch) != dir_entry->name_len) {
                                    fprintf(stderr, "No such file or diretory\n");
                                    exit(1);
                                }
                                // we find the directory inode, then we need to update inode_search
                                inode_search = dir_entry->inode;
                                last_entry = dir_entry;
                                dir_flag = 1;
                                break;
                            }
                            else {
                                sum_len += rec_len;
                            }
                        }
                        // test if it is a file
                        if (((inode+inode_search-1)->i_mode & EXT2_S_IFREG) && (dir_flag == 1)) {
                            file_flag = 1;
                            break;
                        }
                        // we need to jump also when dir_flag = 1
                        if (dir_flag == 1) {
                            break;
                        }
                    }
                    // we find the inode_search, but couldn't find the name, still error
                    if (dir_flag == 0) {
                        fprintf(stderr, "No such file or diretory\n");
                        exit(1);
                    }
                }
            }
            // if dir_flag = 0, we fail to find the inode, this returns error
            if (dir_flag == 0) {
                fprintf(stderr, "No such file or diretory\n");
                exit(1);
            }
            // we dont need to search in the file_inode_list, because file has been found in the directory
            // otherwise this would cause error
        }
        // split to the next string layer after delimiter
        pch = strtok(NULL, "/");
    }


    // if it is one file
    if (file_flag == 1) {
        printf("%.*s\n", last_entry->name_len, last_entry->name);
    }
    // else print all the files and directory entries
    else {
        // from inode_search, find inode block
        for (i = 0; i < dir_inode_cnt; i++) {
            if (dir_inode_list[i] == inode_search) {
                // similar procedure and print out all the names
                i_block_num = (inode + inode_search - 1)->i_blocks / 2;
                for (j = 0; j < i_block_num; j++) {
                    inode_block = (inode + inode_search - 1)->i_block[j];
                    dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * inode_block);
                    sum_len = 0;
                    rec_len = 0;
                    while (sum_len < 1024) {
                        dir_entry = (void *)dir_entry + rec_len;
                        printf("%.*s\n", dir_entry->name_len, dir_entry->name);
                        rec_len = dir_entry->rec_len;
                        sum_len += rec_len;
                    }
                }
                break;
            }
        }
    }


    return 0;
}
