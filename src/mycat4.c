#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

// 获取IO块大小（考虑内存页大小和文件系统块大小）
size_t io_blocksize(int fd) {
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    if (page_size == 0) {
        page_size = 4096; // 默认页面大小
    }
    
    struct stat st;
    if (fstat(fd, &st) == 0) {
        size_t fs_block_size = (size_t)st.st_blksize;
        
        // 检查文件系统块大小是否合理（是2的幂）
        if (fs_block_size > 0 && (fs_block_size & (fs_block_size - 1)) == 0) {
            // 选择页面大小和文件系统块大小的最大值
            size_t block_size = (fs_block_size > page_size) ? fs_block_size : page_size;
            
            // 但不要太大，限制在64KB以内
            if (block_size <= 65536) {
                return block_size;
            }
        }
    }
    
    // 如果无法获取或文件系统块大小不合理，使用页面大小
    return page_size;
}

// 分配页面对齐的内存
char* align_alloc(size_t size) {
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    if (page_size == 0) page_size = 4096;
    
    size_t total_size = size + page_size - 1;
    char* raw_ptr = malloc(total_size + sizeof(void*));
    if (!raw_ptr) {
        return NULL;
    }
    
    char* aligned_ptr = (char*)(((uintptr_t)raw_ptr + sizeof(void*) + page_size - 1) & 
                                ~(page_size - 1));
    
    *((void**)(aligned_ptr - sizeof(void*))) = raw_ptr;
    
    return aligned_ptr;
}

// 释放页面对齐的内存
void align_free(void* ptr) {
    if (ptr) {
        void* raw_ptr = *((void**)((char*)ptr - sizeof(void*)));
        free(raw_ptr);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }
    
    // 获取优化的缓冲区大小
    size_t buffer_size = io_blocksize(fd);
    
    char *buffer = align_alloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error allocating aligned buffer: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }
    
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t result = write(STDOUT_FILENO, buffer + bytes_written, 
                                   bytes_read - bytes_written);
            if (result == -1) {
                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                align_free(buffer);
                close(fd);
                exit(1);
            }
            bytes_written += result;
        }
    }
    
    if (bytes_read == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        align_free(buffer);
        close(fd);
        exit(1);
    }
    
    align_free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        exit(1);
    }
    
    return 0;
} 