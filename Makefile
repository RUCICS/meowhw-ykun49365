CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
SRCDIR = src
TARGETDIR = target

# 所有的mycat版本
TARGETS = $(TARGETDIR)/mycat1 $(TARGETDIR)/mycat2 $(TARGETDIR)/mycat3 \
          $(TARGETDIR)/mycat4 $(TARGETDIR)/mycat5 $(TARGETDIR)/mycat6

.PHONY: all clean test-setup run-experiments

all: $(TARGETS)

# 创建目标目录
$(TARGETDIR):
	mkdir -p $(TARGETDIR)

# 编译各个版本
$(TARGETDIR)/mycat1: $(SRCDIR)/mycat1.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGETDIR)/mycat2: $(SRCDIR)/mycat2.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGETDIR)/mycat3: $(SRCDIR)/mycat3.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGETDIR)/mycat4: $(SRCDIR)/mycat4.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGETDIR)/mycat5: $(SRCDIR)/mycat5.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGETDIR)/mycat6: $(SRCDIR)/mycat6.c | $(TARGETDIR)
	$(CC) $(CFLAGS) -D_GNU_SOURCE -o $@ $<

# 测试数据准备
test-setup:
	@echo "生成测试文件..."
	@python3 -c "import random; random.seed(42); \
	    [open('test.txt', 'wb').write(random.randbytes(1024*1024)) for _ in range(2048)]"
	@echo "测试文件生成完成: test.txt (2GB)"

# 运行缓冲区大小实验
run-experiments:
	@echo "运行缓冲区大小实验..."
	@chmod +x $(SRCDIR)/buffer_size_experiment.py
	@python3 $(SRCDIR)/buffer_size_experiment.py

# 清理
clean:
	rm -rf $(TARGETDIR)
	rm -f test.txt buffer_experiment_results.txt

# 帮助信息
help:
	@echo "可用的目标:"
	@echo "  all          - 编译所有mycat版本"
	@echo "  test-setup   - 生成测试文件"
	@echo "  run-experiments - 运行缓冲区大小实验"
	@echo "  clean        - 清理生成的文件"
	@echo "  help         - 显示此帮助信息" 