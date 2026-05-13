#include "tarsau.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_bundle(int argc, char **argv)
{
    char *output_path;
    char **inputs;
    int input_count;
    int index;
    int status;

    output_path = strdup(DEFAULT_ARCHIVE);
    inputs = calloc((size_t)(argc - 2), sizeof(char *));
    if (output_path == NULL || inputs == NULL) {
        free(output_path);
        free(inputs);
        return 1;
    }

    input_count = 0;
    for (index = 2; index < argc; ++index) {
        if (strcmp(argv[index], "-o") == 0) {
            if (index + 1 >= argc) {
                free(output_path);
                free(inputs);
                return 1;
            }

            free(output_path);
            output_path = strdup(argv[index + 1]);
            if (output_path == NULL) {
                free(inputs);
                return 1;
            }

            ++index;
            continue;
        }

        inputs[input_count++] = argv[index];
    }

    if (input_count == 0) {
        free(output_path);
        free(inputs);
        return 1;
    }

    if (input_count > MAX_INPUT_FILES) {
        free(output_path);
        free(inputs);
        return 1;
    }

    status = bundle_files(inputs, input_count, output_path);
    free(output_path);
    free(inputs);

    if (status < 0) {
        return 1;
    }

    return 0;
}

static int run_extract(int argc, char **argv)
{
    const char *archive_path;
    const char *output_dir;

    if (argc < 3 || argc > 4) {
        return 1;
    }

    archive_path = argv[2];
    output_dir = argc == 4 ? argv[3] : NULL;

    if (extract_archive(archive_path, output_dir) < 0) {
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        return run_bundle(argc, argv);
    }

    if (strcmp(argv[1], "-a") == 0) {
        return run_extract(argc, argv);
    }

    return 1;
}
