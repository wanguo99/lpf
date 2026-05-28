#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EMS SDK Build Script
Build products from the root directory
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path

def get_products():
    """获取所有可用的产品"""
    products_dir = Path(__file__).parent / "products"
    if not products_dir.exists():
        return []

    products = []
    for item in products_dir.iterdir():
        if item.is_dir() and (item / "CMakeLists.txt").exists():
            products.append(item.name)
    return sorted(products)

def get_configs(product):
    """获取产品的所有配置"""
    configs_dir = Path(__file__).parent / "products" / product / "configs"
    if not configs_dir.exists():
        return []

    configs = []
    for item in configs_dir.iterdir():
        if item.is_file() and item.name.endswith("_defconfig"):
            config_name = item.name.replace("_defconfig", "")
            configs.append(config_name)
    return sorted(configs)

def menuconfig(product):
    """打开产品的 menuconfig 配置界面"""
    root_dir = Path(__file__).parent
    product_dir = root_dir / "products" / product

    # 检查产品目录
    if not product_dir.exists():
        print(f"Error: Product directory not found: {product_dir}")
        return False

    # 调用产品的 project.py menuconfig
    project_script = product_dir / "project.py"
    if not project_script.exists():
        print(f"Error: project.py not found: {project_script}")
        return False

    print(f"Opening menuconfig for product: {product}")
    print(f"Product directory: {product_dir}")
    print()

    # 执行 menuconfig
    menuconfig_cmd = ["python3", str(project_script), "menuconfig"]
    result = subprocess.run(menuconfig_cmd, cwd=product_dir)

    if result.returncode != 0:
        print("\nMenuconfig exited with error!")
        return False

    print("\nConfiguration saved.")
    return True

def build(product, config=None, build_dir="build", clean=False, jobs=None):
    """构建产品"""
    root_dir = Path(__file__).parent
    product_dir = root_dir / "products" / product

    # 如果指定了配置，复制配置文件
    if config:
        config_file = product_dir / "configs" / f"{config}_defconfig"
        if not config_file.exists():
            print(f"Error: Config file not found: {config_file}")
            return False

        target_config = product_dir / ".config.mk"
        print(f"Loading config: {config}")
        import shutil
        shutil.copy(config_file, target_config)

    # 清理
    if clean:
        product_build = product_dir / "build"
        if product_build.exists():
            print(f"Cleaning build directory: {product_build}")
            import shutil
            shutil.rmtree(product_build)

    # 调用产品的 project.py
    project_script = product_dir / "project.py"
    if not project_script.exists():
        print(f"Error: project.py not found: {project_script}")
        return False

    print(f"Building product: {product}")

    # 构建命令
    build_cmd = ["python3", str(project_script), "build"]

    result = subprocess.run(build_cmd, cwd=product_dir)
    if result.returncode != 0:
        print("Build failed!")
        return False

    print(f"\nBuild successful!")
    print(f"Output directory: {product_dir / 'build'}")

    # 创建符号链接到根目录 build
    root_build = root_dir / build_dir / product
    root_build.parent.mkdir(parents=True, exist_ok=True)

    if root_build.exists() or root_build.is_symlink():
        root_build.unlink()

    root_build.symlink_to(product_dir / "build", target_is_directory=True)
    print(f"Symlink created: {root_build} -> {product_dir / 'build'}")

    return True

def list_products():
    """列出所有产品"""
    products = get_products()
    if not products:
        print("No products found")
        return

    print("Available products:")
    for product in products:
        configs = get_configs(product)
        print(f"  - {product}")
        if configs:
            print(f"    Configs: {', '.join(configs)}")

def main():
    parser = argparse.ArgumentParser(
        description="EMS SDK Build Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all products
  python3 build.py --list

  # Open menuconfig for a product
  python3 build.py --product ccm --menuconfig

  # Build with default config
  python3 build.py --product ccm

  # Build with specific config
  python3 build.py --product ccm --config h200_100p_v1

  # Clean build
  python3 build.py --product ccm --config h200_200p --clean
        """
    )

    parser.add_argument("--list", action="store_true",
                        help="List all available products and configs")
    parser.add_argument("--menuconfig", action="store_true",
                        help="Open menuconfig for the specified product")
    parser.add_argument("--product", "-p", type=str,
                        help="Product to build or configure")
    parser.add_argument("--config", "-c", type=str,
                        help="Product configuration (defconfig name without _defconfig suffix)")
    parser.add_argument("--build-dir", "-b", type=str, default="build",
                        help="Build directory (default: build)")
    parser.add_argument("--clean", action="store_true",
                        help="Clean before build")
    parser.add_argument("--jobs", "-j", type=int,
                        help="Number of parallel jobs")

    args = parser.parse_args()

    # 列出产品
    if args.list:
        list_products()
        return 0

    # 检查是否指定了产品
    if not args.product:
        parser.print_help()
        print("\nError: --product is required")
        print("\nUse --list to see available products")
        return 1

    # 检查产品是否存在
    products = get_products()
    if args.product not in products:
        print(f"Error: Product '{args.product}' not found")
        print(f"Available products: {', '.join(products)}")
        return 1

    # 打开 menuconfig
    if args.menuconfig:
        success = menuconfig(product=args.product)
        return 0 if success else 1

    # 检查配置是否存在
    if args.config:
        configs = get_configs(args.product)
        if args.config not in configs:
            print(f"Error: Config '{args.config}' not found for product '{args.product}'")
            if configs:
                print(f"Available configs: {', '.join(configs)}")
            return 1

    # 构建
    success = build(
        product=args.product,
        config=args.config,
        build_dir=args.build_dir,
        clean=args.clean,
        jobs=args.jobs
    )

    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
