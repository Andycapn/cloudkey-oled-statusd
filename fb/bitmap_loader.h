#pragma once
#include "bitmap.h"

int load_bitmap(const char *path, int w, int h, struct bitmap *out);
int load_coverage_bitmap(const char *path, int w, int h, struct bitmap *out);
void free_bitmap(struct bitmap *bmp);
