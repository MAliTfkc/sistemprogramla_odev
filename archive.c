#include "tarsau.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static char *permissions_from_mode(mode_t mode)
{
    char *permissions;

    permissions = malloc(4);
    if (permissions == NULL) {
        return NULL;
    }

    snprintf(permissions, 4, "%o", mode & 0777);
    return permissions;
}

static mode_t mode_from_permissions(const char *permissions)
{
    char *end;
    unsigned long value;

    value = strtoul(permissions, &end, 8);
    if (end == permissions || *end != '\0') {
        return 0;
    }

    return (mode_t)(value & 0777);
}

static char *build_index(ArchiveEntry *entries, int count, size_t *index_size)
{
    size_t capacity;
    size_t length;
    char *index;
    int index_entry;

    capacity = 64;
    length = 0;
    index = malloc(capacity);
    if (index == NULL) {
        return NULL;
    }

    for (index_entry = 0; index_entry < count; ++index_entry) {
        int needed;
        char *next;

        needed = snprintf(NULL, 0, "|%s,%s,%zu|",
                          entries[index_entry].name,
                          entries[index_entry].permissions,
                          entries[index_entry].size);
        if (needed < 0) {
            free(index);
            return NULL;
        }

        while (length + (size_t)needed + 1 > capacity) {
            capacity *= 2;
            next = realloc(index, capacity);
            if (next == NULL) {
                free(index);
                return NULL;
            }
            index = next;
        }

        snprintf(index + length, capacity - length, "|%s,%s,%zu|",
                 entries[index_entry].name,
                 entries[index_entry].permissions,
                 entries[index_entry].size);
        length += (size_t)needed;
    }

    *index_size = length;
    return index;
}

static int write_index_size(FILE *output, size_t index_size)
{
    char size_field[INDEX_SIZE_FIELD + 1];

    if (index_size >= 10000000000UL) {
        return -1;
    }

    snprintf(size_field, sizeof(size_field), "%010zu", index_size);
    if (fwrite(size_field, 1, INDEX_SIZE_FIELD, output) != INDEX_SIZE_FIELD) {
        return -1;
    }

    return 0;
}

static int append_file_contents(FILE *output, const char *path)
{
    FILE *input;
    unsigned char buffer[4096];
    size_t read_count;

    input = fopen(path, "rb");
    if (input == NULL) {
        return -1;
    }

    while ((read_count = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        if (fwrite(buffer, 1, read_count, output) != read_count) {
            fclose(input);
            return -1;
        }
    }

    if (ferror(input)) {
        fclose(input);
        return -1;
    }

    fclose(input);
    return 0;
}

int bundle_files(char **inputs, int input_count, const char *output_path)
{
    ArchiveEntry *entries;
    FILE *output;
    char *index;
    size_t index_size;
    size_t total_size;
    int index_entry;
    int status;

    entries = calloc((size_t)input_count, sizeof(ArchiveEntry));
    if (entries == NULL) {
        return -1;
    }

    total_size = 0;
    for (index_entry = 0; index_entry < input_count; ++index_entry) {
        struct stat file_stat;

        char *display_name;
        char *display_copy;

        display_copy = strdup(inputs[index_entry]);
        if (display_copy == NULL) {
            free_entries(entries, input_count);
            return -1;
        }

        display_name = basename(display_copy);
        if (!is_text_file(inputs[index_entry])) {
            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", display_name);
            free(display_copy);
            free_entries(entries, input_count);
            return 0;
        }
        free(display_copy);

        if (stat(inputs[index_entry], &file_stat) != 0) {
            free_entries(entries, input_count);
            return -1;
        }

        if (!S_ISREG(file_stat.st_mode)) {
            display_copy = strdup(inputs[index_entry]);
            if (display_copy == NULL) {
                free_entries(entries, input_count);
                return -1;
            }

            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n",
                    basename(display_copy));
            free(display_copy);
            free_entries(entries, input_count);
            return 0;
        }

        entries[index_entry].name = strdup(inputs[index_entry]);
        entries[index_entry].permissions = permissions_from_mode(file_stat.st_mode);
        entries[index_entry].size = (size_t)file_stat.st_size;
        if (entries[index_entry].name == NULL || entries[index_entry].permissions == NULL) {
            free_entries(entries, input_count);
            return -1;
        }

        total_size += entries[index_entry].size;
        if (total_size > MAX_TOTAL_BYTES) {
            free_entries(entries, input_count);
            return -1;
        }
    }

    index = build_index(entries, input_count, &index_size);
    if (index == NULL) {
        free_entries(entries, input_count);
        return -1;
    }

    output = fopen(output_path, "wb");
    if (output == NULL) {
        free(index);
        free_entries(entries, input_count);
        return -1;
    }

    status = 0;
    if (write_index_size(output, index_size) != 0 ||
        fwrite(index, 1, index_size, output) != index_size) {
        status = -1;
    }

    for (index_entry = 0; status == 0 && index_entry < input_count; ++index_entry) {
        if (append_file_contents(output, inputs[index_entry]) != 0) {
            status = -1;
        }
    }

    if (fclose(output) != 0) {
        status = -1;
    }

    free(index);
    free_entries(entries, input_count);
    return status;
}

static int ensure_directory(const char *path)
{
    struct stat directory_stat;

    if (stat(path, &directory_stat) == 0) {
        return S_ISDIR(directory_stat.st_mode) ? 0 : -1;
    }

    if (mkdir(path, 0755) != 0) {
        return -1;
    }

    return 0;
}

static int parse_index(const char *index, size_t index_size, ArchiveEntry **entries_out, int *count_out)
{
    ArchiveEntry *entries;
    int count;
    size_t position;

    entries = NULL;
    count = 0;
    position = 0;

    while (position < index_size) {
        const char *record_start;
        const char *record_end;
        char record[1024];
        char *name;
        char *permissions;
        char *size_text;
        char *comma_one;
        char *comma_two;
        size_t record_length;
        ArchiveEntry *next_entries;

        while (position < index_size && index[position] != '|') {
            ++position;
        }

        if (position >= index_size) {
            break;
        }

        record_start = index + position + 1;
        record_end = strchr(record_start, '|');
        if (record_end == NULL || record_end == record_start) {
            return -1;
        }

        record_length = (size_t)(record_end - record_start);
        if (record_length >= sizeof(record)) {
            return -1;
        }

        memcpy(record, record_start, record_length);
        record[record_length] = '\0';
        position = (size_t)(record_end - index) + 1;

        comma_one = strchr(record, ',');
        if (comma_one == NULL) {
            return -1;
        }

        comma_two = strchr(comma_one + 1, ',');
        if (comma_two == NULL) {
            return -1;
        }

        *comma_one = '\0';
        *comma_two = '\0';
        name = record;
        permissions = comma_one + 1;
        size_text = comma_two + 1;

        next_entries = realloc(entries, (size_t)(count + 1) * sizeof(ArchiveEntry));
        if (next_entries == NULL) {
            free_entries(entries, count);
            return -1;
        }

        entries = next_entries;
        entries[count].name = strdup(name);
        entries[count].permissions = strdup(permissions);
        entries[count].size = strtoul(size_text, NULL, 10);
        if (entries[count].name == NULL || entries[count].permissions == NULL) {
            free_entries(entries, count + 1);
            return -1;
        }

        ++count;
    }

    *entries_out = entries;
    *count_out = count;
    return 0;
}

static int archive_name_is_valid(const char *archive_path)
{
    size_t length;

    length = strlen(archive_path);
    if (length < 5) {
        return 0;
    }

    return strcmp(archive_path + length - 4, ".sau") == 0;
}

static int size_field_is_valid(const char *size_field)
{
    int index;

    for (index = 0; index < INDEX_SIZE_FIELD; ++index) {
        if (size_field[index] < '0' || size_field[index] > '9') {
            return 0;
        }
    }

    return 1;
}

int extract_archive(const char *archive_path, const char *output_dir)
{
    FILE *input;
    char size_field[INDEX_SIZE_FIELD + 1];
    char *index;
    size_t index_size;
    ArchiveEntry *entries;
    int entry_count;
    int index_entry;
    int status;

    if (!archive_name_is_valid(archive_path)) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    input = fopen(archive_path, "rb");
    if (input == NULL) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    if (fread(size_field, 1, INDEX_SIZE_FIELD, input) != INDEX_SIZE_FIELD) {
        fclose(input);
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    size_field[INDEX_SIZE_FIELD] = '\0';
    if (!size_field_is_valid(size_field)) {
        fclose(input);
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    index_size = strtoul(size_field, NULL, 10);
    if (index_size == 0) {
        fclose(input);
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    index = malloc(index_size + 1);
    if (index == NULL) {
        fclose(input);
        return -1;
    }

    if (fread(index, 1, index_size, input) != index_size) {
        free(index);
        fclose(input);
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    index[index_size] = '\0';

    if (parse_index(index, index_size, &entries, &entry_count) != 0) {
        free(index);
        fclose(input);
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 0;
    }

    free(index);

    if (output_dir != NULL && strchr(output_dir, ' ') == NULL) {
        if (ensure_directory(output_dir) != 0) {
            free_entries(entries, entry_count);
            fclose(input);
            return -1;
        }
    }

    status = 0;
    for (index_entry = 0; index_entry < entry_count; ++index_entry) {
        char output_path[1024];
        FILE *output;
        size_t remaining;
        unsigned char buffer[4096];

        if (output_dir == NULL) {
            snprintf(output_path, sizeof(output_path), "%s", entries[index_entry].name);
        } else {
            snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, entries[index_entry].name);
        }

        output = fopen(output_path, "wb");
        if (output == NULL) {
            status = -1;
            break;
        }

        remaining = entries[index_entry].size;
        while (remaining > 0) {
            size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            size_t read_count = fread(buffer, 1, chunk, input);

            if (read_count != chunk) {
                fclose(output);
                status = -1;
                break;
            }

            if (fwrite(buffer, 1, read_count, output) != read_count) {
                fclose(output);
                status = -1;
                break;
            }

            remaining -= read_count;
        }

        if (status != 0) {
            break;
        }

        if (fclose(output) != 0) {
            status = -1;
            break;
        }

        if (chmod(output_path, mode_from_permissions(entries[index_entry].permissions)) != 0) {
            status = -1;
            break;
        }
    }

    free_entries(entries, entry_count);
    fclose(input);
    return status;
}

void free_entries(ArchiveEntry *entries, int count)
{
    int index_entry;

    if (entries == NULL) {
        return;
    }

    for (index_entry = 0; index_entry < count; ++index_entry) {
        free(entries[index_entry].name);
        free(entries[index_entry].permissions);
    }

    free(entries);
}
