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

int *inode_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int *block_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int count_num_dir(char path[]);
int new_inode_search(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *bg, int* inode_bitmap);
int new_block_search(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *bg, int* block_bitmap);
int update_dir_entry_inode(unsigned char *disk, char *pch, int* block_bitmap, int* inode_bitmap, int block_info[]);
int update_inode_table(unsigned char *disk, int new_inode, int parent_block, int* block_bitmap);
int update_new_inode_block(unsigned char *disk, int new_inode, int new_block, int parent_inode);



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
    int *inode_bitmap = inode_bitmap_setup(disk, bg);
    // store block information in the block_bitmap
    int *block_bitmap = block_bitmap_setup(disk, bg);
    
    // initialize array inode_list storing directories
    int dir_inode_list[sb->s_inodes_count];
    // inode_cnt used for counting the total number of dir_inode
    int dir_inode_cnt = 0;
    int i, j;

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
    // testing if the first char is '/'
    if (path[0] != '/') {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }

    int dir_cnt = count_num_dir(path);
    

    // search for the last directory 
    char *pch;
    strcpy(path, argv[2]);
    pch = strtok(path, "/");
    // initialize pointer pointing to dir_entry
    struct ext2_dir_entry_2 *dir_entry;
    // initialize some variables required
    int i_block_num, inode_block;
    int sum_len, rec_len;
    int dir_flag = 1;
    // count dir layers, and it should be dir_cnt-1 if it is a correct mkdir
    int dir_cnt_2 = 1;
    // start to search from inode 2
    int inode_search = 2;

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
    int block_info[2] = {inode_search, inode_block};

    int new_inode = update_dir_entry_inode(disk, pch, block_bitmap, inode_bitmap, block_info);
    int parent_inode = inode_search;
    int parent_block = inode_block;
    int new_block = update_inode_table(disk, new_inode, parent_block, block_bitmap);
    update_new_inode_block(disk, new_inode, new_block, parent_inode);
    // lastly, we need to update the used_dirs
    bg->bg_used_dirs_count++;


    free(inode_bitmap);
    free(block_bitmap);

    return 0;
}


// store inode information in the inode_bitmap
int *inode_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg) {
    int i, j;
    int *inode_bitmap =(int *)malloc(sizeof(int) * 32);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            // right shift 1 bit and store into inode_bitmap
            *(inode_bitmap + 8*i + j) = *(disk+1024+(bg->bg_inode_bitmap-1)*1024+i)>>j & 1;
        }
    }
    return inode_bitmap;
}


// store block information in the block_bitmap
int *block_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg) {
    int i, j;
    int *block_bitmap = (int *)malloc(sizeof(int) * 128);
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            *(block_bitmap + 8*i + j) = *(disk+1024+(bg->bg_block_bitmap-1)*1024+i)>>j & 1;
        }
    }
    return block_bitmap;
}


// count the number of directory
int count_num_dir(char path[]) {
    char *pch;
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
    return dir_cnt;
}


// update the dir_entry which holds the new directory and return dir_entry
int update_dir_entry_inode(unsigned char *disk, char *pch, int* block_bitmap, int* inode_bitmap, int block_info[]) {
    int sum_len = 0;
    int rec_len = 0;
    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * block_info[1]);
    while (sum_len < 1024) {
        dir_entry = (void *)dir_entry + rec_len;
        rec_len = dir_entry->rec_len;
        sum_len += rec_len;
    }
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
    if (new_name_len % 4 == 0) {
        new_rec_len = new_name_len + 8;
    } else {
        new_rec_len = 4 * (new_name_len / 4 + 1) + 8;
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    int new_inode = new_inode_search(disk, sb, bg, inode_bitmap);
    // justify if we can add the new directory in this block or apply for a new block based on rec_len
    // simply add it
    if (rest_rec_len >= new_rec_len) {
        dir_entry->rec_len = last_rec_len;
        dir_entry = (void *)dir_entry + last_rec_len;
        // update all the information
        dir_entry->inode = new_inode;
        dir_entry->rec_len = rest_rec_len;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_DIR;
        strcpy(dir_entry->name, pch);
    }
    // else we need to allocate a new block
    else {
        int new_block = new_block_search(disk, sb, bg, block_bitmap);
        // we also need to update the current directory inode (block_info[2] = {inode_search, inode_block})
        struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);
        (inode + block_info[0] - 1)->i_blocks += 2;
        int i_block_num = (inode + block_info[0] - 1)->i_blocks / 2;
        (inode + block_info[0] - 1)->i_block[i_block_num-1] = new_block;
        dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * new_block);
        // update the new directory block
        dir_entry->inode = new_inode;
        dir_entry->rec_len = 1024;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_DIR;
        strcpy(dir_entry->name, pch);
    }

    return new_inode;
}


// update inode_table
int update_inode_table(unsigned char *disk, int new_inode, int parent_block, int* block_bitmap) {
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);
    // from the new_inode allocated to this directory and go to that inode
    int new_block = new_block_search(disk, sb, bg, block_bitmap);
    (inode + new_inode - 1)->i_mode = EXT2_S_IFDIR;
    (inode + new_inode - 1)->i_size = 1024;
    (inode + new_inode - 1)->i_links_count = 2;
    (inode + new_inode - 1)->i_blocks = 2;
    (inode + new_inode - 1)->i_block[0] = new_block;
    (inode + new_inode - 1)->i_block[1] = parent_block;

    return new_block;
}


// update the block information based on new_block and new_inode
int update_new_inode_block(unsigned char *disk, int new_inode, int new_block, int parent_inode) {
    
    // go to the block entry
    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * new_block);
    // initialize '.'
    dir_entry->inode = new_inode;
    dir_entry->rec_len = 12;
    dir_entry->name_len = 1;
    dir_entry->file_type = EXT2_FT_DIR;
    strcpy(dir_entry->name, ".");
    // initialize '..'
    dir_entry = (void *)dir_entry + 12;
    dir_entry->inode = parent_inode;
    dir_entry->rec_len = 1012;
    dir_entry->name_len = 2;
    dir_entry->file_type = EXT2_FT_DIR;
    strcpy(dir_entry->name, "..");

    return 0;
}


// search for a new inode when we add the directory and update the inode_bitmap, and all related
int new_inode_search(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *bg, int* inode_bitmap) {
    int new_inode;
    int inode_update = 0;
    int i, m, n;
    for (i = 11; i < 32; i++) {
        // find the first inode=0 starting from #12 (11 in storage)
        if (inode_bitmap[i] == 0) {
            inode_bitmap[i] = 1;
            // update bg->bg_inode_bitmap
            for (m = 0; m < 4; m++) {
                for (n = 0; n < 8; n++) {
                    if ((8*m + n) == i) {
                        *(disk+1024+(bg->bg_inode_bitmap-1)*1024+m) |= (0x01 << n);
                        inode_update = 1;
                        break;
                    }
                }
                if (inode_update == 1) {
                    break;
                }
            }
        }
        if (inode_update == 1) {
            new_inode = i;
            break;
        }
    }
    // we need to update the block group descriptor and super block related to free_inodes_count
    bg->bg_free_inodes_count--;
    sb->s_free_inodes_count--;

    return (new_inode + 1);
}


// search for a new block when we add the directory and update the block_bitmap, and all related
int new_block_search(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *bg, int* block_bitmap) {
    int new_block;
    int block_update = 0;
    int i, m, n;
    for (i = 0; i < 128; i++) {
        // find the first inode=0 starting from #12 (11 in storage)
        if (block_bitmap[i] == 0) {
            block_bitmap[i] = 1;
            // update bg->bg_inode_bitmap
            for (m = 0; m < 16; m++) {
                for (n = 0; n < 8; n++) {
                    if ((8*m + n) == i) {
                        *(disk+1024+(bg->bg_block_bitmap-1)*1024+m) |= (0x01 << n);
                        block_update = 1;
                        break;
                    }
                }
                if (block_update == 1) {
                    break;
                }
            }
        }
        if (block_update == 1) {
            new_block = i;
            break;
        }
    }
    // we need to update the block group descriptor and super block related to free_block_count
    bg->bg_free_blocks_count--;
    sb->s_free_blocks_count--;

    return (new_block + 1);
}



