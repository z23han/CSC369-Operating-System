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

int *inode_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int *block_bitmap_setup(unsigned char *disk, struct ext2_group_desc *bg);
int count_num_dir(char path[]);
int resource_file_check(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt);
int link_file_check(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt);
char *link_file_name(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt);
int new_block_search(unsigned char *disk, int* block_bitmap);
int update_link_dir_entry(unsigned char *disk, int resource_inode, int parent_inode, char *file_name, int* block_bitmap);
int update_parent_inode_table(unsigned char *disk, int parent_inode);


int main(int argc, char **argv) {

	// there are 3 args for this command
    if(argc != 4) {
        fprintf(stderr, "ext2_ln <image name> <specified file on the disk> <linked file>\n");
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

    // initialize array inode_list storing directories and files
    int dir_inode_list[sb->s_inodes_count];
    // inode_cnt used for counting the total number of dir_inode and file_inode
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
    char resource_path[sizeof(argv[2])+1];
    strcpy(resource_path, argv[2]);
    // testing if the first char is '/'
    if (resource_path[0] != '/') {
        fprintf(stderr, "ENOENT\n");
        exit(1);
    }
    int resource_inode = resource_file_check(disk, resource_path, dir_inode_list, dir_inode_cnt);
    //printf("resource_inode: %d\n", resource_inode);

    char link_path[sizeof(argv[3])+1];
    strcpy(link_path, argv[3]);
    // testing if the first char is '/'
    if (link_path[0] != '/') {
    	fprintf(stderr, "ENOENT\n");
        exit(1);
    }
    int parent_inode = link_file_check(disk, link_path, dir_inode_list, dir_inode_cnt);
    
    char *file_name = link_file_name(disk, link_path, dir_inode_list, dir_inode_cnt);
    
    update_link_dir_entry(disk, resource_inode, parent_inode, file_name, block_bitmap);
    
    update_parent_inode_table(disk, parent_inode);



    free(file_name);
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


// check the resource file exists on the disk
int resource_file_check(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt) {

	//struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    struct ext2_dir_entry_2 *dir_entry;

	char *pch;
	pch = strtok(path, "/");

	int inode_search = 2;
	int file_flag = 0;
	int dir_flag = 1;
	int i, j, i_block_num, inode_block, sum_len, rec_len;

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
		pch = strtok(NULL, "/");
	}

	// the correct file name : pch
    // the file inode : inode_search, its block # : inode_block
    // we need to test if file_flag == 1 and dir_flag == 1
    if (file_flag == 1 && dir_flag == 1) {
    	return inode_search;
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


// test the link file validity, it should be a new file but located on an existing directory
// this is similar to mkdir checking, we need to return the parent inode
int link_file_check(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt) {

	//struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    struct ext2_dir_entry_2 *dir_entry;

    // create a link_path to hold the path
    char link_path[sizeof(path)/sizeof(char)+1];
    strcpy(link_path, path);
	// get the cnt for the following comparisons
	int dir_cnt = count_num_dir(link_path);

	// we need to do it again
	strcpy(link_path, path);
	char *pch;
	pch = strtok(link_path, "/");

	int inode_search = 2;
	int file_flag = 0;
	int dir_flag = 0;
	int i, j, i_block_num, inode_block, sum_len, rec_len;
	// count dir layers, and it should be dir_cnt-1 if it is a correct mkdir
    int dir_cnt_2 = 1;

	while (1) {
		// we shouldn't reach the end, otherwise we have all the directories
		if (pch == NULL) {
			fprintf(stderr, "EEXIST\n");
            exit(1);
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
					// test if it is a file after directory
					if (((inode+inode_search-1)->i_mode & EXT2_S_IFREG) && (dir_flag == 1)) {
						fprintf(stderr, "ENOENT\n");
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
		pch = strtok(NULL, "/");
	}

	// for error debugging
	assert(dir_cnt_2 == (dir_cnt - 1));

	return inode_search;
}


// get the name of the new file
// this is the same as link_file_check, but return the name
char *link_file_name(unsigned char *disk, char path[], int dir_inode_list[], int dir_inode_cnt) {

	//struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    struct ext2_dir_entry_2 *dir_entry;

    // create a link_path to hold the path
    char link_path[sizeof(path)/sizeof(char)+1];
    strcpy(link_path, path);
	// get the cnt for the following comparisons
	int dir_cnt = count_num_dir(link_path);

	// we need to do it again
	strcpy(link_path, path);
	char *pch;
	pch = strtok(link_path, "/");

	int inode_search = 2;
	int file_flag = 0;
	int dir_flag = 1;
	int i, j, i_block_num, inode_block, sum_len, rec_len;
	// count dir layers, and it should be dir_cnt-1 if it is a correct mkdir
    int dir_cnt_2 = 1;

	while (1) {
		// we shouldn't reach the end, otherwise we have all the directories
		if (pch == NULL) {
			fprintf(stderr, "EEXIST\n");
            exit(1);
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
					// test if it is a file after directory
					if (((inode+inode_search-1)->i_mode & EXT2_S_IFREG) && (dir_flag == 1)) {
						fprintf(stderr, "ENOENT\n");
                        exit(1);
					}
					// if it is only a directory, we jump as well
					if (dir_flag == 1) {
						break;
					}
				}
				// this means the directory or file doesn't exist on the disk
				if (dir_flag == 0) {
					// check if we have reach the position before the new file
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
		pch = strtok(NULL, "/");
	}

	// for error debugging
	assert(dir_cnt_2 == (dir_cnt - 1));

	char *file_name = (char *)malloc(sizeof(char) * strlen(pch));
	strcpy(file_name, pch);
	return file_name;
}


// update the directory holding the new file with the inode of the resource file 
// return the directory block
int update_link_dir_entry(unsigned char *disk, int resource_inode, int parent_inode, char *file_name, int* block_bitmap) {
	
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
    struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);

    // we need first get the block of the parent directory
    int parent_block = (inode + parent_inode - 1)->i_block[0];
    // go to the dir_entry 
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
    int new_name_len = strlen(file_name);
    int new_rec_len;
    if (new_name_len % 4 == 0) {
        new_rec_len = new_name_len + 8;
    } else {
        new_rec_len = 4 * (new_name_len / 4 + 1) + 8;
    }

    // justify if we can add the new directory in this block or apply for a new block based on rec_len
    if (rest_rec_len >= new_rec_len) {
    	dir_entry->rec_len = last_rec_len;
        dir_entry = (void *)dir_entry + last_rec_len;
        // update all the information
        dir_entry->inode = resource_inode;
        dir_entry->rec_len = rest_rec_len;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_REG_FILE;
        strcpy(dir_entry->name, file_name);

        return parent_block;
    }
    // otherwise allocate a new block
    else {
    	int new_block = new_block_search(disk, block_bitmap);
    	(inode + parent_block - 1)->i_blocks += 2;
    	(inode + parent_block - 1)->i_block[1] = new_block;
    	dir_entry = (struct ext2_dir_entry_2 *)(disk + 1024 * new_block);
    	// create a new inode to hold the new directory block
        dir_entry->inode = resource_inode;
        dir_entry->rec_len = 1024;
        dir_entry->name_len = new_name_len;
        dir_entry->file_type = EXT2_FT_REG_FILE;
        strcpy(dir_entry->name, file_name);

        return new_block;
    }

}


// update inode_table links of the parent inode link
int update_parent_inode_table(unsigned char *disk, int parent_inode) {

	struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024 + 1024);
	struct ext2_inode *inode = (struct ext2_inode *)(disk + bg->bg_inode_table*1024);
	(inode + parent_inode - 1)->i_links_count ++;
	return 0;
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
                        //*(disk+1024+(bg->bg_inode_bitmap-1)*1024+i)>>j & 1;
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

