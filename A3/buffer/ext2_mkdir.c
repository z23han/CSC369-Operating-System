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

int block_bitmap_update(unsigned char *disk, ext2_group_desc * bg, int block_bitmap[]);


int main(int argc, char **argv) {

    // there are 2 args for this command
    if(argc != 3) {
        fprintf(stderr, "ext2_mkdir <image name> <absolute path on the disk>\n");
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

    // store the information in the block_bitmap
    int block_bitmap[sb->s_blocks_count];
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            block_bitmap[8*i+j] = *(disk+1024+(bg->bg_block_bitmap-1)*block_size+i)>>j & 1;
        }
    }


    // initialize array inode_list storing directories and files
    int dir_inode_list[sb->s_inodes_count];
    int file_inode_list[sb->s_inodes_count];
    // inode_cnt used for counting the total number of dir_inode and file_inode
    int dir_inode_cnt = 0;
    int file_inode_cnt = 0;

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
                else {
                    file_inode_list[file_inode_cnt] = i + 1;
                    file_inode_cnt++;
                }
            }
        }
    }



    // get the path and store it into an char array
    char path[sizeof(argv[2])+1];
    strcpy(path, argv[2]);
    char *pch;
    // testing if the first char is '/'
    if (path[0] != '/') {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }

    // I want to get the number of subdirectories the user inputs, including root directory
    // we need to split path by /
    pch = strtok(path, "/");
    int dir_cnt = 1;

    while (1) {
        // I have reached the end
        if (pch == NULL) {
            break;
        }
        pch = strtok(NULL, "/");
        dir_cnt++;
    }
    

    // start to search from inode 2
    int inode_search = 2;
    // reinitialize pch
    strcpy(path, argv[2]);
    pch = strtok(path, "/");
    // initialize pointer pointing to dir_entry
    struct ext2_dir_entry_2 *dir_entry;
    // initialize some variables required
    int i_block_num, inode_block;
    int sum_len, rec_len;
    int dir_flag;
    // count dir layers, and it should be dir_cnt-1 if it is a correct mkdir
    int dir_cnt_2 = 1;


    while (1) {
        // if we have reached the end, we should return error
        if (pch == NULL) {
            fprintf(stderr, "EEXIST\n");
            exit(1);
        }
        dir_flag = 0;
        // search through all the dir_inode_list
        for (i = 0; i < dir_inode_cnt; i++) {
            // if we find the inode_search
            if (dir_inode_list[i] == inode_search) {
                i_block_num = (inode + inode_search - 1)->i_blocks / 2;
                // search through i_block_num
                for (j = 0; j < i_block_num; j++) {
                    // directory entry
                    inode_block = (inode + inode_search - 1)->i_block[j];
                    dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * inode_block);
                    sum_len = 0;
                    rec_len = 0;
                    while (sum_len < 1024) {
                        dir_entry = (void *)dir_entry + rec_len;
                        rec_len = dir_entry->rec_len;
                        if (strncmp(dir_entry->name, pch, dir_entry->name_len) == 0 
                            && strlen(pch) == dir_entry->name_len) {
                            inode_search = dir_entry->inode;
                            dir_flag = 1;
                            dir_cnt_2++;
                            break;
                        } else {
                            sum_len += rec_len;
                        }
                    }
                    // test if it is file, but we don't want file
                    if ((inode+inode_search-1)->i_mode & EXT2_S_IFREG) {
                        fprintf(stderr, "ENOENT\n");
                        exit(1);
                    }
                    // if we have found it, break
                    if (dir_flag == 1) {
                        break;
                    }
                }
                // if we couldn't find it in the dir_entry
                if (dir_flag == 0) {
                    // we check and get the new dir
                    if (dir_cnt_2 == (dir_cnt - 1)) {
                        break;
                    } 
                    // otherwise we cannot find the higher directory
                    else {
                        fprintf(stderr, "ENOENT\n");
                        exit(1);
                    }
                }
            }
            if (dir_flag == 1) {
                break;
            }
        }
        // here we search for it but still couldn't find even the inode_search, return error
        if (dir_flag == 0) {
            if (dir_cnt_2 == (dir_cnt - 1)) {
                break;
            }
            else {
                fprintf(stderr, "ENOENT\n");
                exit(1);
            }
        }
        pch = strtok(NULL, "/");
    }

    // we get the new directory name : pch
    // the directory inode the new directory is located : inode_search, its block # : inode_block
    sum_len = 0;
    rec_len = 0;
    dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * inode_block);
    // we need to loop to the end of the dir_entry
    while (sum_len < 1024) {
        dir_entry = (void *)dir_entry + rec_len;
        rec_len = dir_entry->rec_len;
        sum_len += rec_len;
    }
    //printf("Inode: %d rec_len: %d name_len: %d", inode_search, rec_len, dir_entry->name_len);
    //printf(" name=%.*s\n", dir_entry->name_len, dir_entry->name);
    // variables defining the last dir_entry information
    int last_rec_len;
    if (dir_entry->name_len % 4 == 0) {
        last_rec_len = 8 + dir_entry->name_len;
    } else {
        last_rec_len = 8 + 4 * (dir_entry->name_len / 4 + 1);
    }
    int rest_rec_len = rec_len - last_rec_len;

    // calculate the new directory information
    int new_name_len = strlen(pch);
    int new_rec_len;
    if (new_name_len % 4 != 0) {
        new_rec_len = 4 * (new_name_len / 4 + 1) + 8;
    } else {
        new_rec_len = new_name_len + 8;
    }
    

    // find a new inode to hold the new directory
    // we need to update bg_inode_table
    int new_inode;
    int inode_update = 0;
    for (i = 11; i < 32; i++) {
        // find the first inode=0 starting from #12 (11 in storage)
        if (inode_bitmap[i] == 0) {
            inode_bitmap[i] = 1;
            // update bg->bg_inode_bitmap
            int m, n;
            for (m = 0; m < 4; m++) {
                for (n = 0; n < 8; n++) {
                    if ((8*m + n) == i) {
                        *(disk+1024+(bg->bg_inode_bitmap-1)*block_size+m) |= (0x01 << (n-1));
                        inode_update = 1;
                        break;
                    }
                }
                if (inode_update == 1) {
                    break;
                }
            }
            new_inode = i;
            break;
        }
    }

    // update dir_entry information
    // if the rest of rec_len is large enough
    if (rest_rec_len >= new_rec_len) {
        // update the last dir_entry
        dir_entry->rec_len = last_rec_len;
        // add the new dir_entry
        dir_entry = (void *)dir_entry + last_rec_len;
        dir_entry->inode = new_inode;
        dir_entry->rec_len = rest_rec_len;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_DIR;
        strcpy(dir_entry->name, pch);
    }
    // otherwise we shouldn't change anything inside dir_entry
    // we need to allocate a new block and hold the dir_entry
    else {
        // need to check the first empty block in the block_bitmap
        int new_block = block_bitmap_update(disk, bg, block_bitmap);
        block_bitmap[new_block] = 1;
        // we need to add the new block to the inode_search
        int old_i_blocks = (inode + inode_search - 1)->i_blocks / 2 - 1;
        (inode + inode_search - 1)->i_blocks += 2;
        (inode + inode_search - 1)->i_blocks[old_i_blocks+1] = new_block;
        dir_entry = (struct ext2_dir_entry_2 *)(disk + block_size * new_block);
        // this is a new block, thus we only need to update from beginning
        dir_entry->inode = new_inode;
        dir_entry->rec_len = 1024;
        dir_entry->name_len = new_name_len;
        strcpy(dir_entry->name, pch);
    }


    // then I need to allocate a new block based on the new_inode
    int new_dir_block = block_bitmap_update(disk, bg, block_bitmap);
    block_bitmap[new_dir_block] = 1;

    return 0;
}



// update block_bitmap by allocating a new block
// return block # and update group descriptor
int block_bitmap_update(unsigned char *disk, ext2_group_desc * bg, int block_bitmap[]) {
    int i, j, m, n;
    int block_update = 0;
    int new_block;
    for (i = 0; i < sb->s_blocks_count; i++) {
        if (block_bitmap[i] == 0) {
            for (m = 0; m < 16; m++) {
                for (n = 0; n < 8; n++) {
                    if ((8*m+n) == i) {
                        *(disk+1024+(bg->bg_block_bitmap-1)*block_size+m) |= (0x01 << (n-1));
                        block_update = 1;
                        break;
                    }
                }
                if (block_update == 1) {
                    break;
                }
            }
            new_block = i;
            break;
        }
    }
    return new_block;
}


// 