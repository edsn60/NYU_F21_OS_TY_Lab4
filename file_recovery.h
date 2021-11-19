//
// Created by ysw on 11/17/21.
//

#ifndef NYUFILE_FILE_RECOVERY_H
#define NYUFILE_FILE_RECOVERY_H

void recover_file_with_name(char* filename);
void recover_file_with_sha1(char *filename, char* sha1);
void recover_non_contiguous_file_with_sha1(char* filename, char *sha1);
#endif //NYUFILE_FILE_RECOVERY_H
