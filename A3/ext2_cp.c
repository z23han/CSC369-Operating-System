#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/limits.h>
#include <libgen.h>
#include <assert.h>
#include "ext2.h"


unsigned char *disk;

/*
 * A struct holds information of a file or link
*/ 
struct file_info {
    int name_mode;      /* justify if the file has a new or not */
    int file_inode;
    int parent_inode;
    int *file_blocks;
    int block_cnt;
    int file_size;
    char file_name[1024];
};

enum {
    HAS_NAME = 1,
    NO_NAME = 0
};

int *inode_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int *block_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int count_num_dir(char path[]);
int new_inode_search(unsigned char *disk, int* inode_bitmap);
int new_block_search(unsigned char *disk, int* block_bitmap);
struct file_info *check_file_valid(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt);
int allocate_file_blocks(unsigned char *disk, int* block_bitmap, struct file_info *file_inode_info, int file_num_blocks);
int update_inode_table(unsigned char *disk, struct file_info *file_inode_info);
int update_dir_entry(unsigned char *disk, struct file_info *file_inode_info, int* block_bitmap);


int main(int argc, char **argv) {

    // there are 3 args for this command
    if(argc != 4) {
        fprintf(stderr, "ext2_cp <image name> <path to the file on native OS> <absolute path on the disk>\n");
        exit(1);
    }

    // disk image
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
    int i;

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
    
    // get the file_size and file_name from argv[2]
    struct stat statbuf;

    if (stat(argv[2], &statbuf) == -1) {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }
    // file size from local FS
    long file_size = (long) statbuf.st_size;
    // calculate num of blocks to hold the new file
    int file_num_blocks;
    if (file_size % 1024 == 0) {
        file_num_blocks = file_size / 1024;
    } else {
        file_num_blocks = file_size / 1024 + 1;
    }
    

    char *basec = strdup(argv[2]);
    // file name from local FS
    char *file_name = (basename(basec));
    

    // get the argv[3] information
    char disk_file_path[sizeof(argv[3])+1];
    strcpy(disk_file_path, argv[3]);
    // test if the first char is '/'
    if (disk_file_path[0] != '/') {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }

    // I need to detect if I'm given a path or file on the disk, and its validity
    struct file_info *file_inode_info = check_file_valid(disk, disk_file_path, dir_inode_list, dir_inode_cnt);

    if (file_inode_info->name_mode == NO_NAME) {
        strcpy(file_inode_info->file_name, file_name);
    }
    file_inode_info->file_size = file_size;

    // I've got the name, but I'm left with file_inode and file_block

    // find a new inode and update inode_bitmap in group descriptor
    int file_inode = new_inode_search(disk, inode_bitmap);
    file_inode_info->file_inode = file_inode;
    
    // find a new block or several new block to hold the new file
    allocate_file_blocks(disk, block_bitmap, file_inode_info, file_num_blocks);
    // now file_inode_info has been updated with necessary information

    // I've done all updating the bitmap, then I need to update inode_table and dir_entry
    update_inode_table(disk, file_inode_info);
    update_dir_entry(disk, file_inode_info, block_bitmap);


    free(file_inode_info->file_blocks);
    free(file_inode_info);
    free(inode_bitmap);
    free(block_bitmap);
    free(basec);

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


// count the number of directory, here directory can include the last file
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

// check file or directory. 
// Íf it is a file, the name should be new; if it is a directory, it should exists, but we give the file name later
struct file_info *check_file_valid(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt) {

    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    struct ext2_dir_entry_2 *dir_entry;

    // create a cp_path to hold the path
    char cp_path[sizeof(path)/sizeof(char)+1];
    strcpy(cp_path, path);
    // get the cnt number of the path
    int dir_cnt = count_num_dir(cp_path);

    // we need to do it again
    strcpy(cp_path, path);
    char *pch;
    pch = strtok(cp_path, "/");


    int inode_search = 2;
    int file_flag = 0;
    int dir_flag = 1;
    int i, j, i_block_num, inode_block, sum_len, rec_len;
    // we also need to check the layers, since we don't know if it is a directory or an unknown file
    int dir_cnt_2 = 1;

    while (1) {
        // if we have reached the end, we should break
        if (pch == NULL) {
            break;
        }
        // otherwise, we justify the inode
        // we cannot get the file/xxx option
        if (file_flag == 1) {
            fprintf(stderr, "ENOENT\n");
            exit(1);
        }
        // set the dir_flag = 0
        dir_flag = 0;
        // search through all the inode_list
        for (i = 0; i < dir_inode_cnt; i++) {
            // if we find the inode_search
            if (dir_inode_list[i] == inode_search) {
                i_block_num = (inode + inode_search - 1)->i_blocks / 2;
                for (j = 0; j < i_block_num; j++) {
                    inode_block = (inode + inode_search - 1)->i_block[j];
                    dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * inode_block);
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
                        }
                        else {
                            sum_len += rec_len;
                        }
                    }
                    // test if it is a file after directory, but we're not allowed to get the known file
                    if (((inode+inode_search-1)->i_mode & EXT2_S_IFREG) && (dir_flag == 1)) {
                        fprintf(stderr, "EEXIST\n");
                        exit(1);
                    }
                    // if it is only a directory, we jump as well
                    if (dir_flag == 1) {
                        break;
                    }
                }
                // this means the directory or file doesn't exist on the disk
                if (dir_flag == 0) {
                    // check if we have reached the position before the new file
                    if (dir_cnt_2 == (dir_cnt - 1)) {
                        break;
                    } 
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
        // here we search for it but still couldn't find the inode_search, return error
        if (dir_flag == 0) {
            if (dir_cnt_2 == (dir_cnt - 1)) {
                break;
            }
            else {
                fprintf(stderr, "ENOENT\n");
                exit(1);
            }
        }
        //previous_pch = pch;
        pch = strtok(NULL, "/");
    }

    // malloc a new struct holding file inode information
    struct file_info *file_inode_info = (struct file_info *)malloc(sizeof(struct file_info));


    // if we just have only the directory, we have dir_cnt_2 == dir_cnt
    if (dir_cnt_2 == dir_cnt) {
        assert(dir_flag == 1);
        assert(file_flag == 0);
        file_inode_info->name_mode = NO_NAME;
        file_inode_info->parent_inode = inode_search;
    }
    // else if we have a new file name already defined, we have this condition
    else if (dir_cnt_2 == (dir_cnt - 1)) {
        assert(dir_flag == 0);
        assert(file_flag == 0);
        file_inode_info->name_mode = HAS_NAME;
        file_inode_info->parent_inode = inode_search;
        strcpy(file_inode_info->file_name, pch);
    }
    // otherwise we return error
    else {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }

    return file_inode_info;

}


// find a new block or several new block to hold the new file
int allocate_file_blocks(unsigned char *disk, int* block_bitmap, struct file_info *file_inode_info, int file_num_blocks) {
    
    int i;
    // we check whether we need indirect block to hold the new file data size, compare 12 with file_num_blocks
    // when we don't need singly-indirect block
    if (file_num_blocks <= 12) {
        file_inode_info->file_blocks = (int *)malloc(sizeof(int) * file_num_blocks);
        for (i = 0; i < file_num_blocks; i++) {
            *(file_inode_info->file_blocks + i) = new_block_search(disk, block_bitmap);
            //printf("%d ", *(new_block_list + i));
        }
        file_inode_info->block_cnt = file_num_blocks;
    }
    // otherwise we need (file_num_blocks + 1) blocks, and the #12 holds singly-indirect block pointer
    else {
        file_inode_info->file_blocks = (int *)malloc(sizeof(int) * (file_num_blocks + 1));
        for (i = 0; i < 12; i++) {
            *(file_inode_info->file_blocks + i) = new_block_search(disk, block_bitmap);
        }
        // #12 holds the information of indirected block
        int singly_indirect = new_block_search(disk, block_bitmap);
        *(file_inode_info->file_blocks + 12) = singly_indirect;
        // first I need to set all the 1024 bytes (128x8) in singly_indirect block to be 0
        for (i = 0; i < 128; i++) {
            *(disk + singly_indirect * 1024 + i) = 0x00;
        }
        // then simply set the first (file_num_blocks+1)-13 blocks, by checking available block
        int directed_block;
        for (i = 0; i < (file_num_blocks-12); i++) {
            directed_block = new_block_search(disk, block_bitmap);
            *(disk + singly_indirect * 1024 + i) = directed_block;
            *(file_inode_info->file_blocks + 13 + i) = directed_block;
        }
        file_inode_info->block_cnt = file_num_blocks + 1;
    }


    return 0;
}


// update inode_table
int update_inode_table(unsigned char *disk, struct file_info *file_inode_info) {
    
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    int file_inode = file_inode_info->file_inode;
    (inode + file_inode - 1)->i_mode = EXT2_S_IFREG;
    (inode + file_inode - 1)->i_size = file_inode_info->file_size;
    (inode + file_inode - 1)->i_links_count = 1;
    (inode + file_inode - 1)->i_blocks = file_inode_info->block_cnt * 2;
    int i;
    for (i = 0; i < file_inode_info->block_cnt; i++) {
        (inode + file_inode - 1)->i_block[i] = file_inode_info->file_blocks[i];
    }

    return 0;
}


// update directory entry
int update_dir_entry(unsigned char *disk, struct file_info *file_inode_info, int* block_bitmap) {

    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);
    // go to the block entry
    int parent_inode = file_inode_info->parent_inode;
    int parent_block = (inode + parent_inode - 1)->i_block[0];
    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * parent_block);

    int sum_len = 0;
    int rec_len = 0;
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

    // calculate the new file information
    int new_name_len = (int)strlen(file_inode_info->file_name);
    int new_rec_len;
    if (new_name_len % 4 == 0) {
        new_rec_len = new_name_len + 8;
    } else {
        new_rec_len = 4 * (new_name_len / 4 + 1) + 8;
    }

    // justify if we can add the new file in this entry or apply for a new block based on rec_len
    if (rest_rec_len >= new_rec_len) {
        dir_entry->rec_len = last_rec_len;
        dir_entry = (void *)dir_entry + last_rec_len;
        // update all the information
        dir_entry->inode = file_inode_info->file_inode;
        dir_entry->rec_len = rest_rec_len;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_REG_FILE;
        strcpy(dir_entry->name, file_inode_info->file_name);
    }
    // else we need to allocate a new block
    else {
        int new_block = new_block_search(disk, block_bitmap);
        (inode + parent_inode - 1)->i_blocks += 2;
        int i_block_num = (inode + parent_inode - 1)->i_blocks / 2;
        (inode + parent_inode - 1)->i_block[i_block_num-1] = new_block;
        dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * new_block);
        dir_entry->inode = file_inode_info->file_inode;
        dir_entry->rec_len = 1024;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_REG_FILE;
        strcpy(dir_entry->name, file_inode_info->file_name);
    }


    return 0;
}



// search for a new inode when we add the directory and update the inode_bitmap, and all related
int new_inode_search(unsigned char *disk, int* inode_bitmap) {

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);

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
int new_block_search(unsigned char *disk, int* block_bitmap) {

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);

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

