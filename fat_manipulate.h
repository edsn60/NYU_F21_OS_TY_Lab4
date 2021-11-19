//
// Created by ysw on 11/17/21.
//

#ifndef NYUFILE_FAT_MANIPULATE_H
#define NYUFILE_FAT_MANIPULATE_H

unsigned int *get_fat_start_addr();
void recover_fats(unsigned int *clusters, unsigned int cluster_num);

#endif //NYUFILE_FAT_MANIPULATE_H
