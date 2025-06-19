#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

// 基于实验结果的优化缓冲区大小
#define OPTIMAL_BUFFER_SIZE 65536

// 获取IO块大小（考虑系统调用开销）
size_t io_blocksize(int fd) {
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    if (page_size == 0) {
        page_size = 4096;
    }
    
    struct stat st;
    size_t fs_block_size = page_size;
    
    if (fstat(fd, &st) == 0 && st.st_blksize > 0) {
        fs_block_size = (size_t)st.st_blksize;
    }
    
    // 计算基础块大小（页面大小和文件系统块大小的较大者）
    size_t base_size = (fs_block_size > page_size) ? fs_block_size : page_size;
    
    // 根据实验结果，使用固定的最优大小，但确保它是基础块大小的倍数
    size_t optimal_size = OPTIMAL_BUFFER_SIZE;
    
    // 确保最优大小是基础块大小的倍数
    if (optimal_size < base_size) {
        optimal_size = base_size;
    } else {
        // 向上取整到base_size的倍数
        optimal_size = ((optimal_size + base_size - 1) / base_size) * base_size;
    }
    
    return optimal_size;
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