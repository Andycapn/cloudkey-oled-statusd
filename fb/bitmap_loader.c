#include "bitmap_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int load_bitmap(const char *path, int w, int h, struct bitmap *out) {
    int row_bytes = (w + 7) / 8;
    size_t expected_size = row_bytes * h;

    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat bitmap");
        return -1;
    }

    if ((size_t)st.st_size != expected_size) {
        fprintf(stderr,
            "bitmap size mismatch: %s (got %ld, expected %zu)\n",
            path, st.st_size, expected_size);
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen bitmap");
        return -1;
    }

    uint8_t *data = malloc(expected_size);
    if (!data) {
        perror("malloc bitmap");
        fclose(f);
        return -1;
    }

    if (fread(data, 1, expected_size, f) != expected_size) {
        perror("fread bitmap");
        fclose(f);
        free(data);
        return -1;
    }

    fclose(f);

    out->w = w;
    out->h = h;
    out->data = data;

    return 0;
}

void free_bitmap(struct bitmap *bmp) {
    if (bmp->data) {
        free((void *)bmp->data);
        bmp->data = NULL;
    }
}
