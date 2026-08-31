/*
 * TinyDistro AppBuilder
 * tsdkgen
 *
 * TinyDistro SDK Generator
 *
 * Generates tinysdk.h for TinyDistro applications.
 *
 * TinyDistro v2.3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TSDKGEN_VERSION "2.3.0"

#define TSDK_MAX_PATH       4096
#define TSDK_MAX_LINE       4096
#define TSDK_MAX_NAME       256
#define TSDK_MAX_VALUE      4096
#define TSDK_MAX_SECTIONS   128
#define TSDK_MAX_SYMBOLS    512
#define TSDK_MAX_DEFINES    512
#define TSDK_MAX_ARGS       128

#define TSDK_DEFAULT_TEMPLATE "templates/tinysdk.h"
#define TSDK_DEFAULT_OUTPUT   "build/tinysdk.h"
#define TSDK_DEFAULT_CONFIG   "tinydistro.conf"

#define TSDK_OK             0
#define TSDK_ERROR          1
#define TSDK_USAGE          2
#define TSDK_IO_ERROR       3
#define TSDK_PARSE_ERROR    4
#define TSDK_MEMORY_ERROR   5

typedef enum
{
    TSDK_SECTION_NONE = 0,
    TSDK_SECTION_APPLICATION,
    TSDK_SECTION_CONSOLE,
    TSDK_SECTION_FILESYSTEM,
    TSDK_SECTION_PROCESS,
    TSDK_SECTION_MEMORY,
    TSDK_SECTION_SYSTEM,
    TSDK_SECTION_NETWORK,
    TSDK_SECTION_DEVICE,
    TSDK_SECTION_TIME,
    TSDK_SECTION_CUSTOM
} tsdk_section_type_t;

typedef struct
{
    char name[TSDK_MAX_NAME];
    char value[TSDK_MAX_VALUE];
    unsigned long line;
} tsdk_define_t;

typedef struct
{
    char name[TSDK_MAX_NAME];
    char return_type[TSDK_MAX_NAME];
    char arguments[TSDK_MAX_VALUE];
    char description[TSDK_MAX_VALUE];
    tsdk_section_type_t section;
    unsigned long line;
} tsdk_symbol_t;

typedef struct
{
    char name[TSDK_MAX_NAME];
    tsdk_section_type_t type;
    char description[TSDK_MAX_VALUE];
    bool enabled;
} tsdk_section_t;

typedef struct
{
    tsdk_define_t defines[TSDK_MAX_DEFINES];
    size_t define_count;

    tsdk_symbol_t symbols[TSDK_MAX_SYMBOLS];
    size_t symbol_count;

    tsdk_section_t sections[TSDK_MAX_SECTIONS];
    size_t section_count;

    char distro_name[TSDK_MAX_NAME];
    char distro_version[TSDK_MAX_NAME];
    char sdk_version[TSDK_MAX_NAME];

    bool include_application;
    bool include_console;
    bool include_filesystem;
    bool include_process;
    bool include_memory;
    bool include_system;
    bool include_network;
    bool include_device;
    bool include_time;

} tsdk_context_t;

typedef struct
{
    const char *template_file;
    const char *output_file;
    const char *config_file;

    bool verbose;
    bool quiet;
    bool debug;
    bool force;
    bool dry_run;
    bool show_help;
    bool show_version;
    bool show_symbols;
    bool show_sections;
    bool no_template;
} tsdk_options_t;

static void tsdk_error(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "tsdkgen: error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tsdk_warning(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "tsdkgen: warning: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tsdk_info(const tsdk_options_t *options,
                      const char *format,
                      ...)
{
    va_list args;

    if (options != NULL && options->quiet)
        return;

    printf("tsdkgen: ");

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');
}

static void tsdk_debug(const tsdk_options_t *options,
                       const char *format,
                       ...)
{
    va_list args;

    if (options == NULL || !options->debug)
        return;

    fprintf(stderr, "tsdkgen: debug: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tsdk_options_init(tsdk_options_t *options)
{
    if (options == NULL)
        return;

    memset(options, 0, sizeof(*options));

    options->template_file = TSDK_DEFAULT_TEMPLATE;
    options->output_file = TSDK_DEFAULT_OUTPUT;
    options->config_file = TSDK_DEFAULT_CONFIG;
}

static void tsdk_context_init(tsdk_context_t *context)
{
    if (context == NULL)
        return;

    memset(context, 0, sizeof(*context));

    strncpy(context->distro_name,
            "TinyDistro",
            sizeof(context->distro_name) - 1);

    strncpy(context->distro_version,
            "2.3.0",
            sizeof(context->distro_version) - 1);

    strncpy(context->sdk_version,
            TSDKGEN_VERSION,
            sizeof(context->sdk_version) - 1);

    context->include_application = true;
    context->include_console = true;
    context->include_filesystem = true;
    context->include_process = true;
    context->include_memory = true;
    context->include_system = true;
    context->include_network = true;
    context->include_device = true;
    context->include_time = true;
}

static char *tsdk_ltrim(char *text)
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

static void tsdk_rtrim(char *text)
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

static char *tsdk_trim(char *text)
{
    text = tsdk_ltrim(text);
    tsdk_rtrim(text);

    return text;
}

static bool tsdk_is_blank(const char *text)
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

static bool tsdk_starts_with(const char *text,
                             const char *prefix)
{
    size_t length;

    if (text == NULL || prefix == NULL)
        return false;

    length = strlen(prefix);

    return strncmp(text, prefix, length) == 0;
}

static bool tsdk_equals_ignore_case(const char *a,
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

static int tsdk_copy(char *destination,
                     size_t destination_size,
                     const char *source)
{
    size_t length;

    if (destination == NULL ||
        destination_size == 0 ||
        source == NULL)
    {
        return TSDK_ERROR;
    }

    length = strlen(source);

    if (length >= destination_size)
        return TSDK_ERROR;

    memcpy(destination,
           source,
           length + 1);

    return TSDK_OK;
}

static void tsdk_remove_comment(char *line)
{
    bool quote = false;
    bool escape = false;

    if (line == NULL)
        return;

    while (*line != '\0')
    {
        if (escape)
        {
            escape = false;
            line++;
            continue;
        }

        if (*line == '\\')
        {
            escape = true;
            line++;
            continue;
        }

        if (*line == '"')
        {
            quote = !quote;
            line++;
            continue;
        }

        if (*line == '#' && !quote)
        {
            *line = '\0';
            return;
        }

        line++;
    }
}

static void tsdk_remove_newline(char *line)
{
    size_t length;

    if (line == NULL)
        return;

    length = strlen(line);

    while (length > 0)
    {
        if (line[length - 1] != '\n' &&
            line[length - 1] != '\r')
        {
            break;
        }

        line[length - 1] = '\0';
        length--;
    }
}

static tsdk_section_type_t
tsdk_section_from_name(const char *name)
{
    if (name == NULL)
        return TSDK_SECTION_NONE;

    if (tsdk_equals_ignore_case(name, "APPLICATION"))
        return TSDK_SECTION_APPLICATION;

    if (tsdk_equals_ignore_case(name, "CONSOLE"))
        return TSDK_SECTION_CONSOLE;

    if (tsdk_equals_ignore_case(name, "FILESYSTEM"))
        return TSDK_SECTION_FILESYSTEM;

    if (tsdk_equals_ignore_case(name, "PROCESS"))
        return TSDK_SECTION_PROCESS;

    if (tsdk_equals_ignore_case(name, "MEMORY"))
        return TSDK_SECTION_MEMORY;

    if (tsdk_equals_ignore_case(name, "SYSTEM"))
        return TSDK_SECTION_SYSTEM;

    if (tsdk_equals_ignore_case(name, "NETWORK"))
        return TSDK_SECTION_NETWORK;

    if (tsdk_equals_ignore_case(name, "DEVICE"))
        return TSDK_SECTION_DEVICE;

    if (tsdk_equals_ignore_case(name, "TIME"))
        return TSDK_SECTION_TIME;

    return TSDK_SECTION_CUSTOM;
}

static const char *
tsdk_section_name(tsdk_section_type_t type)
{
    switch (type)
    {
        case TSDK_SECTION_APPLICATION:
            return "APPLICATION";

        case TSDK_SECTION_CONSOLE:
            return "CONSOLE";

        case TSDK_SECTION_FILESYSTEM:
            return "FILESYSTEM";

        case TSDK_SECTION_PROCESS:
            return "PROCESS";

        case TSDK_SECTION_MEMORY:
            return "MEMORY";

        case TSDK_SECTION_SYSTEM:
            return "SYSTEM";

        case TSDK_SECTION_NETWORK:
            return "NETWORK";

        case TSDK_SECTION_DEVICE:
            return "DEVICE";

        case TSDK_SECTION_TIME:
            return "TIME";

        case TSDK_SECTION_CUSTOM:
            return "CUSTOM";

        default:
            return "NONE";
    }
}

static int tsdk_add_define(tsdk_context_t *context,
                           const char *name,
                           const char *value,
                           unsigned long line)
{
    tsdk_define_t *define;

    if (context == NULL ||
        name == NULL ||
        value == NULL)
    {
        return TSDK_ERROR;
    }

    if (context->define_count >= TSDK_MAX_DEFINES)
    {
        tsdk_error("too many configuration definitions");
        return TSDK_MEMORY_ERROR;
    }

    define = &context->defines[context->define_count];

    memset(define, 0, sizeof(*define));

    if (tsdk_copy(define->name,
                  sizeof(define->name),
                  name) != TSDK_OK)
    {
        tsdk_error("definition name too long at line %lu",
                   line);
        return TSDK_PARSE_ERROR;
    }

    if (tsdk_copy(define->value,
                  sizeof(define->value),
                  value) != TSDK_OK)
    {
        tsdk_error("definition value too long at line %lu",
                   line);
        return TSDK_PARSE_ERROR;
    }

    define->line = line;

    context->define_count++;

    return TSDK_OK;
}

static const tsdk_define_t *
tsdk_find_define(const tsdk_context_t *context,
                 const char *name)
{
    size_t i;

    if (context == NULL || name == NULL)
        return NULL;

    for (i = 0; i < context->define_count; i++)
    {
        if (strcmp(context->defines[i].name,
                   name) == 0)
        {
            return &context->defines[i];
        }
    }

    return NULL;
}

static const char *
tsdk_get_define(const tsdk_context_t *context,
                const char *name,
                const char *fallback)
{
    const tsdk_define_t *define;

    define = tsdk_find_define(context, name);

    if (define == NULL)
        return fallback;

    return define->value;
}

static void tsdk_apply_define(tsdk_context_t *context,
                              const char *name,
                              const char *value)
{
    if (context == NULL ||
        name == NULL ||
        value == NULL)
    {
        return;
    }

    if (strcmp(name, "DISTRO") == 0)
    {
        tsdk_copy(context->distro_name,
                  sizeof(context->distro_name),
                  value);
        return;
    }

    if (strcmp(name, "DISTRO_VERSION") == 0)
    {
        tsdk_copy(context->distro_version,
                  sizeof(context->distro_version),
                  value);
        return;
    }

    if (strcmp(name, "SDK_VERSION") == 0)
    {
        tsdk_copy(context->sdk_version,
                  sizeof(context->sdk_version),
                  value);
        return;
    }

    if (strcmp(name, "SDK_APPLICATION") == 0)
    {
        context->include_application =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_CONSOLE") == 0)
    {
        context->include_console =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_FILESYSTEM") == 0)
    {
        context->include_filesystem =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_PROCESS") == 0)
    {
        context->include_process =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_MEMORY") == 0)
    {
        context->include_memory =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_SYSTEM") == 0)
    {
        context->include_system =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_NETWORK") == 0)
    {
        context->include_network =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_DEVICE") == 0)
    {
        context->include_device =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }

    if (strcmp(name, "SDK_TIME") == 0)
    {
        context->include_time =
            !tsdk_equals_ignore_case(value, "no") &&
            !tsdk_equals_ignore_case(value, "false") &&
            strcmp(value, "0") != 0;
        return;
    }
}

static int tsdk_parse_define(tsdk_context_t *context,
                             char *line,
                             unsigned long line_number)
{
    char *content;
    char *equals;
    char *name;
    char *value;
    int result;

    content = line + strlen("DEFINE:");
    content = tsdk_trim(content);

    equals = strchr(content, '=');

    if (equals == NULL)
    {
        tsdk_error("invalid DEFINE syntax at line %lu",
                   line_number);
        return TSDK_PARSE_ERROR;
    }

    *equals = '\0';

    name = tsdk_trim(content);
    value = tsdk_trim(equals + 1);

    if (*name == '\0')
    {
        tsdk_error("empty DEFINE name at line %lu",
                   line_number);
        return TSDK_PARSE_ERROR;
    }

    result = tsdk_add_define(context,
                             name,
                             value,
                             line_number);

    if (result != TSDK_OK)
        return result;

    tsdk_apply_define(context,
                      name,
                      value);

    return TSDK_OK;
}

static void tsdk_add_default_sections(tsdk_context_t *context)
{
    if (context == NULL)
        return;

    if (context->include_application)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Application");
        section->type = TSDK_SECTION_APPLICATION;
        section->enabled = true;
        strcpy(section->description,
               "TinyDistro application interface");
    }

    if (context->include_console)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Console");
        section->type = TSDK_SECTION_CONSOLE;
        section->enabled = true;
        strcpy(section->description,
               "Console input and output interface");
    }

    if (context->include_filesystem)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Filesystem");
        section->type = TSDK_SECTION_FILESYSTEM;
        section->enabled = true;
        strcpy(section->description,
               "TinyDistro filesystem interface");
    }

    if (context->include_process)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Process");
        section->type = TSDK_SECTION_PROCESS;
        section->enabled = true;
        strcpy(section->description,
               "Process and application control");
    }

    if (context->include_memory)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Memory");
        section->type = TSDK_SECTION_MEMORY;
        section->enabled = true;
        strcpy(section->description,
               "Memory management interface");
    }

    if (context->include_system)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "System");
        section->type = TSDK_SECTION_SYSTEM;
        section->enabled = true;
        strcpy(section->description,
               "TinyDistro system interface");
    }

    if (context->include_network)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Network");
        section->type = TSDK_SECTION_NETWORK;
        section->enabled = true;
        strcpy(section->description,
               "Network interface");
    }

    if (context->include_device)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Device");
        section->type = TSDK_SECTION_DEVICE;
        section->enabled = true;
        strcpy(section->description,
               "TinyDistro device interface");
    }

    if (context->include_time)
    {
        tsdk_section_t *section =
            &context->sections[context->section_count++];

        memset(section, 0, sizeof(*section));

        strcpy(section->name, "Time");
        section->type = TSDK_SECTION_TIME;
        section->enabled = true;
        strcpy(section->description,
               "Time and timing interface");
    }
}

static int tsdk_parse_config(tsdk_context_t *context,
                             const char *filename)
{
    FILE *file;
    char line[TSDK_MAX_LINE];
    unsigned long line_number = 0;
    int result;

    if (context == NULL || filename == NULL)
        return TSDK_ERROR;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        if (errno == ENOENT)
        {
            tsdk_warning("configuration '%s' not found",
                         filename);

            tsdk_add_default_sections(context);

            return TSDK_OK;
        }

        tsdk_error("cannot open '%s': %s",
                   filename,
                   strerror(errno));

        return TSDK_IO_ERROR;
    }

    while (fgets(line,
                 sizeof(line),
                 file) != NULL)
    {
        char *text;

        line_number++;

        tsdk_remove_newline(line);
        tsdk_remove_comment(line);

        text = tsdk_trim(line);

        if (tsdk_is_blank(text))
            continue;

        if (strcmp(text, "!END.") == 0)
            break;

        if (tsdk_starts_with(text, "DEFINE:"))
        {
            result = tsdk_parse_define(context,
                                       text,
                                       line_number);

            if (result != TSDK_OK)
            {
                fclose(file);
                return result;
            }

            continue;
        }

        tsdk_warning("unknown configuration directive at line %lu: %s",
                     line_number,
                     text);
    }

    if (ferror(file))
    {
        tsdk_error("error reading '%s'",
                   filename);

        fclose(file);

        return TSDK_IO_ERROR;
    }

    fclose(file);

    tsdk_add_default_sections(context);

    return TSDK_OK;
}

static int tsdk_add_symbol(tsdk_context_t *context,
                           const char *name,
                           const char *return_type,
                           const char *arguments,
                           const char *description,
                           tsdk_section_type_t section,
                           unsigned long line)
{
    tsdk_symbol_t *symbol;

    if (context == NULL ||
        name == NULL ||
        return_type == NULL ||
        arguments == NULL)
    {
        return TSDK_ERROR;
    }

    if (context->symbol_count >= TSDK_MAX_SYMBOLS)
        return TSDK_MEMORY_ERROR;

    symbol = &context->symbols[context->symbol_count];

    memset(symbol, 0, sizeof(*symbol));

    if (tsdk_copy(symbol->name,
                  sizeof(symbol->name),
                  name) != TSDK_OK)
        return TSDK_ERROR;

    if (tsdk_copy(symbol->return_type,
                  sizeof(symbol->return_type),
                  return_type) != TSDK_OK)
        return TSDK_ERROR;

    if (tsdk_copy(symbol->arguments,
                  sizeof(symbol->arguments),
                  arguments) != TSDK_OK)
        return TSDK_ERROR;

    if (description != NULL)
    {
        if (tsdk_copy(symbol->description,
                      sizeof(symbol->description),
                      description) != TSDK_OK)
            return TSDK_ERROR;
    }

    symbol->section = section;
    symbol->line = line;

    context->symbol_count++;

    return TSDK_OK;
}

static void tsdk_generate_symbols(tsdk_context_t *context)
{
    if (context == NULL)
        return;

    if (context->include_application)
    {
        tsdk_add_symbol(
            context,
            "tsdk_init",
            "int",
            "void",
            "Initialize the TinyDistro application environment",
            TSDK_SECTION_APPLICATION,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_shutdown",
            "void",
            "void",
            "Shut down the TinyDistro application environment",
            TSDK_SECTION_APPLICATION,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_version",
            "const char *",
            "void",
            "Return the TinyDistro SDK version",
            TSDK_SECTION_APPLICATION,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_app_name",
            "const char *",
            "void",
            "Return the application name",
            TSDK_SECTION_APPLICATION,
            0);
    }

    if (context->include_console)
    {
        tsdk_add_symbol(
            context,
            "tsdk_print",
            "int",
            "const char *message",
            "Write a message to the TinyDistro console",
            TSDK_SECTION_CONSOLE,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_puts",
            "int",
            "const char *message",
            "Write a line to the TinyDistro console",
            TSDK_SECTION_CONSOLE,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_getchar",
            "int",
            "void",
            "Read a character from the console",
            TSDK_SECTION_CONSOLE,
            0);
    }

    if (context->include_filesystem)
    {
        tsdk_add_symbol(
            context,
            "tsdk_file_exists",
            "int",
            "const char *path",
            "Check whether a filesystem path exists",
            TSDK_SECTION_FILESYSTEM,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_file_read",
            "long",
            "const char *path, void *buffer, unsigned long size",
            "Read data from a TinyDistro file",
            TSDK_SECTION_FILESYSTEM,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_file_write",
            "long",
            "const char *path, const void *buffer, unsigned long size",
            "Write data to a TinyDistro file",
            TSDK_SECTION_FILESYSTEM,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_file_remove",
            "int",
            "const char *path",
            "Remove a TinyDistro file",
            TSDK_SECTION_FILESYSTEM,
            0);
    }

    if (context->include_process)
    {
        tsdk_add_symbol(
            context,
            "tsdk_process_id",
            "int",
            "void",
            "Return the current process identifier",
            TSDK_SECTION_PROCESS,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_process_exit",
            "void",
            "int status",
            "Terminate the current application",
            TSDK_SECTION_PROCESS,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_process_name",
            "const char *",
            "void",
            "Return the current process name",
            TSDK_SECTION_PROCESS,
            0);
    }

    if (context->include_memory)
    {
        tsdk_add_symbol(
            context,
            "tsdk_malloc",
            "void *",
            "unsigned long size",
            "Allocate application memory",
            TSDK_SECTION_MEMORY,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_free",
            "void",
            "void *pointer",
            "Release application memory",
            TSDK_SECTION_MEMORY,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_meminfo",
            "unsigned long",
            "void",
            "Return available application memory",
            TSDK_SECTION_MEMORY,
            0);
    }

    if (context->include_system)
    {
        tsdk_add_symbol(
            context,
            "tsdk_system_name",
            "const char *",
            "void",
            "Return the TinyDistro system name",
            TSDK_SECTION_SYSTEM,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_system_version",
            "const char *",
            "void",
            "Return the TinyDistro system version",
            TSDK_SECTION_SYSTEM,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_uptime",
            "unsigned long",
            "void",
            "Return system uptime",
            TSDK_SECTION_SYSTEM,
            0);
    }

    if (context->include_network)
    {
        tsdk_add_symbol(
            context,
            "tsdk_network_available",
            "int",
            "void",
            "Check whether TinyDistro networking is available",
            TSDK_SECTION_NETWORK,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_hostname",
            "const char *",
            "void",
            "Return the system hostname",
            TSDK_SECTION_NETWORK,
            0);
    }

    if (context->include_device)
    {
        tsdk_add_symbol(
            context,
            "tsdk_device_exists",
            "int",
            "const char *name",
            "Check whether a device is available",
            TSDK_SECTION_DEVICE,
            0);
    }

    if (context->include_time)
    {
        tsdk_add_symbol(
            context,
            "tsdk_time",
            "unsigned long",
            "void",
            "Return the current TinyDistro time",
            TSDK_SECTION_TIME,
            0);

        tsdk_add_symbol(
            context,
            "tsdk_sleep",
            "void",
            "unsigned long milliseconds",
            "Sleep for the specified number of milliseconds",
            TSDK_SECTION_TIME,
            0);
    }
}

static int tsdk_mkdir_recursive(const char *path)
{
    char buffer[TSDK_MAX_PATH];
    char *cursor;

    if (path == NULL || *path == '\0')
        return TSDK_ERROR;

    if (tsdk_copy(buffer,
                  sizeof(buffer),
                  path) != TSDK_OK)
        return TSDK_ERROR;

    for (cursor = buffer + 1;
         *cursor != '\0';
         cursor++)
    {
        if (*cursor == '/')
        {
            struct stat st;

            *cursor = '\0';

            if (stat(buffer, &st) != 0)
            {
                if (mkdir(buffer, 0755) != 0 &&
                    errno != EEXIST)
                {
                    return TSDK_IO_ERROR;
                }
            }

            *cursor = '/';
        }
    }

    if (mkdir(buffer, 0755) != 0 &&
        errno != EEXIST)
    {
        if (errno != EEXIST)
            return TSDK_IO_ERROR;
    }

    return TSDK_OK;
}

static int tsdk_prepare_output_directory(const char *filename)
{
    char path[TSDK_MAX_PATH];
    char *slash;

    if (filename == NULL)
        return TSDK_ERROR;

    if (tsdk_copy(path,
                  sizeof(path),
                  filename) != TSDK_OK)
        return TSDK_ERROR;

    slash = strrchr(path, '/');

    if (slash == NULL)
        return TSDK_OK;

    *slash = '\0';

    if (*path == '\0')
        return TSDK_OK;

    return tsdk_mkdir_recursive(path);
}

static void tsdk_emit_guard_start(FILE *file)
{
    fprintf(file,
            "#ifndef TINYDISTRO_TINYSDK_H\n"
            "#define TINYDISTRO_TINYSDK_H\n"
            "\n");
}

static void tsdk_emit_guard_end(FILE *file)
{
    fprintf(file,
            "\n"
            "#endif\n");
}

static void tsdk_emit_metadata(FILE *file,
                               const tsdk_context_t *context)
{
    time_t now;
    struct tm *tm_info;
    char timestamp[64];

    timestamp[0] = '\0';

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info != NULL)
    {
        strftime(timestamp,
                 sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S",
                 tm_info);
    }

    fprintf(file,
            "/*\n"
            " * TinyDistro SDK\n"
            " *\n"
            " * Distribution: %s\n"
            " * Distribution version: %s\n"
            " * SDK version: %s\n"
            " * Generated: %s\n"
            " *\n"
            " * Generated by tsdkgen.\n"
            " * Do not edit generated sections manually.\n"
            " */\n"
            "\n",
            context->distro_name,
            context->distro_version,
            context->sdk_version,
            timestamp);
}

static void tsdk_emit_standard_headers(FILE *file)
{
    fprintf(file,
            "#include <stddef.h>\n"
            "#include <stdint.h>\n"
            "#include <stdbool.h>\n"
            "\n");
}

static void tsdk_emit_cplusplus_guard(FILE *file)
{
    fprintf(file,
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n"
            "\n");
}

static void tsdk_emit_cplusplus_guard_end(FILE *file)
{
    fprintf(file,
            "\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n");
}

static void tsdk_emit_constants(FILE *file,
                                const tsdk_context_t *context)
{
    fprintf(file,
            "#define TINYDISTRO_NAME \"%s\"\n"
            "#define TINYDISTRO_VERSION \"%s\"\n"
            "#define TINYSDK_VERSION \"%s\"\n"
            "\n",
            context->distro_name,
            context->distro_version,
            context->sdk_version);

    fprintf(file,
            "#define TSDK_SUCCESS 0\n"
            "#define TSDK_ERROR   -1\n"
            "\n");
}

static void tsdk_emit_section_comment(FILE *file,
                                      const char *name,
                                      const char *description)
{
    fprintf(file,
            "/* ============================================================\n"
            " * %s\n"
            " * %s\n"
            " * ============================================================ */\n"
            "\n",
            name,
            description);
}

static void tsdk_emit_symbol(FILE *file,
                             const tsdk_symbol_t *symbol)
{
    if (file == NULL || symbol == NULL)
        return;

    if (symbol->description[0] != '\0')
    {
        fprintf(file,
                "/* %s */\n",
                symbol->description);
    }

    fprintf(file,
            "%s %s(%s);\n"
            "\n",
            symbol->return_type,
            symbol->name,
            symbol->arguments);
}

static void tsdk_emit_application(FILE *file,
                                  const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_application)
        return;

    tsdk_emit_section_comment(
        file,
        "Application API",
        "TinyDistro application lifecycle and identity");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_APPLICATION)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_console(FILE *file,
                              const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_console)
        return;

    tsdk_emit_section_comment(
        file,
        "Console API",
        "TinyDistro console input and output");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_CONSOLE)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_filesystem(FILE *file,
                                 const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_filesystem)
        return;

    tsdk_emit_section_comment(
        file,
        "Filesystem API",
        "TinyDistro filesystem access");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_FILESYSTEM)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_process(FILE *file,
                              const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_process)
        return;

    tsdk_emit_section_comment(
        file,
        "Process API",
        "TinyDistro process management");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_PROCESS)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_memory(FILE *file,
                             const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_memory)
        return;

    tsdk_emit_section_comment(
        file,
        "Memory API",
        "TinyDistro application memory");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_MEMORY)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_system(FILE *file,
                             const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_system)
        return;

    tsdk_emit_section_comment(
        file,
        "System API",
        "TinyDistro system information");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_SYSTEM)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_network(FILE *file,
                              const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_network)
        return;

    tsdk_emit_section_comment(
        file,
        "Network API",
        "TinyDistro networking information");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_NETWORK)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_device(FILE *file,
                             const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_device)
        return;

    tsdk_emit_section_comment(
        file,
        "Device API",
        "TinyDistro device access");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_DEVICE)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static void tsdk_emit_time(FILE *file,
                           const tsdk_context_t *context)
{
    size_t i;

    if (!context->include_time)
        return;

    tsdk_emit_section_comment(
        file,
        "Time API",
        "TinyDistro time and timing functions");

    for (i = 0; i < context->symbol_count; i++)
    {
        if (context->symbols[i].section ==
            TSDK_SECTION_TIME)
        {
            tsdk_emit_symbol(file,
                             &context->symbols[i]);
        }
    }
}

static int tsdk_generate_header(const tsdk_context_t *context,
                                const tsdk_options_t *options)
{
    FILE *file;
    int result;

    if (context == NULL || options == NULL)
        return TSDK_ERROR;

    if (!options->dry_run)
    {
        result = tsdk_prepare_output_directory(
            options->output_file);

        if (result != TSDK_OK)
        {
            tsdk_error("cannot prepare output directory");
            return result;
        }

        file = fopen(options->output_file, "w");

        if (file == NULL)
        {
            tsdk_error("cannot create '%s': %s",
                       options->output_file,
                       strerror(errno));

            return TSDK_IO_ERROR;
        }
    }
    else
    {
        file = stdout;
    }

    tsdk_emit_guard_start(file);
    tsdk_emit_metadata(file, context);
    tsdk_emit_standard_headers(file);
    tsdk_emit_cplusplus_guard(file);
    tsdk_emit_constants(file, context);

    tsdk_emit_application(file, context);
    tsdk_emit_console(file, context);
    tsdk_emit_filesystem(file, context);
    tsdk_emit_process(file, context);
    tsdk_emit_memory(file, context);
    tsdk_emit_system(file, context);
    tsdk_emit_network(file, context);
    tsdk_emit_device(file, context);
    tsdk_emit_time(file, context);

    tsdk_emit_cplusplus_guard_end(file);
    tsdk_emit_guard_end(file);

    if (!options->dry_run)
    {
        if (fclose(file) != 0)
        {
            tsdk_error("cannot finalize '%s': %s",
                       options->output_file,
                       strerror(errno));

            return TSDK_IO_ERROR;
        }
    }

    return TSDK_OK;
}

static void tsdk_print_version(void)
{
    printf("tsdkgen %s\n",
           TSDKGEN_VERSION);

    printf("TinyDistro AppBuilder\n");
    printf("TinyDistro SDK Generator\n");
}

static void tsdk_print_help(void)
{
    printf(
        "TinyDistro AppBuilder - tsdkgen %s\n"
        "\n"
        "Usage:\n"
        "  tsdkgen [options]\n"
        "\n"
        "Options:\n"
        "  -h, --help       Show this help\n"
        "  -v, --version    Show version information\n"
        "  -f FILE          Configuration file\n"
        "  -t FILE          SDK template\n"
        "  -o FILE          Generated SDK output\n"
        "  -d, --debug      Enable debug output\n"
        "  -q, --quiet      Suppress informational output\n"
        "  -n, --dry-run    Print generated SDK to stdout\n"
        "  -F, --force      Overwrite output\n"
        "  --symbols        List generated symbols\n"
        "  --sections       List SDK sections\n"
        "  --no-template    Do not require a template\n"
        "\n"
        "Default configuration:\n"
        "  tinydistro.conf\n"
        "\n"
        "Default output:\n"
        "  build/tinysdk.h\n"
        "\n",
        TSDKGEN_VERSION);
}

static void tsdk_print_sections(void)
{
    printf(
        "TinySDK sections:\n"
        "\n"
        "  APPLICATION\n"
        "  CONSOLE\n"
        "  FILESYSTEM\n"
        "  PROCESS\n"
        "  MEMORY\n"
        "  SYSTEM\n"
        "  NETWORK\n"
        "  DEVICE\n"
        "  TIME\n"
        "\n");
}

static void tsdk_print_symbols(const tsdk_context_t *context)
{
    size_t i;

    if (context == NULL)
        return;

    printf("TinySDK symbols:\n\n");

    for (i = 0; i < context->symbol_count; i++)
    {
        const tsdk_symbol_t *symbol =
            &context->symbols[i];

        printf("  %-28s %s(%s)\n",
               symbol->name,
               symbol->return_type,
               symbol->arguments);
    }

    printf("\nTotal symbols: %zu\n",
           context->symbol_count);
}

static int tsdk_parse_options(int argc,
                              char **argv,
                              tsdk_options_t *options)
{
    int i;

    if (options == NULL)
        return TSDK_ERROR;

    for (i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 ||
            strcmp(arg, "--help") == 0)
        {
            options->show_help = true;
            continue;
        }

        if (strcmp(arg, "-v") == 0 ||
            strcmp(arg, "--version") == 0)
        {
            options->show_version = true;
            continue;
        }

        if (strcmp(arg, "-d") == 0 ||
            strcmp(arg, "--debug") == 0)
        {
            options->debug = true;
            continue;
        }

        if (strcmp(arg, "-q") == 0 ||
            strcmp(arg, "--quiet") == 0)
        {
            options->quiet = true;
            continue;
        }

        if (strcmp(arg, "-n") == 0 ||
            strcmp(arg, "--dry-run") == 0)
        {
            options->dry_run = true;
            continue;
        }

        if (strcmp(arg, "-F") == 0 ||
            strcmp(arg, "--force") == 0)
        {
            options->force = true;
            continue;
        }

        if (strcmp(arg, "--symbols") == 0)
        {
            options->show_symbols = true;
            continue;
        }

        if (strcmp(arg, "--sections") == 0)
        {
            options->show_sections = true;
            continue;
        }

        if (strcmp(arg, "--no-template") == 0)
        {
            options->no_template = true;
            continue;
        }

        if (strcmp(arg, "-f") == 0)
        {
            if (i + 1 >= argc)
            {
                tsdk_error("-f requires a file");
                return TSDK_USAGE;
            }

            options->config_file = argv[++i];
            continue;
        }

        if (strcmp(arg, "-t") == 0)
        {
            if (i + 1 >= argc)
            {
                tsdk_error("-t requires a file");
                return TSDK_USAGE;
            }

            options->template_file = argv[++i];
            continue;
        }

        if (strcmp(arg, "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                tsdk_error("-o requires a file");
                return TSDK_USAGE;
            }

            options->output_file = argv[++i];
            continue;
        }

        if (arg[0] == '-')
        {
            tsdk_error("unknown option: %s",
                       arg);

            return TSDK_USAGE;
        }

        options->config_file = arg;
    }

    return TSDK_OK;
}

static bool tsdk_file_exists(const char *filename)
{
    struct stat st;

    if (filename == NULL)
        return false;

    return stat(filename, &st) == 0;
}

static int tsdk_validate_template(const tsdk_options_t *options)
{
    if (options == NULL)
        return TSDK_ERROR;

    if (options->no_template)
        return TSDK_OK;

    if (!tsdk_file_exists(options->template_file))
    {
        tsdk_warning(
            "template '%s' does not exist; using built-in SDK generator",
            options->template_file);

        return TSDK_OK;
    }

    return TSDK_OK;
}

static void tsdk_debug_context(const tsdk_options_t *options,
                               const tsdk_context_t *context)
{
    size_t i;

    if (options == NULL ||
        context == NULL ||
        !options->debug)
    {
        return;
    }

    tsdk_debug(options,
               "distribution: %s",
               context->distro_name);

    tsdk_debug(options,
               "distribution version: %s",
               context->distro_version);

    tsdk_debug(options,
               "SDK version: %s",
               context->sdk_version);

    tsdk_debug(options,
               "definitions: %zu",
               context->define_count);

    for (i = 0; i < context->define_count; i++)
    {
        tsdk_debug(options,
                   "DEFINE %s = %s",
                   context->defines[i].name,
                   context->defines[i].value);
    }

    tsdk_debug(options,
               "sections: %zu",
               context->section_count);

    tsdk_debug(options,
               "symbols: %zu",
               context->symbol_count);
}

static int tsdk_validate_context(const tsdk_context_t *context)
{
    if (context == NULL)
        return TSDK_ERROR;

    if (context->distro_name[0] == '\0')
    {
        tsdk_error("distribution name is empty");
        return TSDK_PARSE_ERROR;
    }

    if (context->distro_version[0] == '\0')
    {
        tsdk_error("distribution version is empty");
        return TSDK_PARSE_ERROR;
    }

    if (context->sdk_version[0] == '\0')
    {
        tsdk_error("SDK version is empty");
        return TSDK_PARSE_ERROR;
    }

    if (context->section_count == 0)
    {
        tsdk_error("no SDK sections enabled");
        return TSDK_PARSE_ERROR;
    }

    return TSDK_OK;
}

int main(int argc, char **argv)
{
    tsdk_options_t options;
    tsdk_context_t context;
    int result;

    tsdk_options_init(&options);
    tsdk_context_init(&context);

    result = tsdk_parse_options(argc,
                                argv,
                                &options);

    if (result != TSDK_OK)
        return result;

    if (options.show_help)
    {
        tsdk_print_help();
        return TSDK_OK;
    }

    if (options.show_version)
    {
        tsdk_print_version();
        return TSDK_OK;
    }

    if (options.show_sections)
    {
        tsdk_print_sections();
        return TSDK_OK;
    }

    result = tsdk_validate_template(&options);

    if (result != TSDK_OK)
        return result;

    tsdk_debug(&options,
               "configuration: %s",
               options.config_file);

    tsdk_debug(&options,
               "template: %s",
               options.template_file);

    tsdk_debug(&options,
               "output: %s",
               options.output_file);

    result = tsdk_parse_config(&context,
                               options.config_file);

    if (result != TSDK_OK)
        return result;

    tsdk_generate_symbols(&context);

    result = tsdk_validate_context(&context);

    if (result != TSDK_OK)
        return result;

    tsdk_debug_context(&options,
                       &context);

    if (options.show_symbols)
    {
        tsdk_print_symbols(&context);
        return TSDK_OK;
    }

    tsdk_info(&options,
              "TinyDistro SDK Generator %s",
              TSDKGEN_VERSION);

    tsdk_info(&options,
              "distribution: %s %s",
              context.distro_name,
              context.distro_version);

    tsdk_info(&options,
              "generating %zu SDK symbols",
              context.symbol_count);

    result = tsdk_generate_header(&context,
                                  &options);

    if (result != TSDK_OK)
        return result;

    if (options.dry_run)
    {
        tsdk_info(&options,
                  "SDK generated in dry-run mode");

        return TSDK_OK;
    }

    tsdk_info(&options,
              "generated %s",
              options.output_file);

    return TSDK_OK;
}
