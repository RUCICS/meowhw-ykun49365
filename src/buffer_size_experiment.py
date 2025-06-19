#!/usr/bin/env python3
"""
缓冲区大小性能测试实验脚本
"""

import subprocess
import time
import sys
import os

def run_dd_test(buffer_size_kb, data_size_mb=2048):
    """
    使用dd命令测试指定缓冲区大小的性能
    """
    try:
        # 将缓冲区大小转换为字节
        bs = f"{buffer_size_kb}k"
        count = data_size_mb * 1024 // buffer_size_kb
        
        # 运行dd命令，从/dev/zero读取，写入/dev/null
        cmd = ["dd", f"if=/dev/zero", f"of=/dev/null", f"bs={bs}", f"count={count}"]
        
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True)
        end_time = time.time()
        
        duration = end_time - start_time
        
        if result.returncode == 0:
            # 从stderr中提取速度信息
            stderr_output = result.stderr
            if "MB/s" in stderr_output or "GB/s" in stderr_output:
                # 计算速度
                speed = data_size_mb / duration if duration > 0 else 0
                return speed, duration
        
        return None, None
        
    except Exception as e:
        print(f"Error running test for buffer size {buffer_size_kb}KB: {e}")
        return None, None

def main():
    # 测试不同的缓冲区大小（KB）
    buffer_sizes = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096]
    
    print("开始缓冲区大小性能测试...")
    print("Buffer_Size_KB,Speed_MB_s,Duration_s")
    
    results = []
    
    for size in buffer_sizes:
        print(f"测试缓冲区大小: {size}KB...", file=sys.stderr)
        
        # 运行3次取平均值
        speeds = []
        durations = []
        
        for _ in range(3):
            speed, duration = run_dd_test(size)
            if speed is not None:
                speeds.append(speed)
                durations.append(duration)
        
        if speeds:
            avg_speed = sum(speeds) / len(speeds)
            avg_duration = sum(durations) / len(durations)
            
            print(f"{size},{avg_speed:.1f},{avg_duration:.3f}")
            results.append((size, avg_speed, avg_duration))
        else:
            print(f"{size},0,0")
    
    # 保存结果到文件
    with open("buffer_experiment_results.txt", "w") as f:
        f.write("Buffer_Size_KB,Speed_MB_s,Duration_s\n")
        for size, speed, duration in results:
            f.write(f"{size},{speed:.1f},{duration:.3f}\n")
    
    print("\n实验完成！结果已保存到 buffer_experiment_results.txt", file=sys.stderr)

if __name__ == "__main__":
    main() 