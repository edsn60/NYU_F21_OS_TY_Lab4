#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "fsinfo.h"
#include "file_recovery.h"

#define USAGE_MESSAGE "Usage: ./nyufile disk <options>\n-i                     Print the file system information.\n-l                     List the root directory.\n-r filename [-s sha1]  Recover a contiguous file.\n-R filename -s sha1    Recover a possibly non-contiguous file.\n"


void *fs_image;
BootEntry *boot_entry;
size_t image_size;


void print_usage_message(){
    fprintf(stderr, USAGE_MESSAGE);
    exit(-1);
}


void map_fs_image(char *disk_image_name){
    struct stat st;
    int fd = open(disk_image_name, O_RDWR);
    if (fd == -1){
        fprintf(stderr, "Error: %s, no such file\n", disk_image_name);
    }
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Error: fstat error\n");
        exit(-1);
    }
    image_size = st.st_size;
    fs_image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    boot_entry = (BootEntry*)fs_image;
    if (fs_image == MAP_FAILED){
        fprintf(stderr, "Error, mmap failed: %d: %s\n", errno, strerror(errno));
        abort();
    }
}


int main(int argc, char *const argv[]) {
    if (argc < 3 || argc > 6) {
        print_usage_message();
    }
    unsigned char arg_mask = 0b00000;

    char *filename;
    char *sha1;

    int status;

    while ((status = getopt(argc, argv, ":ilr:R:s:")) != -1){
        switch (status) {
            case 105:   // i
                if ((arg_mask | 0b00001) == arg_mask){
                    print_usage_message();
                    exit(-1);
                }
                arg_mask |= 0b00001;
                break;
            case 108:   // l
                if ((arg_mask | 0b00010) == arg_mask){
                    print_usage_message();
                    exit(-1);
                }
                arg_mask |= 0b00010;
                break;
            case 114:   // r
                if ((arg_mask | 0b00100) == arg_mask){
                    print_usage_message();
                    exit(-1);
                }
                filename = optarg;
                arg_mask |= 0b00100;
                break;
            case 82:    // R
                if ((arg_mask | 0b01000) == arg_mask){
                    print_usage_message();
                    exit(-1);
                }
                filename = optarg;
                arg_mask |= 0b01000;
                break;
            case 115:   // s
                if ((arg_mask | 0b10000) == arg_mask){
                    print_usage_message();
                    exit(-1);
                }
                sha1 = optarg;
                arg_mask |= 0b10000;
                break;
            default:
                print_usage_message();
        }
    }
    if (!argv[optind]){
        print_usage_message();
    }
    char *disk_image_name = argv[optind];
    map_fs_image(disk_image_name);

    switch (arg_mask) {
        case 0b00001:   // -i
            print_fs_info();
            break;
        case 0b00010:   // -l
            list_root_dir();
            break;
        case 0b00100:   // -r
            recover_file_with_name(filename);
            break;
        case 0b10100:   // -r -s
            recover_file_with_sha1(filename, sha1);
            break;
        case 0b11000:   // -R -s
            recover_non_contiguous_file_with_sha1(filename, sha1);
            break;
        default:
            print_usage_message();
    }
}