#include "huffman.h"

static HuffmanNode* new_node(unsigned char d, unsigned int f,
    HuffmanNode* l, HuffmanNode* r) {
    HuffmanNode* n = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    if (!n) return NULL;
    n->data = d; n->freq = f; n->left = l; n->right = r;
    return n;
}

static int cmp_node(const void* a, const void* b) {
    const HuffmanNode* const* pa = (const HuffmanNode* const*)a;
    const HuffmanNode* const* pb = (const HuffmanNode* const*)b;
    return (int)((*pa)->freq - (*pb)->freq);
}

HuffmanNode* create_tree(unsigned int* freq) {
    HuffmanNode* nodes[256];
    int n = 0;
    for (int i = 0; i < 256; i++)
        if (freq[i]) nodes[n++] = new_node((unsigned char)i, freq[i], NULL, NULL);
    if (n == 0) return NULL;
    if (n == 1) return new_node(0, 0, nodes[0], NULL);

    while (n > 1) {
        qsort(nodes, n, sizeof(HuffmanNode*), cmp_node);
        HuffmanNode* l = nodes[0];
        HuffmanNode* r = nodes[1];
        HuffmanNode* p = new_node(0, l->freq + r->freq, l, r);
        if (!p) return NULL;
        nodes[0] = p;
        for (int i = 1; i < n - 1; i++) nodes[i] = nodes[i + 1];
        n--;
    }
    return nodes[0];
}

static void gen_code(HuffmanNode* p, HuffmanCode* tab, unsigned int code, unsigned char len) {
    if (!p) return;
    if (!p->left && !p->right) {
        tab[p->data].code = code;
        tab[p->data].len = len;
        return;
    }
    gen_code(p->left, tab, code << 1, len + 1);
    gen_code(p->right, tab, (code << 1) | 1, len + 1);
}

void build_codes(HuffmanNode* root, HuffmanCode* tab) {
    memset(tab, 0, sizeof(HuffmanCode) * 256);
    gen_code(root, tab, 0, 0);
}

void free_tree(HuffmanNode* p) {
    if (!p) return;
    free_tree(p->left);
    free_tree(p->right);
    free(p);
}

unsigned long long file_size(const char* fn) {
    struct stat st;
    return stat(fn, &st) ? 0 : (unsigned long long)st.st_size;
}

bool file_exists(const char* fn) {
    struct stat st;
    return stat(fn, &st) == 0;
}

void mk_dir(const char* path) {
    if (!file_exists(path)) {
        char cmd[MAX_FILENAME + 20];
        sprintf_s(cmd, sizeof(cmd), "mkdir \"%s\" 2>nul", path);
        system(cmd);
    }
}

//单文件内容
int compress_one(const char* in, const char* out) {
    FILE* fi, * fo;
    if (fopen_s(&fi, in, "rb") != 0) return -1;

    unsigned int freq[256] = { 0 };
    unsigned char buf[BLOCK_SIZE];
    size_t rd;
    while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0)
        for (size_t i = 0; i < rd; i++) freq[buf[i]]++;

    HuffmanNode* tree = create_tree(freq);
    if (!tree) { fclose(fi); return -2; }
    HuffmanCode tab[256];
    build_codes(tree, tab);

    if (fopen_s(&fo, out, "wb") != 0) { fclose(fi); free_tree(tree); return -3; }

    HufHeader hdr;
    memcpy(hdr.magic, MAGIC_NUMBER, 4);
    hdr.file_cnt = 1;
    memcpy(hdr.freq, freq, sizeof(freq));
    fwrite(&hdr, sizeof(hdr), 1, fo);

    FileInfo fi_info;
    strncpy_s(fi_info.filename, MAX_FILENAME, in, _TRUNCATE);
    fi_info.orig_size = file_size(in);
    fi_info.comp_size = 0;
    fi_info.offset = sizeof(HufHeader) + sizeof(FileInfo);
    fwrite(&fi_info, sizeof(FileInfo), 1, fo);

    rewind(fi);
    unsigned char cur = 0;
    int bit = 0;
    unsigned long long comp = 0;

    while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0) {
        for (size_t i = 0; i < rd; i++) {
            HuffmanCode c = tab[buf[i]];
            for (int j = c.len - 1; j >= 0; j--) {
                cur = (cur << 1) | ((c.code >> j) & 1);
                if (++bit == 8) {
                    fputc(cur, fo);
                    comp++;
                    cur = 0; bit = 0;
                }
            }
        }
    }
    if (bit > 0) {
        cur <<= (8 - bit);
        fputc(cur, fo);
        comp++;
    }

    fi_info.comp_size = comp;
    fseek(fo, sizeof(HufHeader), SEEK_SET);
    fwrite(&fi_info, sizeof(FileInfo), 1, fo);

    fclose(fi); fclose(fo); free_tree(tree);
    return 0;
}

int decompress_one(const char* in, const char* out) {
    FILE* fi, * fo;
    if (fopen_s(&fi, in, "rb") != 0) return -1;

    HufHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fi) != 1) { fclose(fi); return -2; }
    if (memcmp(hdr.magic, MAGIC_NUMBER, 4) != 0) { fclose(fi); return -3; }

    FileInfo fi_info;
    if (fread(&fi_info, sizeof(FileInfo), 1, fi) != 1) { fclose(fi); return -4; }

    char outpath[MAX_FILENAME];
    if (!out || strlen(out) == 0) strcpy_s(outpath, MAX_FILENAME, fi_info.filename);
    else strcpy_s(outpath, MAX_FILENAME, out);

    HuffmanNode* tree = create_tree(hdr.freq);
    if (!tree) { fclose(fi); return -5; }

    if (fopen_s(&fo, outpath, "wb") != 0) { fclose(fi); free_tree(tree); return -6; }

    fseek(fi, (long)fi_info.offset, SEEK_SET);
    unsigned char buf[BLOCK_SIZE];
    size_t rd;
    HuffmanNode* p = tree;
    unsigned long long written = 0;

    while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0 && written < fi_info.orig_size) {
        for (size_t i = 0; i < rd && written < fi_info.orig_size; i++) {
            unsigned char b = buf[i];
            for (int j = 7; j >= 0 && written < fi_info.orig_size; j--) {
                int bit = (b >> j) & 1;
                p = bit ? p->right : p->left;
                if (!p->left && !p->right) {
                    fputc(p->data, fo);
                    written++;
                    p = tree;
                }
            }
        }
    }

    fclose(fi); fclose(fo); free_tree(tree);
    return 0;
}
//多文件内容
int compress_multi(const char** files, int cnt, const char* out) {
    if (cnt <= 0 || cnt > MAX_FILES) return -1;
    FILE* fo;
    if (fopen_s(&fo, out, "wb") != 0) return -2;
    //统计全局
    unsigned int freq[256] = { 0 };
    unsigned char buf[BLOCK_SIZE];
    size_t rd;
    for (int i = 0; i < cnt; i++) {
        FILE* fi;
        if (fopen_s(&fi, files[i], "rb") != 0) { fclose(fo); return -3; }
        while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0)
            for (size_t j = 0; j < rd; j++) freq[buf[j]]++;
        fclose(fi);
    }

    HuffmanNode* tree = create_tree(freq);
    if (!tree) { fclose(fo); return -4; }
    HuffmanCode tab[256];
    build_codes(tree, tab);

    //写头
    HufHeader hdr;
    memcpy(hdr.magic, MAGIC_NUMBER, 4);
    hdr.file_cnt = cnt;
    memcpy(hdr.freq, freq, sizeof(freq));
    fwrite(&hdr, sizeof(hdr), 1, fo);

    //预留 FileInfo 区
    FileInfo* infos = (FileInfo*)calloc(cnt, sizeof(FileInfo));
    if (!infos) { fclose(fo); free_tree(tree); return -5; }
    fwrite(infos, sizeof(FileInfo), cnt, fo);

    //逐个文件压缩
    for (int i = 0; i < cnt; i++) {
        FILE* fi;
        if (fopen_s(&fi, files[i], "rb") != 0) { fclose(fo); free(infos); free_tree(tree); return -6; }

        strncpy_s(infos[i].filename, MAX_FILENAME, files[i], _TRUNCATE);
        infos[i].orig_size = file_size(files[i]);
        infos[i].offset = (unsigned long long)ftell(fo);

        unsigned long long start = (unsigned long long)ftell(fo);
        unsigned char cur = 0;
        int bit = 0;

        while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0) {
            for (size_t j = 0; j < rd; j++) {
                HuffmanCode c = tab[buf[j]];
                for (int k = c.len - 1; k >= 0; k--) {
                    cur = (cur << 1) | ((c.code >> k) & 1);
                    if (++bit == 8) {
                        fputc(cur, fo);
                        cur = 0; bit = 0;
                    }
                }
            }
        }
        if (bit > 0) {
            cur <<= (8 - bit);
            fputc(cur, fo);
        }

        infos[i].comp_size = (unsigned long long)ftell(fo) - start;
        fclose(fi);
    }
    fseek(fo, sizeof(HufHeader), SEEK_SET);
    fwrite(infos, sizeof(FileInfo), cnt, fo);

    fclose(fo);
    free(infos);
    free_tree(tree);
    return 0;
}

int decompress_multi(const char* in, const char* outdir) {
    FILE* fi;
    if (fopen_s(&fi, in, "rb") != 0) return -1;

    HufHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fi) != 1) { fclose(fi); return -2; }
    if (memcmp(hdr.magic, MAGIC_NUMBER, 4) != 0) { fclose(fi); return -3; }

    int cnt = hdr.file_cnt;
    FileInfo* infos = (FileInfo*)malloc(cnt * sizeof(FileInfo));
    if (!infos) { fclose(fi); return -4; }
    if (fread(infos, sizeof(FileInfo), cnt, fi) != (size_t)cnt) { fclose(fi); free(infos); return -5; }

    HuffmanNode* tree = create_tree(hdr.freq);
    if (!tree) { fclose(fi); free(infos); return -6; }

    if (outdir && strlen(outdir) > 0) mk_dir(outdir);

    //逐个解压
    for (int i = 0; i < cnt; i++) {
        char outpath[MAX_FILENAME];
        if (outdir && strlen(outdir) > 0)
            sprintf_s(outpath, MAX_FILENAME, "%s\\%s", outdir, infos[i].filename);
        else
            strcpy_s(outpath, MAX_FILENAME, infos[i].filename);

        FILE* fo;
        if (fopen_s(&fo, outpath, "wb") != 0) { fclose(fi); free(infos); free_tree(tree); return -7; }

        fseek(fi, (long)infos[i].offset, SEEK_SET);
        unsigned char buf[BLOCK_SIZE];
        size_t rd;
        HuffmanNode* p = tree;
        unsigned long long written = 0;

        while ((rd = fread(buf, 1, BLOCK_SIZE, fi)) > 0 && written < infos[i].orig_size) {
            for (size_t j = 0; j < rd && written < infos[i].orig_size; j++) {
                unsigned char b = buf[j];
                for (int k = 7; k >= 0 && written < infos[i].orig_size; k--) {
                    int bit = (b >> k) & 1;
                    p = bit ? p->right : p->left;
                    if (!p->left && !p->right) {
                        fputc(p->data, fo);
                        written++;
                        p = tree;
                    }
                }
            }
        }
        fclose(fo);
    }

    fclose(fi);
    free(infos);
    free_tree(tree);
    return 0;
}

void bench(const char* fn) {
    if (!file_exists(fn)) { printf("不存在：%s\n", fn); return; }
    printf("=== Benchmark ===\n文件：%s\n大小：%llu bytes\n", fn, file_size(fn));

    char cmp[512], dec[512];
    sprintf_s(cmp, sizeof(cmp), "%s.bench.huf", fn);
    sprintf_s(dec, sizeof(dec), "%s.bench.out", fn);

    clock_t t = clock();
    compress_one(fn, cmp);
    double ct = (double)(clock() - t) / CLOCKS_PER_SEC;

    t = clock();
    decompress_one(cmp, dec);
    double dt = (double)(clock() - t) / CLOCKS_PER_SEC;

    unsigned long long os = file_size(fn);
    unsigned long long cs = file_size(cmp);
    printf("压缩时间：%.3fs | 解压时间：%.3fs\n压缩比：%.2f%%\n",
        ct, dt, 100.0 * cs / os);

    remove(cmp); remove(dec);
} 