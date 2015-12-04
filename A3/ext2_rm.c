#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/limits.h>
#include <assert.h>
#include "ext2.h"

unsigned char *disk;

/*
 * A struct holds information of a file or link
*/ 
struct file_info {
	int file_inode;
	int parent_inode;
	int file_block;
	char file_name[1024];
};


int *inode_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int *block_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
struct file_info *check_file_valid(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt);
int dir_entry_update(unsigned char *disk, struct file_info *file_inode_info);
int group_descriptor_update(unsigned char *disk, struct file_info *file_inode_info);


int main(int argc, char **argv) {

	// there are 2 args for this command
    if(argc != 3) {
        fprintf(stderr, "ext2_rm <image name> <absolute path to a file or link>\n");
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

    // get the path and store it into an char array
    char file_path[sizeof(argv[2])+1];
    strcpy(file_path, argv[2]);
    // testing if the first char is '/'
    if (file_path[0] != '/') {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }

    // I need to check the file or link is valid and exists on the disk, and get necessary information
    struct file_info *file_inode_info = check_file_valid(disk, file_path, dir_inode_list, dir_inode_cnt);
    // I need to go to dir_entry block and delete the file information
    dir_entry_update(disk, file_inode_info);
    // I need to go to group descriptor and update information(inode_bitmap, block_bitmap, free blocks, etc.)
    group_descriptor_update(disk, file_inode_info);
    


    free(file_inode_info);
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



struct file_info *check_file_valid(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt) {

	//struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    struct ext2_dir_entry_2 *dir_entry;

    char *pch;
	pch = strtok(path, "/");

	int inode_search = 2;
	int parent_inode = 2;
	int file_flag = 0;
	int dir_flag = 1;
	int i, j, i_block_num, inode_block, sum_len, rec_len;
	char *previous_pch = pch;

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
							parent_inode = inode_search;
							inode_search = dir_entry->inode;
							dir_flag = 1;
							break;
						}
						else {
							sum_len += rec_len;
						}
					}
					// test if it is a file after directory
					if (((inode+inode_search-1)->i_mode & EXT2_S_IFREG) && (dir_flag == 1)) {
						file_flag = 1;
						break;
					}
					// if it is only a directory, we jump as well
					if (dir_flag == 1) {
						break;
					}
				}
				// this means the directory or file doesn't exist on the disk
				if (dir_flag == 0) {
					fprintf(stderr, "ENOENT\n");
					exit(1);
				}
			}
			if (dir_flag == 1) {
				break;
			}
		}
		// here we search for it but still couldn't find the inode_search, return error
		if (dir_flag == 0) {
			fprintf(stderr, "ENOENT\n");
			exit(1);
		}
		previous_pch = pch;
		pch = strtok(NULL, "/");
	}

	// the correct file name : previous_pch
    // the file inode : inode_search, its block # : inode_block
    // we need to test if file_flag == 1 and dir_flag == 1
    if (file_flag == 1 && dir_flag == 1) {
    	struct file_info *file_inode_info = (struct file_info *)malloc(sizeof(struct file_info));
    	file_inode_info->file_inode = inode_search;
    	file_inode_info->parent_inode = parent_inode;
    	file_inode_info->file_block = inode_block;
    	strcpy(file_inode_info->file_name, previous_pch);
    	return file_inode_info;
    } 
    // else if we only get dir_flag == 1 but file_flag == 0
    else if (file_flag == 0 && dir_flag == 1) {
    	fprintf(stderr, "EISDIR\n");
    	exit(1);
    } 
    else {
    	fprintf(stderr, "ENOENT\n");
    	exit(1);
    }
}


int dir_entry_update(unsigned char *disk, struct file_info *file_inode_info) {

	//struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    int parent_inode = file_inode_info->parent_inode;
    int parent_block = (inode + parent_inode - 1)->i_block[0];
    // we need a LinkedList sorta implementation to achieve deletion of one node
    // go to dir_entry
    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * parent_block);
    struct ext2_dir_entry_2 *dir_entry_update;
    int file_inode = file_inode_info->file_inode;
    int file_found = 0;
    int sum_len = 0;
    int rec_len = 0;
    int prev_len = 0;
    int file_found_flag = 0;
    int file_rec_len = 0;
    int has_updated = 0;
    while (sum_len < 1024) {
    	prev_len = dir_entry->rec_len;
    	dir_entry = (void *)dir_entry + rec_len;
    	// if we find it
    	if (dir_entry->inode == file_inode) {
    		// record the rec_len
    		file_rec_len = dir_entry->rec_len;
    		file_found = 1;
    		// we can check if we have reached the end of the parent directory, we should update here
    		// this situation is different from that file is located in the middle of dir_entry
    		if ((sum_len += file_rec_len) == 1024) {
    			dir_entry_update = (struct ext2_dir_entry_2 *)((void *)dir_entry - prev_len);
    			dir_entry_update->rec_len = dir_entry_update->rec_len + file_rec_len;
    			has_updated = 1;
    			break;
    		}
    	}
    	// if we have found the file, we need to move the following pointers ahead of file_rec_len
    	if (file_found_flag == 1) {
    		// we move back and set dir_entry_update 
    		dir_entry_update = (struct ext2_dir_entry_2 *)((void *)dir_entry - file_rec_len);
    		dir_entry_update->inode = dir_entry->inode;
    		dir_entry_update->rec_len = dir_entry->rec_len;
    		dir_entry_update->name_len = dir_entry->name_len;
    		dir_entry_update->file_type = dir_entry->file_type;
    		strcpy(dir_entry_update->name, dir_entry->name);
    	}
		rec_len = dir_entry->rec_len;
    	sum_len += rec_len;
    	if (file_found == 1 && file_found_flag == 0) {
    		file_found_flag = 1;
    	}
    }
    // if we didn't find it, return error
    if (file_found == 0) {
    	fprintf(stderr, "error on file\n");
    	exit(1);
    }
    if (has_updated == 0) {
    	// we reach to the end of dir_entry, but we need to move back to file_rec_len and update
	    dir_entry_update = (struct ext2_dir_entry_2 *)((void *)dir_entry - file_rec_len);
	    dir_entry_update->inode = dir_entry->inode;
		dir_entry_update->rec_len = dir_entry->rec_len + file_rec_len;
		dir_entry_update->name_len = dir_entry->name_len;
		dir_entry_update->file_type = dir_entry->file_type;
		strcpy(dir_entry_update->name, dir_entry->name);
    }

    return 0;
}


int group_descriptor_update(unsigned char *disk, struct file_info *file_inode_info) {

	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    int file_inode = file_inode_info->file_inode;
    int file_block_cnt = (inode + file_inode - 1)->i_blocks / 2;
    // allocate an array holding all the used blocks related to this file
    int block_num[file_block_cnt];
    int i, j;
    // if it is smaller or equal to 12, there's no indirect block
    if (file_block_cnt <= 12) {
    	for (i = 0; i < file_block_cnt; i++) {
    		block_num[i] = (inode + file_inode - 1)->i_block[i];
    	}
    } 
    // otherwise we have to deal with singly-indirect block
    else {
    	for (i = 0; i < 12; i++) {
    		block_num[i] = (inode + file_inode - 1)->i_block[i];
    	}
    	// singly-indirect pointer block
    	block_num[12] = (inode + file_inode - 1)->i_block[12];
    	int singly_indirected = 13;
    	for (i = 0; i < 16; i++) {
    		if (*(disk + block_num[12] * 1024 + i) != 0) {
    			block_num[singly_indirected] = *(disk + block_num[12] * 1024 + i);
    			singly_indirected ++;
    		}
    	}
    }
    
    // update group descriptor
    // inode_bitmap
    for (i = 0; i < 4; i++) {
    	for (j = 0; j < 8; j++) {
    		if ((8*i + j) == (file_inode - 1)) {
    			*(disk+1024+(bg->bg_inode_bitmap-1)*1024+i) &= ~(0x01 << j);
    		}
    	}
    }
    // block_bitmap
    int m;
    for (m = 0; m < file_block_cnt; m++) {
    	for (i = 0; i < 16; i++) {
	    	for (j = 0; j < 8; j++) {
	    		if ((8*i + j) == block_num[m] - 1) {
	    			*(disk+1024+(bg->bg_block_bitmap-1)*1024+i) &= ~(0x01 << j);
	    		}
	    	}
	    }
    }

    // update free_inodes_count and free_blocks_count
    bg->bg_free_inodes_count ++;
    sb->s_free_inodes_count ++;
    for (m = 0; m < file_block_cnt; m++) {
    	bg->bg_free_blocks_count ++;
    	sb->s_free_blocks_count ++;
    }
    

    return 0;
    
    
}

