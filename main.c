#include "huffman.h"

void print_banner();
void print_menu();
void clear_input_buffer();
void handle_compress_single();
void handle_decompress_single();
void handle_compress_multi();
void handle_decompress_multi();
void handle_benchmark();

int main() {
    int choice;

    print_banner();

    while (1) {
        print_menu();
        printf("请输入选择 (1-6): ");

        if (scanf_s("%d", &choice) != 1) {
            clear_input_buffer();
            printf("\n输入无效，请输入数字\n");
            printf("\n按回车键继续...");
            getchar();
            system("cls");
            continue;
        }

        clear_input_buffer();
        printf("\n");

        switch (choice) {
        case 1:
            handle_compress_single();
            break;
        case 2:
            handle_decompress_single();
            break;
        case 3:
            handle_compress_multi();
            break;
        case 4:
            handle_decompress_multi();
            break;
        case 5:
            handle_benchmark();
            break;
        case 6:
            printf(" \n");
            return 0;
        default:
            printf("无效选择，输入1-6之间的数字\n");
            break;
        }
        printf("\n按回车键继续...");
        getchar();
        system("cls");
    }
    return 0;
}
void print_banner() {
    system("cls");
    printf("|-----------------------------------|\n");
    printf("|霍夫曼编码文件压缩解压             |\n");
    printf("|-----------------------------------|\n");
}

void print_menu() {
    printf("|请选择操作：                       |\n");
    printf("|1. 压缩单个文件                    |\n");
    printf("|2. 解压单个文件                    |\n");
    printf("|3. 压缩多个文件                    |\n");
    printf("|4. 解压多文件压缩包                |\n");
    printf("|5. 运行性能基准测试                |\n");
    printf("|6. 退出程序                        |\n");
    printf("|-----------------------------------|\n\n");
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void handle_compress_single() {
    char input_file[MAX_FILENAME];
    char output_file[MAX_FILENAME];

    printf("=== 压缩单个文件 ===\n");
    printf("输入要压缩的文件路径: ");
    fgets(input_file, MAX_FILENAME, stdin);
    input_file[strcspn(input_file, "\n")] = 0;

    if (!file_exists(input_file)) {
        printf("错误: 文件不存在！\n");
        return;
    }

    printf("输入输出压缩文件路径 (默认: %s.huf): ", input_file);
    fgets(output_file, MAX_FILENAME, stdin);
    output_file[strcspn(output_file, "\n")] = 0;

    if (strlen(output_file) == 0) {
        sprintf_s(output_file, MAX_FILENAME, "%s.huf", input_file);
    }

    printf("\n正在压缩文件...\n");

    clock_t start = clock();
    int result = compress_one(input_file, output_file);
    clock_t end = clock();

    if (result == 0) {
        double time = (double)(end - start) / CLOCKS_PER_SEC;
        unsigned long long original_size = file_size(input_file);
        unsigned long long compressed_size = file_size(output_file);
        double ratio = (double)compressed_size / original_size * 100;

        printf("压缩成功 \n");
        printf("原始大小: %llu 字节\n", original_size);
        printf("压缩后: %llu 字节\n", compressed_size);
        printf("压缩比: %.2f%%\n", ratio);
        printf("耗时: %.3f 秒\n", time);
    }
    else {
        printf("压缩失败 错误代码: %d\n", result);
    }
}

void handle_decompress_single() {
    char input_file[MAX_FILENAME];
    char output_file[MAX_FILENAME];

    printf("=== 解压单个文件 ===\n");
    printf("输入要解压的 .huf 文件路径: ");
    fgets(input_file, MAX_FILENAME, stdin);
    input_file[strcspn(input_file, "\n")] = 0;

    if (!file_exists(input_file)) {
        printf("错误: 文件不存在！\n");
        return;
    }

    printf("输入输出路径 (默认自动识别): ");
    fgets(output_file, MAX_FILENAME, stdin);
    output_file[strcspn(output_file, "\n")] = 0;

    printf("\n正在解压...\n");

    clock_t start = clock();
    int result = decompress_one(input_file, strlen(output_file) > 0 ? output_file : NULL);
    clock_t end = clock();

    if (result == 0) {
        printf("解压成功 耗时: %.3f 秒\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
    else {
        printf("解压失败 错误代码: %d\n", result);
    }
}

void handle_compress_multi() {
    char input_files_str[4096];
    char output_file[MAX_FILENAME];
    const char* input_files[MAX_FILES];
    int file_count = 0;

    printf("=== 压缩多个文件 ===\n");
    printf("输入文件路径(空格分隔): ");
    fgets(input_files_str, sizeof(input_files_str), stdin);
    input_files_str[strcspn(input_files_str, "\n")] = 0;

    char* token = strtok(input_files_str, " ");
    while (token != NULL && file_count < MAX_FILES) {
        if (file_exists(token)) {
            input_files[file_count++] = token;
        }
        else {
            printf("跳过不存在文件: %s\n", token);
        }
        token = strtok(NULL, " ");
    }

    if (file_count == 0) {
        printf("错误: 无有效文件\n");
        return;
    }

    printf("输入输出压缩包名称: ");
    fgets(output_file, MAX_FILENAME, stdin);
    output_file[strcspn(output_file, "\n")] = 0;

    printf("\n正在压缩 %d 个文件...\n", file_count);

    clock_t start = clock();
    int result = compress_multi(input_files, file_count, output_file);
    clock_t end = clock();

    if (result == 0) {
        printf("压缩成功 耗时: %.3f 秒\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
    else {
        printf("压缩失败 错误代码: %d\n", result);
    }
}

void handle_decompress_multi() {
    char input_file[MAX_FILENAME];
    char output_dir[MAX_FILENAME];

    printf("=== 解压多文件压缩包 ===\n");
    printf("输入 .huf 压缩包路径: ");
    fgets(input_file, MAX_FILENAME, stdin);
    input_file[strcspn(input_file, "\n")] = 0;

    if (!file_exists(input_file)) {
        printf("错误: 文件不存在\n");
        return;
    }

    printf("输入输出目录(默认当前目录): ");
    fgets(output_dir, MAX_FILENAME, stdin);
    output_dir[strcspn(output_dir, "\n")] = 0;

    printf("\n正在解压...\n");

    clock_t start = clock();
    int result = decompress_multi(input_file, strlen(output_dir) > 0 ? output_dir : NULL);
    clock_t end = clock();

    if (result == 0) {
        printf("解压成功 耗时: %.3f 秒\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
    else {
        printf("解压失败 错误代码: %d\n", result);
    }
}

void handle_benchmark() {
    char test_file[MAX_FILENAME];
    printf("=== 性能测试 ===\n");
    printf("输入测试文件路径: ");
    fgets(test_file, MAX_FILENAME, stdin);
    test_file[strcspn(test_file, "\n")] = 0;
    bench(test_file);
}