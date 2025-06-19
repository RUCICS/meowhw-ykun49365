#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

// 获取IO块大小（内存页面大小）
size_t io_blocksize() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        fprintf(stderr, "Warning: Could not get page size, using 4KB\n");
        return 4096;
    }
    return (size_t)page_size;
}

// 分配页面对齐的内存
char* align_alloc(size_t size) {
    size_t page_size = io_blocksize();
    
    // 分配额外的内存以保证对齐
    size_t total_size = size + page_size - 1;
    char* raw_ptr = malloc(total_size + sizeof(void*));
    if (!raw_ptr) {
        return NULL;
    }
    
    // 计算对齐的地址
    char* aligned_ptr = (char*)(((uintptr_t)raw_ptr + sizeof(void*) + page_size - 1) & 
                                ~(page_size - 1));
    
    // 在对齐地址前存储原始指针以便释放
    *((void**)(aligned_ptr - sizeof(void*))) = raw_ptr;
    
    return aligned_ptr;
}

// 释放页面对齐的内存
void align_free(void* ptr) {
    if (ptr) {
        // 获取原始指针并释放
        void* raw_ptr = *((void**)((char*)ptr - sizeof(void*)));
        free(raw_ptr);
    }
}

int main(int argc, char *argv[]) {
    // 检查命令行参数
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    
    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }
    
    // 获取缓冲区大小
    size_t buffer_size = io_blocksize();
    
    // 分配对齐的缓冲区
    char *buffer = align_alloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error allocating aligned buffer: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }
    
    // 使用缓冲区读取并输出
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
    
    // 检查读取错误
    if (bytes_read == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        align_free(buffer);
        close(fd);
        exit(1);
    }
    
    // 清理
    align_free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        exit(1);
    }
    
    return 0;
} 