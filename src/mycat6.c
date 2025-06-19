#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

// 在Linux上需要定义_GNU_SOURCE来使用fadvise
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
    
    size_t base_size = (fs_block_size > page_size) ? fs_block_size : page_size;
    size_t optimal_size = OPTIMAL_BUFFER_SIZE;
    
    if (optimal_size < base_size) {
        optimal_size = base_size;
    } else {
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
    
    // 使用fadvise优化文件访问
    // POSIX_FADV_SEQUENTIAL: 告诉内核我们将顺序读取文件
    // POSIX_FADV_WILLNEED: 告诉内核我们将需要这些数据，可以预读
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        // fadvise失败不是致命错误，只是性能可能不是最优
        fprintf(stderr, "Warning: fadvise SEQUENTIAL failed\n");
    }
    
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED) != 0) {
        fprintf(stderr, "Warning: fadvise WILLNEED failed\n");
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
    off_t offset = 0;
    
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
        
        // 告诉内核我们已经不再需要刚刚读取的数据了
        // 这可以帮助内核更好地管理缓存
        if (posix_fadvise(fd, offset, bytes_read, POSIX_FADV_DONTNEED) != 0) {
            // 不是致命错误
        }
        
        offset += bytes_read;
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