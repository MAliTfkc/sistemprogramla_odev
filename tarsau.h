#ifndef TARSAU_H
#define TARSAU_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>

#define MAX_INPUT_FILES 32
#define MAX_TOTAL_BYTES (200UL * 1024UL * 1024UL)
#define INDEX_SIZE_FIELD 10
#define DEFAULT_ARCHIVE "a.sau"

typedef struct {
    char *name;
    char *permissions;
    size_t size;
} ArchiveEntry;

int is_text_file(const char *path);
int bundle_files(char **inputs, int input_count, const char *output_path);
int extract_archive(const char *archive_path, const char *output_dir);
void free_entries(ArchiveEntry *entries, int count);

#endif
