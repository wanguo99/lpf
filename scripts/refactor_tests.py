#!/usr/bin/env python3
"""
重构测试文件，移除宏定义，改用函数指针数组注册方式
"""

import os
import re
import sys
from pathlib import Path


def extract_test_functions(content):
    """提取所有测试函数定义"""
    test_funcs = []

    # 匹配 static void test_xxx(void) 格式的函数定义
    pattern = r'^\s*static\s+void\s+(test_\w+)\s*\(\s*void\s*\)'

    for line in content.split('\n'):
        match = re.match(pattern, line)
        if match:
            func_name = match.group(1)
            test_funcs.append(func_name)

    return test_funcs


def extract_suite_info(filepath):
    """从文件路径提取套件信息"""
    parts = filepath.stem.split('_')

    # 判断层级
    if 'unit' in str(filepath):
        layer_map = {
            'osal': 'OSAL',
            'hal': 'HAL',
            'pdl': 'PDL',
            'prl': 'PRL',
            'pcl': 'PCL',
            'pconfig': 'PCONFIG',
            'aconfig': 'ACONFIG'
        }
        category = 'TEST_CATEGORY_UNIT'
        tags = 'TEST_TAG_FAST'
        timeout = 100
    elif 'performance' in str(filepath):
        layer_map = {
            'osal': 'OSAL',
            'hal': 'HAL',
            'pdl': 'PDL',
            'pcl': 'PCL',
            'acl': 'ACL'
        }
        category = 'TEST_CATEGORY_PERFORMANCE'
        tags = 'TEST_TAG_SLOW'
        timeout = 5000
    elif 'stress' in str(filepath):
        layer_map = {
            'osal': 'OSAL',
            'hal': 'HAL',
            'pdl': 'PDL',
            'pcl': 'PCL',
            'acl': 'ACL'
        }
        category = 'TEST_CATEGORY_STRESS'
        tags = 'TEST_TAG_SLOW'
        timeout = 10000
    elif 'system' in str(filepath):
        layer_map = {
            'osal': 'OSAL',
            'hal': 'HAL',
            'pdl': 'PDL',
            'pconfig': 'PCONFIG',
            'aconfig': 'ACONFIG'
        }
        category = 'TEST_CATEGORY_SYSTEM'
        tags = 'TEST_TAG_SLOW | TEST_TAG_HARDWARE'
        timeout = 5000
    else:
        layer_map = {}
        category = 'TEST_CATEGORY_UNIT'
        tags = 'TEST_TAG_FAST'
        timeout = 1000

    # 提取层级名称
    layer_name = None
    for key, value in layer_map.items():
        if key in parts:
            layer_name = value
            break

    if not layer_name:
        layer_name = "UNKNOWN"

    # 生成套件名称（去掉 test_ 前缀）
    suite_name = filepath.stem
    if suite_name.startswith('test_'):
        suite_name = suite_name[5:]

    return {
        'suite_name': suite_name,
        'layer_name': layer_name,
        'category': category,
        'tags': tags,
        'timeout': timeout
    }


def generate_test_case_array(test_funcs):
    """生成测试用例数组代码"""
    lines = []
    lines.append("/* 测试用例数组 - 使用函数指针数组 */")
    lines.append(f"static const test_case_t test_cases[] = {{")

    for func in test_funcs:
        lines.append(f"\t{{")
        lines.append(f"\t\t.name = \"{func}\",")
        lines.append(f"\t\t.func = {func},")
        lines.append(f"\t\t.setup = NULL,")
        lines.append(f"\t\t.teardown = NULL")
        lines.append(f"\t}},")

    lines.append("};")
    return '\n'.join(lines)


def generate_suite_registration(suite_info, description=""):
    """生成测试套件注册代码"""
    suite_name = suite_info['suite_name']
    layer_name = suite_info['layer_name']
    category = suite_info['category']
    tags = suite_info['tags']
    timeout = suite_info['timeout']

    if not description:
        description = f"{layer_name} {suite_name} tests"

    lines = []
    lines.append("")
    lines.append("/* 测试套件定义 */")
    lines.append(f"static const test_suite_t test_suite = {{")
    lines.append(f"\t.suite_name = \"{suite_name}\",")
    lines.append(f"\t.module_name = \"{suite_name}\",")
    lines.append(f"\t.layer_name = \"{layer_name}\",")
    lines.append(f"\t.cases = test_cases,")
    lines.append(f"\t.case_count = sizeof(test_cases) / sizeof(test_case_t),")
    lines.append(f"\t.suite_setup = NULL,")
    lines.append(f"\t.suite_teardown = NULL,")
    lines.append(f"\t.metadata = {{")
    lines.append(f"\t\t.category = {category},")
    lines.append(f"\t\t.tags = {tags},")
    lines.append(f"\t\t.timeout_ms = {timeout},")
    lines.append(f"\t\t.description = \"{description}\"")
    lines.append(f"\t}}")
    lines.append("};")
    lines.append("")
    lines.append("/* 测试套件注册函数 */")
    lines.append("__attribute__((constructor))")
    lines.append(f"static void register_{suite_name}_tests(void)")
    lines.append("{")
    lines.append(f"\tlibutest_register_suite(&test_suite);")
    lines.append("}")

    return '\n'.join(lines)


def remove_old_registration(content):
    """移除旧的注册代码（从测试用例数组开始到文件末尾）"""
    # 找到第一个测试用例数组定义
    pattern = r'/\*\s*测试用例数组.*?\*/\s*static\s+const\s+test_case_t\s+\w+.*?$'
    match = re.search(pattern, content, re.DOTALL | re.MULTILINE)

    if match:
        # 截断到数组定义之前
        content = content[:match.start()]

    # 移除末尾的空白行
    content = content.rstrip()

    return content


def refactor_test_file(filepath):
    """重构单个测试文件"""
    print(f"Processing: {filepath}")

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # 提取测试函数
    test_funcs = extract_test_functions(content)
    if not test_funcs:
        print(f"  No test functions found, skipping")
        return False

    print(f"  Found {len(test_funcs)} test functions: {', '.join(test_funcs[:3])}...")

    # 提取套件信息
    suite_info = extract_suite_info(Path(filepath))
    print(f"  Suite: {suite_info['suite_name']}, Layer: {suite_info['layer_name']}")

    # 移除旧的注册代码
    content = remove_old_registration(content)

    # 生成新的注册代码
    test_case_array = generate_test_case_array(test_funcs)
    suite_registration = generate_suite_registration(suite_info)

    # 添加新的代码
    content += '\n\n' + test_case_array
    content += '\n' + suite_registration
    content += '\n'

    # 写回文件
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"  ✓ Refactored successfully")
    return True


def main():
    # 测试文件目录
    test_dirs = [
        '/home/wanguo/ES-Middleware/products/tests/unit',
        '/home/wanguo/ES-Middleware/products/tests/performance',
        '/home/wanguo/ES-Middleware/products/tests/stress',
        '/home/wanguo/ES-Middleware/products/tests/system'
    ]

    total_files = 0
    success_files = 0

    for test_dir in test_dirs:
        if not os.path.exists(test_dir):
            continue

        # 递归查找所有 .c 文件
        for root, dirs, files in os.walk(test_dir):
            for file in files:
                if file.endswith('.c') and file.startswith('test_'):
                    filepath = os.path.join(root, file)
                    total_files += 1

                    try:
                        if refactor_test_file(filepath):
                            success_files += 1
                    except Exception as e:
                        print(f"  ✗ Error: {e}")
                        import traceback
                        traceback.print_exc()

    print(f"\n{'='*60}")
    print(f"Total files processed: {total_files}")
    print(f"Successfully refactored: {success_files}")
    print(f"Failed: {total_files - success_files}")
    print(f"{'='*60}")


if __name__ == '__main__':
    main()
