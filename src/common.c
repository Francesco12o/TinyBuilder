/*
 * TinyDistro AppBuilder
 * common.c
 *
 * Shared functionality for tmakegen and tsdkgen.
 *
 * TinyDistro v2.3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/common.h"

#define TDA_VERSION "2.3.0"
#define TDA_MAX_PATH 4096
#define TDA_MAX_LINE 4096

void tda_error(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "TinyDistro AppBuilder: error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

void tda_warning(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "TinyDistro AppBuilder: warning: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

void tda_info(const char *format, ...)
{
    va_list args;

    printf("TinyDistro AppBuilder: ");

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    fputc('\n', stdout);
}

void tda_debug(bool enabled, const char *format, ...)
{
    va_list args;

    if (!enabled)
        return;

    fprintf(stderr, "TinyDistro AppBuilder: debug: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

const char *tda_version(void)
{
    return TDA_VERSION;
}

char *tda_ltrim(char *text)
{
    if (text == NULL)
        return NULL;

    while (*text != '\0' &&
           isspace((unsigned char)*text))
    {
        text++;
    }

    return text;
}

void tda_rtrim(char *text)
{
    size_t length;

    if (text == NULL)
        return;

    length = strlen(text);

    while (length > 0 &&
           isspace((unsigned char)text[length - 1]))
    {
        text[length - 1] = '\0';
        length--;
    }
}

char *tda_trim(char *text)
{
    text = tda_ltrim(text);
    tda_rtrim(text);

    return text;
}

bool tda_is_blank(const char *text)
{
    if (text == NULL)
        return true;

    while (*text != '\0')
    {
        if (!isspace((unsigned char)*text))
            return false;

        text++;
    }

    return true;
}

bool tda_starts_with(const char *text,
                     const char *prefix)
{
    size_t length;

    if (text == NULL || prefix == NULL)
        return false;

    length = strlen(prefix);

    return strncmp(text, prefix, length) == 0;
}

bool tda_ends_with(const char *text,
                   const char *suffix)
{
    size_t text_length;
    size_t suffix_length;

    if (text == NULL || suffix == NULL)
        return false;

    text_length = strlen(text);
    suffix_length = strlen(suffix);

    if (suffix_length > text_length)
        return false;

    return strcmp(text + text_length - suffix_length,
                  suffix) == 0;
}

bool tda_equals_ignore_case(const char *a,
                            const char *b)
{
    if (a == NULL || b == NULL)
        return false;

    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) !=
            tolower((unsigned char)*b))
        {
            return false;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

int tda_copy(char *destination,
             size_t destination_size,
             const char *source)
{
    size_t length;

    if (destination == NULL ||
        destination_size == 0 ||
        source == NULL)
    {
        return -1;
    }

    length = strlen(source);

    if (length >= destination_size)
        return -1;

    memcpy(destination,
           source,
           length + 1);

    return 0;
}

void tda_remove_newline(char *text)
{
    size_t length;

    if (text == NULL)
        return;

    length = strlen(text);

    while (length > 0)
    {
        if (text[length - 1] != '\n' &&
            text[length - 1] != '\r')
        {
            break;
        }

        text[length - 1] = '\0';
        length--;
    }
}

void tda_remove_comment(char *text)
{
    bool quoted = false;
    bool escaped = false;

    if (text == NULL)
        return;

    while (*text != '\0')
    {
        if (escaped)
        {
            escaped = false;
            text++;
            continue;
        }

        if (*text == '\\')
        {
            escaped = true;
            text++;
            continue;
        }

        if (*text == '"')
        {
            quoted = !quoted;
            text++;
            continue;
        }

        if (*text == '#' && !quoted)
        {
            *text = '\0';
            return;
        }

        text++;
    }
}

bool tda_file_exists(const char *path)
{
    struct stat st;

    if (path == NULL)
        return false;

    return stat(path, &st) == 0;
}

bool tda_directory_exists(const char *path)
{
    struct stat st;

    if (path == NULL)
        return false;

    if (stat(path, &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}

long tda_file_size(const char *path)
{
    struct stat st;

    if (path == NULL)
        return -1;

    if (stat(path, &st) != 0)
        return -1;

    return (long)st.st_size;
}

int tda_make_directory(const char *path)
{
    if (path == NULL || *path == '\0')
        return -1;

    if (tda_directory_exists(path))
        return 0;

    if (mkdir(path, 0755) == 0)
        return 0;

    if (errno == EEXIST)
        return 0;

    return -1;
}

int tda_make_directory_recursive(const char *path)
{
    char buffer[TDA_MAX_PATH];
    char *cursor;

    if (path == NULL || *path == '\0')
        return -1;

    if (tda_copy(buffer,
                 sizeof(buffer),
                 path) != 0)
    {
        return -1;
    }

    for (cursor = buffer + 1;
         *cursor != '\0';
         cursor++)
    {
        if (*cursor == '/')
        {
            *cursor = '\0';

            if (*buffer != '\0' &&
                !tda_directory_exists(buffer))
            {
                if (tda_make_directory(buffer) != 0)
                    return -1;
            }

            *cursor = '/';
        }
    }

    if (!tda_directory_exists(buffer))
    {
        if (tda_make_directory(buffer) != 0)
            return -1;
    }

    return 0;
}

int tda_prepare_parent_directory(const char *filename)
{
    char buffer[TDA_MAX_PATH];
    char *slash;

    if (filename == NULL)
        return -1;

    if (tda_copy(buffer,
                 sizeof(buffer),
                 filename) != 0)
    {
        return -1;
    }

    slash = strrchr(buffer, '/');

    if (slash == NULL)
        return 0;

    *slash = '\0';

    if (*buffer == '\0')
        return 0;

    return tda_make_directory_recursive(buffer);
}

char *tda_read_file(const char *path,
                    size_t *size)
{
    FILE *file;
    char *buffer;
    long length;
    size_t read_size;

    if (size != NULL)
        *size = 0;

    if (path == NULL)
        return NULL;

    file = fopen(path, "rb");

    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    length = ftell(file);

    if (length < 0)
    {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)length + 1);

    if (buffer == NULL)
    {
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer,
                      1,
                      (size_t)length,
                      file);

    fclose(file);

    if (read_size != (size_t)length)
    {
        free(buffer);
        return NULL;
    }

    buffer[read_size] = '\0';

    if (size != NULL)
        *size = read_size;

    return buffer;
}

int tda_write_file(const char *path,
                   const void *data,
                   size_t size)
{
    FILE *file;
    size_t written;

    if (path == NULL || data == NULL)
        return -1;

    if (tda_prepare_parent_directory(path) != 0)
        return -1;

    file = fopen(path, "wb");

    if (file == NULL)
        return -1;

    written = fwrite(data,
                     1,
                     size,
                     file);

    if (fclose(file) != 0)
        return -1;

    if (written != size)
        return -1;

    return 0;
}

int tda_copy_file(const char *source,
                  const char *destination)
{
    FILE *input;
    FILE *output;
    unsigned char buffer[8192];
    size_t count;

    if (source == NULL ||
        destination == NULL)
    {
        return -1;
    }

    input = fopen(source, "rb");

    if (input == NULL)
        return -1;

    if (tda_prepare_parent_directory(destination) != 0)
    {
        fclose(input);
        return -1;
    }

    output = fopen(destination, "wb");

    if (output == NULL)
    {
        fclose(input);
        return -1;
    }

    while ((count = fread(buffer,
                          1,
                          sizeof(buffer),
                          input)) > 0)
    {
        if (fwrite(buffer,
                   1,
                   count,
                   output) != count)
        {
            fclose(input);
            fclose(output);
            return -1;
        }
    }

    if (ferror(input))
    {
        fclose(input);
        fclose(output);
        return -1;
    }

    if (fclose(input) != 0)
    {
        fclose(output);
        return -1;
    }

    if (fclose(output) != 0)
        return -1;

    return 0;
}

int tda_remove_file(const char *path)
{
    if (path == NULL)
        return -1;

    if (remove(path) == 0)
        return 0;

    if (errno == ENOENT)
        return 0;

    return -1;
}

const char *tda_basename(const char *path)
{
    const char *slash;

    if (path == NULL)
        return NULL;

    slash = strrchr(path, '/');

    if (slash == NULL)
        return path;

    return slash + 1;
}

bool tda_has_extension(const char *path,
                       const char *extension)
{
    if (path == NULL ||
        extension == NULL)
    {
        return false;
    }

    return tda_ends_with(path, extension);
}

bool tda_is_c_source(const char *path)
{
    return tda_has_extension(path, ".c");
}

bool tda_is_cpp_source(const char *path)
{
    if (path == NULL)
        return false;

    return tda_has_extension(path, ".cpp") ||
           tda_has_extension(path, ".cc") ||
           tda_has_extension(path, ".cxx") ||
           tda_has_extension(path, ".C");
}

bool tda_is_header(const char *path)
{
    if (path == NULL)
        return false;

    return tda_has_extension(path, ".h") ||
           tda_has_extension(path, ".hh") ||
           tda_has_extension(path, ".hpp") ||
           tda_has_extension(path, ".hxx");
}

const char *tda_language_name(const char *path)
{
    if (tda_is_c_source(path))
        return "C";

    if (tda_is_cpp_source(path))
        return "C++";

    if (tda_is_header(path))
        return "HEADER";

    return "UNKNOWN";
}

int tda_replace_extension(const char *path,
                          const char *extension,
                          char *output,
                          size_t output_size)
{
    const char *dot;
    size_t prefix_length;

    if (path == NULL ||
        extension == NULL ||
        output == NULL ||
        output_size == 0)
    {
        return -1;
    }

    dot = strrchr(path, '.');

    if (dot == NULL ||
        strchr(dot, '/') != NULL)
    {
        prefix_length = strlen(path);
    }
    else
    {
        prefix_length = (size_t)(dot - path);
    }

    if (prefix_length +
        strlen(extension) + 1 >
        output_size)
    {
        return -1;
    }

    memcpy(output,
           path,
           prefix_length);

    output[prefix_length] = '\0';

    strcat(output, extension);

    return 0;
}

int tda_join_path(const char *first,
                  const char *second,
                  char *output,
                  size_t output_size)
{
    size_t first_length;
    bool separator;

    if (first == NULL ||
        second == NULL ||
        output == NULL ||
        output_size == 0)
    {
        return -1;
    }

    first_length = strlen(first);

    separator = first_length > 0 &&
                first[first_length - 1] != '/';

    if (first_length +
        strlen(second) +
        (separator ? 1 : 0) +
        1 >
        output_size)
    {
        return -1;
    }

    strcpy(output, first);

    if (separator)
        strcat(output, "/");

    strcat(output, second);

    return 0;
}

int tda_read_lines(const char *path,
                   int (*callback)(const char *,
                                   unsigned long,
                                   void *),
                   void *userdata)
{
    FILE *file;
    char line[TDA_MAX_LINE];
    unsigned long line_number = 0;

    if (path == NULL ||
        callback == NULL)
    {
        return -1;
    }

    file = fopen(path, "r");

    if (file == NULL)
        return -1;

    while (fgets(line,
                 sizeof(line),
                 file) != NULL)
    {
        int result;

        line_number++;

        tda_remove_newline(line);

        result = callback(line,
                          line_number,
                          userdata);

        if (result != 0)
        {
            fclose(file);
            return result;
        }
    }

    if (ferror(file))
    {
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

char *tda_strdup(const char *text)
{
    size_t length;
    char *result;

    if (text == NULL)
        return NULL;

    length = strlen(text);

    result = malloc(length + 1);

    if (result == NULL)
        return NULL;

    memcpy(result,
           text,
           length + 1);

    return result;
}

bool tda_parse_bool(const char *text,
                    bool fallback)
{
    if (text == NULL)
        return fallback;

    if (tda_equals_ignore_case(text, "yes") ||
        tda_equals_ignore_case(text, "true") ||
        tda_equals_ignore_case(text, "on") ||
        strcmp(text, "1") == 0)
    {
        return true;
    }

    if (tda_equals_ignore_case(text, "no") ||
        tda_equals_ignore_case(text, "false") ||
        tda_equals_ignore_case(text, "off") ||
        strcmp(text, "0") == 0)
    {
        return false;
    }

    return fallback;
}

int tda_parse_integer(const char *text,
                      int fallback)
{
    char *end;
    long value;

    if (text == NULL)
        return fallback;

    errno = 0;

    value = strtol(text,
                   &end,
                   10);

    if (errno != 0 ||
        end == text ||
        *tda_trim(end) != '\0')
    {
        return fallback;
    }

    return (int)value;
}

void tda_print_banner(const char *tool)
{
    if (tool == NULL)
        tool = "AppBuilder";

    printf(
        "TinyDistro AppBuilder %s\n"
        "%s\n"
        "TinyDistro v2.3.0\n"
        "\n",
        TDA_VERSION,
        tool);
}

void tda_print_build_info(void)
{
    printf(
        "TinyDistro AppBuilder\n"
        "Version: %s\n"
        "Toolchain: C\n"
        "Project: TinyDistro v2.3.0\n",
        TDA_VERSION);
}
