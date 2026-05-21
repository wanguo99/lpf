/*
 * fixdep - 简化版依赖修复工具
 * 用于处理 gcc -MD 生成的依赖文件
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	/* 简化版：直接复制依赖文件 */
	if (argc < 4) {
		fprintf(stderr, "Usage: %s <depfile> <target> <cmdline>\n", argv[0]);
		return 1;
	}

	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		perror(argv[1]);
		return 1;
	}

	char line[4096];
	while (fgets(line, sizeof(line), fp)) {
		printf("%s", line);
	}

	fclose(fp);
	return 0;
}
