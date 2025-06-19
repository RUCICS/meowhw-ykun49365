#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// 获取IO块大小（内存页面大小）
size_t io_blocksize() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        // 如果获取页面大小失败，使用默认的4KB
        fprintf(stderr, "Warning: Could not get page size, using 4KB\n");
        return 4096;
    }
    return (size_t)page_size;
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
    
    // 动态分配缓冲区
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error allocating buffer: %s\n", strerror(errno));
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
                free(buffer);
                close(fd);
                exit(1);
            }
            bytes_written += result;
        }
    }
    
    // 检查读取错误
    if (bytes_read == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        free(buffer);
        close(fd);
        exit(1);
    }
    
    // 清理
    free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        exit(1);
    }
    
    return 0;
} 