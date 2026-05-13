#include "tarsau.h"

#include <stdio.h>
#include <stdlib.h>

static int is_ascii_byte(unsigned char value)
{
    return value <= 0x7F;
}

int is_text_file(const char *path)
{
    FILE *file;
    unsigned char buffer[4096];
    size_t read_count;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    while ((read_count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        size_t index;

        for (index = 0; index < read_count; ++index) {
            if (!is_ascii_byte(buffer[index])) {
                fclose(file);
                return 0;
            }
        }
    }

    if (ferror(file)) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}
