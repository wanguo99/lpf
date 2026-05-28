#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EMS 构建系统压力测试脚本
对每个产品配置进行多次编译测试，确保构建系统的稳定性
"""

import argparse
import subprocess
import sys
import time
from pathlib import Path
from datetime import datetime

def get_products_and_configs():
    """获取所有产品和配置"""
    products_configs = {
        'ccm': ['h200_100p_v1', 'h200_100p_v2', 'h200_200p'],
        'sample': ['default']
    }
    return products_configs

def distclean_product(product):
    """清理产品构建目录"""
    root_dir = Path(__file__).parent

    # 使用 build.py --clean
    clean_cmd = [
        "python3", "build.py",
        "--product", product,
        "--clean"
    ]

    result = subprocess.run(
        clean_cmd,
        cwd=root_dir,
        capture_output=True,
        text=True
    )
    return result.returncode == 0

def build_product(product, config):
    """构建产品"""
    root_dir = Path(__file__).parent

    build_cmd = [
        "python3", "build.py",
        "--product", product,
        "--config", config
    ]

    result = subprocess.run(
        build_cmd,
        cwd=root_dir,
        capture_output=True,
        text=True
    )

    return result.returncode == 0, result.stdout, result.stderr

def stress_test(product, config, iterations=300):
    """对指定产品配置进行压力测试"""
    print(f"\n{'='*80}")
    print(f"开始压力测试: {product} - {config}")
    print(f"测试次数: {iterations}")
    print(f"{'='*80}\n")

    success_count = 0
    fail_count = 0
    total_time = 0

    for i in range(1, iterations + 1):
        start_time = time.time()

        # 每次编译前清理
        print(f"[{i}/{iterations}] 清理 {product}...", end=" ", flush=True)
        if not distclean_product(product):
            print("❌ 清理失败")
            fail_count += 1
            continue
        print("✓", end=" ", flush=True)

        # 编译
        print(f"编译 {product}/{config}...", end=" ", flush=True)
        success, stdout, stderr = build_product(product, config)

        elapsed = time.time() - start_time
        total_time += elapsed

        if success:
            print(f"✓ ({elapsed:.2f}s)")
            success_count += 1
        else:
            print(f"❌ ({elapsed:.2f}s)")
            fail_count += 1

            # 保存失败日志
            log_dir = Path(__file__).parent / "stress_test_logs"
            log_dir.mkdir(exist_ok=True)

            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            log_file = log_dir / f"fail_{product}_{config}_{i}_{timestamp}.log"

            with open(log_file, 'w') as f:
                f.write(f"Product: {product}\n")
                f.write(f"Config: {config}\n")
                f.write(f"Iteration: {i}/{iterations}\n")
                f.write(f"Time: {datetime.now()}\n")
                f.write(f"\n{'='*80}\n")
                f.write("STDOUT:\n")
                f.write(stdout)
                f.write(f"\n{'='*80}\n")
                f.write("STDERR:\n")
                f.write(stderr)

            print(f"    失败日志已保存: {log_file}")

        # 每 10 次显示进度统计
        if i % 10 == 0:
            avg_time = total_time / i
            success_rate = (success_count / i) * 100
            print(f"    进度: {i}/{iterations} | 成功率: {success_rate:.1f}% | 平均耗时: {avg_time:.2f}s")

    # 最终统计
    print(f"\n{'='*80}")
    print(f"压力测试完成: {product} - {config}")
    print(f"{'='*80}")
    print(f"总测试次数: {iterations}")
    print(f"成功次数:   {success_count}")
    print(f"失败次数:   {fail_count}")
    print(f"成功率:     {(success_count/iterations)*100:.2f}%")
    print(f"总耗时:     {total_time:.2f}s")
    print(f"平均耗时:   {total_time/iterations:.2f}s")
    print(f"{'='*80}\n")

    return success_count, fail_count

def main():
    parser = argparse.ArgumentParser(
        description="EMS 构建系统压力测试",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 测试所有产品配置（每个 300 次）
  python3 stress_test_build.py

  # 测试所有产品配置（每个 100 次）
  python3 stress_test_build.py --iterations 100

  # 只测试 CCM 产品
  python3 stress_test_build.py --product ccm

  # 只测试特定配置
  python3 stress_test_build.py --product ccm --config h200_100p_v1 --iterations 50
        """
    )

    parser.add_argument("--iterations", "-n", type=int, default=300,
                        help="每个配置的测试次数（默认: 300）")
    parser.add_argument("--product", "-p", type=str,
                        help="指定测试的产品（不指定则测试所有）")
    parser.add_argument("--config", "-c", type=str,
                        help="指定测试的配置（需要同时指定 --product）")

    args = parser.parse_args()

    # 获取所有产品配置
    all_products_configs = get_products_and_configs()

    # 确定要测试的产品配置
    if args.product:
        if args.product not in all_products_configs:
            print(f"错误: 产品 '{args.product}' 不存在")
            print(f"可用产品: {', '.join(all_products_configs.keys())}")
            return 1

        if args.config:
            if args.config not in all_products_configs[args.product]:
                print(f"错误: 配置 '{args.config}' 不存在于产品 '{args.product}'")
                print(f"可用配置: {', '.join(all_products_configs[args.product])}")
                return 1
            test_configs = {args.product: [args.config]}
        else:
            test_configs = {args.product: all_products_configs[args.product]}
    else:
        test_configs = all_products_configs

    # 开始测试
    print(f"\n{'#'*80}")
    print(f"# EMS 构建系统压力测试")
    print(f"# 开始时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"# 每个配置测试次数: {args.iterations}")
    print(f"{'#'*80}")

    total_success = 0
    total_fail = 0

    for product, configs in test_configs.items():
        for config in configs:
            success, fail = stress_test(product, config, args.iterations)
            total_success += success
            total_fail += fail

    # 总体统计
    total_tests = total_success + total_fail
    print(f"\n{'#'*80}")
    print(f"# 所有测试完成")
    print(f"# 结束时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"{'#'*80}")
    print(f"总测试次数: {total_tests}")
    print(f"总成功次数: {total_success}")
    print(f"总失败次数: {total_fail}")
    print(f"总成功率:   {(total_success/total_tests)*100:.2f}%")
    print(f"{'#'*80}\n")

    return 0 if total_fail == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
