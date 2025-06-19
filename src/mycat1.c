#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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
    
    // 逐字符读取并输出
    char c;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, &c, 1)) > 0) {
        if (write(STDOUT_FILENO, &c, 1) == -1) {
            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
            close(fd);
            exit(1);
        }
    }
    
    // 检查读取错误
    if (bytes_read == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }
    
    // 关闭文件
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        exit(1);
    }
    
    return 0;
} 