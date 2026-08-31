/*
 * TinyDistro AppBuilder
 * tmakegen
 *
 * TinyDistro Makefile Generator
 *
 * Copyright (C) TinyDistro Project
 *
 * tmakegen reads tinydistro.conf and generates a Makefile
 * for TinyDistro applications.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#define TMAKEGEN_VERSION "2.3.0"

#define TG_MAX_LINE 4096
#define TG_MAX_NAME 256
#define TG_MAX_VALUE 4096
#define TG_MAX_ITEMS 1024
#define TG_MAX_RULES 1024
#define TG_MAX_DEFINES 512
#define TG_MAX_HEADERS 512
#define TG_MAX_BUILDS 1024
#define TG_MAX_PATH 4096

#define TG_SUCCESS 0
#define TG_ERROR 1
#define TG_USAGE 2
#define TG_PARSE_ERROR 3
#define TG_IO_ERROR 4
#define TG_MEMORY_ERROR 5

#define TG_DEFAULT_CONFIG "tinydistro.conf"
#define TG_DEFAULT_OUTPUT "Makefile"

typedef enum
{
    TG_LANG_UNKNOWN = 0,
    TG_LANG_C,
    TG_LANG_CPP
} tg_language_t;

typedef enum
{
    TG_DEFINE_UNKNOWN = 0,
    TG_DEFINE_CC,
    TG_DEFINE_CXX,
    TG_DEFINE_HEADER,
    TG_DEFINE_CFLAGS,
    TG_DEFINE_CXXFLAGS,
    TG_DEFINE_LDFLAGS,
    TG_DEFINE_LDLIBS,
    TG_DEFINE_TARGET,
    TG_DEFINE_OUTPUT,
    TG_DEFINE_SRC,
    TG_DEFINE_BUILD
} tg_define_type_t;

typedef enum
{
    TG_RULE_UNKNOWN = 0,
    TG_RULE_BUILD,
    TG_RULE_HEADER
} tg_rule_type_t;

typedef struct
{
    char name[TG_MAX_NAME];
    char value[TG_MAX_VALUE];
    tg_define_type_t type;
    unsigned long line;
} tg_define_t;

typedef struct
{
    char source[TG_MAX_PATH];
    char compiler[TG_MAX_NAME];
    char object[TG_MAX_PATH];
    tg_language_t language;
    unsigned long line;
} tg_build_rule_t;

typedef struct
{
    char header[TG_MAX_PATH];
    char dependency[TG_MAX_NAME];
    unsigned long line;
} tg_header_rule_t;

typedef struct
{
    tg_define_t defines[TG_MAX_DEFINES];
    size_t define_count;

    tg_build_rule_t builds[TG_MAX_BUILDS];
    size_t build_count;

    tg_header_rule_t headers[TG_MAX_HEADERS];
    size_t header_count;

    char target[TG_MAX_NAME];
    char output[TG_MAX_PATH];

    tg_language_t language;

    bool ended;
} tg_config_t;

typedef struct
{
    const char *config_file;
    const char *output_file;

    bool verbose;
    bool quiet;
    bool debug;
    bool dry_run;
    bool force_c;
    bool force_cpp;
    bool show_help;
    bool show_version;
    bool show_syntax;
    bool clean;

} tg_options_t;

static void tg_error(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "tmakegen: error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tg_warning(const char *format, ...)
{
    va_list args;

    fprintf(stderr, "tmakegen: warning: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tg_info(const tg_options_t *options, const char *format, ...)
{
    va_list args;

    if (options != NULL && options->quiet)
        return;

    printf("tmakegen: ");

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');
}

static void tg_debug(const tg_options_t *options, const char *format, ...)
{
    va_list args;

    if (options == NULL || !options->debug)
        return;

    fprintf(stderr, "tmakegen: debug: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

static void tg_config_init(tg_config_t *config)
{
    if (config == NULL)
        return;

    memset(config, 0, sizeof(*config));

    config->language = TG_LANG_UNKNOWN;

    strncpy(config->output,
            TG_DEFAULT_OUTPUT,
            sizeof(config->output) - 1);

    config->output[sizeof(config->output) - 1] = '\0';
}

static void tg_options_init(tg_options_t *options)
{
    if (options == NULL)
        return;

    memset(options, 0, sizeof(*options));

    options->config_file = TG_DEFAULT_CONFIG;
    options->output_file = TG_DEFAULT_OUTPUT;
}

static char *tg_ltrim(char *text)
{
    if (text == NULL)
        return NULL;

    while (*text != '\0' && isspace((unsigned char)*text))
        text++;

    return text;
}

static void tg_rtrim(char *text)
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

static char *tg_trim(char *text)
{
    text = tg_ltrim(text);
    tg_rtrim(text);
    return text;
}

static bool tg_starts_with(const char *text, const char *prefix)
{
    if (text == NULL || prefix == NULL)
        return false;

    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool tg_ends_with(const char *text, const char *suffix)
{
    size_t text_len;
    size_t suffix_len;

    if (text == NULL || suffix == NULL)
        return false;

    text_len = strlen(text);
    suffix_len = strlen(suffix);

    if (suffix_len > text_len)
        return false;

    return strcmp(text + text_len - suffix_len, suffix) == 0;
}

static bool tg_is_blank(const char *text)
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

static void tg_remove_comment(char *text)
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

static int tg_copy_string(char *destination,
                          size_t destination_size,
                          const char *source)
{
    size_t length;

    if (destination == NULL ||
        destination_size == 0 ||
        source == NULL)
    {
        return TG_ERROR;
    }

    length = strlen(source);

    if (length >= destination_size)
        return TG_ERROR;

    memcpy(destination, source, length + 1);

    return TG_SUCCESS;
}

static tg_define_type_t tg_define_type_from_name(const char *name)
{
    if (name == NULL)
        return TG_DEFINE_UNKNOWN;

    if (strcmp(name, "CC") == 0)
        return TG_DEFINE_CC;

    if (strcmp(name, "CXX") == 0)
        return TG_DEFINE_CXX;

    if (strcmp(name, "HD") == 0)
        return TG_DEFINE_HEADER;

    if (strcmp(name, "CFLAGS") == 0)
        return TG_DEFINE_CFLAGS;

    if (strcmp(name, "CXXFLAGS") == 0)
        return TG_DEFINE_CXXFLAGS;

    if (strcmp(name, "LDFLAGS") == 0)
        return TG_DEFINE_LDFLAGS;

    if (strcmp(name, "LDLIBS") == 0)
        return TG_DEFINE_LDLIBS;

    if (strcmp(name, "TARGET") == 0)
        return TG_DEFINE_TARGET;

    if (strcmp(name, "OUTPUT") == 0)
        return TG_DEFINE_OUTPUT;

    if (strcmp(name, "SRC") == 0)
        return TG_DEFINE_SRC;

    if (strcmp(name, "BUILD") == 0)
        return TG_DEFINE_BUILD;

    return TG_DEFINE_UNKNOWN;
}

static const char *tg_define_type_name(tg_define_type_t type)
{
    switch (type)
    {
        case TG_DEFINE_CC:
            return "CC";

        case TG_DEFINE_CXX:
            return "CXX";

        case TG_DEFINE_HEADER:
            return "HD";

        case TG_DEFINE_CFLAGS:
            return "CFLAGS";

        case TG_DEFINE_CXXFLAGS:
            return "CXXFLAGS";

        case TG_DEFINE_LDFLAGS:
            return "LDFLAGS";

        case TG_DEFINE_LDLIBS:
            return "LDLIBS";

        case TG_DEFINE_TARGET:
            return "TARGET";

        case TG_DEFINE_OUTPUT:
            return "OUTPUT";

        case TG_DEFINE_SRC:
            return "SRC";

        case TG_DEFINE_BUILD:
            return "BUILD";

        default:
            return "UNKNOWN";
    }
}

static int tg_add_define(tg_config_t *config,
                         const char *name,
                         const char *value,
                         unsigned long line)
{
    tg_define_t *define;
    tg_define_type_t type;

    if (config == NULL ||
        name == NULL ||
        value == NULL)
    {
        return TG_ERROR;
    }

    if (config->define_count >= TG_MAX_DEFINES)
    {
        tg_error("too many DEFINE entries");
        return TG_ERROR;
    }

    type = tg_define_type_from_name(name);

    define = &config->defines[config->define_count];

    if (tg_copy_string(define->name,
                       sizeof(define->name),
                       name) != TG_SUCCESS)
    {
        tg_error("DEFINE name is too long at line %lu", line);
        return TG_PARSE_ERROR;
    }

    if (tg_copy_string(define->value,
                       sizeof(define->value),
                       value) != TG_SUCCESS)
    {
        tg_error("DEFINE value is too long at line %lu", line);
        return TG_PARSE_ERROR;
    }

    define->type = type;
    define->line = line;

    config->define_count++;

    return TG_SUCCESS;
}

static const tg_define_t *tg_find_define(const tg_config_t *config,
                                         const char *name)
{
    size_t i;

    if (config == NULL || name == NULL)
        return NULL;

    for (i = 0; i < config->define_count; i++)
    {
        if (strcmp(config->defines[i].name, name) == 0)
            return &config->defines[i];
    }

    return NULL;
}

static const char *tg_get_define(const tg_config_t *config,
                                 const char *name,
                                 const char *fallback)
{
    const tg_define_t *define;

    define = tg_find_define(config, name);

    if (define == NULL)
        return fallback;

    return define->value;
}

static tg_language_t tg_detect_language(const char *filename)
{
    if (filename == NULL)
        return TG_LANG_UNKNOWN;

    if (tg_ends_with(filename, ".c"))
        return TG_LANG_C;

    if (tg_ends_with(filename, ".cc"))
        return TG_LANG_CPP;

    if (tg_ends_with(filename, ".cpp"))
        return TG_LANG_CPP;

    if (tg_ends_with(filename, ".cxx"))
        return TG_LANG_CPP;

    if (tg_ends_with(filename, ".C"))
        return TG_LANG_CPP;

    return TG_LANG_UNKNOWN;
}

static void tg_remove_newline(char *line)
{
    size_t length;

    if (line == NULL)
        return;

    length = strlen(line);

    while (length > 0 &&
           (line[length - 1] == '\n' ||
            line[length - 1] == '\r'))
    {
        line[length - 1] = '\0';
        length--;
    }
}

static int tg_parse_define(tg_config_t *config,
                           char *line,
                           unsigned long line_number)
{
    char *content;
    char *equals;
    char *name;
    char *value;

    content = line + strlen("DEFINE:");
    content = tg_trim(content);

    equals = strchr(content, '=');

    if (equals == NULL)
    {
        tg_error("invalid DEFINE syntax at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    *equals = '\0';

    name = tg_trim(content);
    value = tg_trim(equals + 1);

    if (*name == '\0')
    {
        tg_error("empty DEFINE name at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    if (*value == '\0')
    {
        tg_warning("empty DEFINE value for '%s' at line %lu",
                   name,
                   line_number);
    }

    return tg_add_define(config,
                          name,
                          value,
                          line_number);
}

static int tg_parse_build(tg_config_t *config,
                          char *line,
                          unsigned long line_number)
{
    char *content;
    char *dollar;
    char *source;
    char *compiler;
    tg_build_rule_t *rule;

    if (config->build_count >= TG_MAX_BUILDS)
    {
        tg_error("too many BUILD entries");
        return TG_ERROR;
    }

    content = line + strlen("BUILD:");
    content = tg_trim(content);

    dollar = strchr(content, '$');

    if (dollar != NULL)
    {
        *dollar = '\0';
        compiler = tg_trim(dollar + 1);
    }
    else
    {
        compiler = NULL;
    }

    source = tg_trim(content);

    if (*source == '\0')
    {
        tg_error("BUILD entry has no source at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    rule = &config->builds[config->build_count];

    memset(rule, 0, sizeof(*rule));

    if (tg_copy_string(rule->source,
                       sizeof(rule->source),
                       source) != TG_SUCCESS)
    {
        tg_error("source path too long at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    if (compiler != NULL && *compiler != '\0')
    {
        if (tg_copy_string(rule->compiler,
                           sizeof(rule->compiler),
                           compiler) != TG_SUCCESS)
        {
            tg_error("compiler name too long at line %lu",
                     line_number);
            return TG_PARSE_ERROR;
        }
    }
    else
    {
        tg_copy_string(rule->compiler,
                       sizeof(rule->compiler),
                       "CC");
    }

    rule->language = tg_detect_language(source);
    if (tg_copy_string(rule->object, sizeof(rule->object), source) != TG_SUCCESS) return TG_PARSE_ERROR;
    rule->line = line_number;

    config->build_count++;

    return TG_SUCCESS;
}

static int tg_parse_header(tg_config_t *config,
                           char *line,
                           unsigned long line_number)
{
    char *content;
    char *dollar;
    char *header;
    char *dependency;
    tg_header_rule_t *rule;

    if (config->header_count >= TG_MAX_HEADERS)
    {
        tg_error("too many HEADER entries");
        return TG_ERROR;
    }

    content = line + strlen("HEADER:");
    content = tg_trim(content);

    dollar = strchr(content, '$');

    if (dollar != NULL)
    {
        *dollar = '\0';
        dependency = tg_trim(dollar + 1);
    }
    else
    {
        dependency = NULL;
    }

    header = tg_trim(content);

    if (*header == '\0')
    {
        tg_error("HEADER entry has no header at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    rule = &config->headers[config->header_count];

    memset(rule, 0, sizeof(*rule));

    if (tg_copy_string(rule->header,
                       sizeof(rule->header),
                       header) != TG_SUCCESS)
    {
        tg_error("header path too long at line %lu",
                 line_number);
        return TG_PARSE_ERROR;
    }

    if (dependency != NULL && *dependency != '\0')
    {
        if (tg_copy_string(rule->dependency,
                           sizeof(rule->dependency),
                           dependency) != TG_SUCCESS)
        {
            tg_error("header dependency too long at line %lu",
                     line_number);
            return TG_PARSE_ERROR;
        }
    }

    rule->line = line_number;

    config->header_count++;

    return TG_SUCCESS;
}

static int tg_parse_line(tg_config_t *config,
                         char *line,
                         unsigned long line_number)
{
    char *text;

    tg_remove_newline(line);
    tg_remove_comment(line);

    text = tg_trim(line);

    if (*text == '\0')
        return TG_SUCCESS;

    if (strcmp(text, "!END.") == 0)
    {
        config->ended = true;
        return TG_SUCCESS;
    }

    if (tg_starts_with(text, "DEFINE:"))
        return tg_parse_define(config,
                               text,
                               line_number);

    if (tg_starts_with(text, "BUILD:"))
        return tg_parse_build(config,
                              text,
                              line_number);

    if (tg_starts_with(text, "HEADER:"))
        return tg_parse_header(config,
                               text,
                               line_number);

    tg_error("unknown directive at line %lu: %s",
             line_number,
             text);

    return TG_PARSE_ERROR;
}

static int tg_parse_file(tg_config_t *config,
                         const char *filename)
{
    FILE *file;
    char line[TG_MAX_LINE];
    unsigned long line_number = 0;
    int result;

    if (config == NULL || filename == NULL)
        return TG_ERROR;

    file = fopen(filename, "r");

    if (file == NULL)
    {
        tg_error("cannot open '%s': %s",
                 filename,
                 strerror(errno));
        return TG_IO_ERROR;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;

        if (strchr(line, '\n') == NULL &&
            !feof(file))
        {
            tg_error("line %lu is too long",
                     line_number);
            fclose(file);
            return TG_PARSE_ERROR;
        }

        result = tg_parse_line(config,
                               line,
                               line_number);

        if (result != TG_SUCCESS)
        {
            fclose(file);
            return result;
        }
    }

    if (ferror(file))
    {
        tg_error("error reading '%s'",
                 filename);
        fclose(file);
        return TG_IO_ERROR;
    }

    fclose(file);

    if (!config->ended)
    {
        tg_warning("configuration has no !END. directive");
    }

    return TG_SUCCESS;
}

static void tg_make_object_name(const char *source,
                                char *object,
                                size_t object_size)
{
    size_t i;
    size_t length;

    if (source == NULL ||
        object == NULL ||
        object_size == 0)
    {
        return;
    }

    length = strlen(source);

    if (length >= object_size)
        length = object_size - 1;

    memcpy(object, source, length);
    object[length] = '\0';

    for (i = 0; object[i] != '\0'; i++)
    {
        if (object[i] == '/')
            object[i] = '_';
    }

    if (tg_ends_with(object, ".cpp"))
    {
        object[strlen(object) - 4] = '\0';
    }
    else if (tg_ends_with(object, ".cxx"))
    {
        object[strlen(object) - 4] = '\0';
    }
    else if (tg_ends_with(object, ".cc"))
    {
        object[strlen(object) - 3] = '\0';
    }
    else if (tg_ends_with(object, ".c"))
    {
        object[strlen(object) - 2] = '\0';
    }

    if (strlen(object) + 3 < object_size)
        strcat(object, ".o");
}

static int tg_prepare_build_rules(tg_config_t *config)
{
    size_t i;

    if (config == NULL)
        return TG_ERROR;

    for (i = 0; i < config->build_count; i++)
    {
        tg_build_rule_t *rule = &config->builds[i];

        tg_make_object_name(rule->source,
                            rule->object,
                            sizeof(rule->object));

        if (rule->language == TG_LANG_UNKNOWN)
        {
            if (strcmp(rule->compiler, "CXX") == 0)
                rule->language = TG_LANG_CPP;
            else
                rule->language = TG_LANG_C;
        }
    }

    return TG_SUCCESS;
}

static int tg_validate_config(tg_config_t *config)
{
    size_t i;
    const tg_define_t *target_define;

    if (config == NULL)
        return TG_ERROR;

    if (config->build_count == 0)
    {
        tg_error("configuration contains no BUILD entries");
        return TG_PARSE_ERROR;
    }

    target_define = tg_find_define(config, "TARGET");

    if (target_define != NULL &&
        target_define->value[0] != '\0')
    {
        if (tg_copy_string(config->target,
                           sizeof(config->target),
                           target_define->value) != TG_SUCCESS)
        {
            tg_error("TARGET value is too long");
            return TG_PARSE_ERROR;
        }
    }
    else
    {
        tg_copy_string(config->target,
                       sizeof(config->target),
                       "app");
    }

    {
        const tg_define_t *output_define;

        output_define = tg_find_define(config, "OUTPUT");

        if (output_define != NULL &&
            output_define->value[0] != '\0')
        {
            if (tg_copy_string(config->output,
                               sizeof(config->output),
                               output_define->value) != TG_SUCCESS)
            {
                tg_error("OUTPUT value is too long");
                return TG_PARSE_ERROR;
            }
        }
    }

    for (i = 0; i < config->build_count; i++)
    {
        if (config->builds[i].source[0] == '\0')
        {
            tg_error("BUILD rule %zu has an empty source",
                     i + 1);
            return TG_PARSE_ERROR;
        }
    }

    return TG_SUCCESS;
}

static void tg_emit_header(FILE *file,
                           const tg_config_t *config)
{
    fprintf(file,
            "# TinyDistro AppBuilder generated Makefile\n"
            "# Generated by tmakegen %s\n"
            "# Do not edit this file manually.\n"
            "\n",
            TMAKEGEN_VERSION);

    fprintf(file,
            "TARGET := %s\n",
            config->target);

    fprintf(file,
            "\n");

    fprintf(file,
            "CC ?= gcc\n");

    fprintf(file,
            "CXX ?= g++\n");

    fprintf(file,
            "CFLAGS ?= -Wall -Wextra\n");

    fprintf(file,
            "CXXFLAGS ?= -Wall -Wextra\n");

    fprintf(file,
            "LDFLAGS ?=\n");

    fprintf(file,
            "LDLIBS ?=\n");

    fprintf(file,
            "\n");
}

static void tg_emit_sources(FILE *file,
                            const tg_config_t *config)
{
    size_t i;

    fprintf(file, "SOURCES := \\\n");

    for (i = 0; i < config->build_count; i++)
    {
        fprintf(file,
                "\t%s%s\n",
                config->builds[i].source,
                i + 1 == config->build_count ? "" : " \\");
    }

    fprintf(file, "\n");

    fprintf(file, "OBJECTS := \\\n");

    for (i = 0; i < config->build_count; i++)
    {
        fprintf(file,
                "\t%s%s\n",
                config->builds[i].object,
                i + 1 == config->build_count ? "" : " \\");
    }

    fprintf(file, "\n");
}

static void tg_emit_default_target(FILE *file,
                                   const tg_config_t *config)
{
    fprintf(file,
            ".PHONY: all clean\n"
            "\n");

    fprintf(file,
            "all: $(TARGET)\n"
            "\n");

    fprintf(file,
            "$(TARGET): $(OBJECTS)\n"
            "\t$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)\n"
            "\n");

    (void)config;
}

static void tg_emit_build_rule(FILE *file,
                               const tg_build_rule_t *rule)
{
    const char *compiler;

    if (rule == NULL)
        return;

    if (rule->language == TG_LANG_CPP)
        compiler = "$(CXX)";
    else
        compiler = "$(CC)";

    fprintf(file,
            "%s: %s",
            rule->object,
            rule->source);

    fputc('\n', file);

    if (rule->language == TG_LANG_CPP)
    {
        fprintf(file,
                "\t%s $(CXXFLAGS) -c $< -o $@\n",
                compiler);
    }
    else
    {
        fprintf(file,
                "\t%s $(CFLAGS) -c $< -o $@\n",
                compiler);
    }

    fprintf(file, "\n");
}

static void tg_emit_header_rules(FILE *file,
                                 const tg_config_t *config)
{
    size_t i;

    for (i = 0; i < config->header_count; i++)
    {
        const tg_header_rule_t *rule =
            &config->headers[i];

        if (rule->dependency[0] != '\0')
        {
            fprintf(file,
                    "$(OBJECTS): %s\n",
                    rule->header);
        }
        else
        {
            fprintf(file,
                    "$(OBJECTS): %s\n",
                    rule->header);
        }
    }

    if (config->header_count > 0)
        fprintf(file, "\n");
}

static void tg_emit_clean(FILE *file,
                          const tg_config_t *config)
{
    fprintf(file,
            "clean:\n"
            "\trm -f $(OBJECTS) $(TARGET)\n"
            "\n");

    (void)config;
}

static int tg_generate_makefile(const tg_config_t *config,
                                const char *filename)
{
    FILE *file;
    size_t i;

    if (config == NULL || filename == NULL)
        return TG_ERROR;

    file = fopen(filename, "w");

    if (file == NULL)
    {
        tg_error("cannot create '%s': %s",
                 filename,
                 strerror(errno));
        return TG_IO_ERROR;
    }

    tg_emit_header(file, config);
    tg_emit_sources(file, config);
    tg_emit_default_target(file, config);

    for (i = 0; i < config->build_count; i++)
        tg_emit_build_rule(file,
                           &config->builds[i]);

    tg_emit_header_rules(file, config);
    tg_emit_clean(file, config);

    if (fclose(file) != 0)
    {
        tg_error("cannot finalize '%s': %s",
                 filename,
                 strerror(errno));
        return TG_IO_ERROR;
    }

    return TG_SUCCESS;
}

static void tg_print_version(void)
{
    printf("tmakegen %s\n",
           TMAKEGEN_VERSION);

    printf("TinyDistro AppBuilder\n");
    printf("TinyDistro Makefile Generator\n");
}

static void tg_print_help(void)
{
    printf(
        "TinyDistro AppBuilder - tmakegen %s\n"
        "\n"
        "Usage:\n"
        "  tmakegen [options] [tinydistro.conf]\n"
        "\n"
        "Options:\n"
        "  -h, --help       Show this help message\n"
        "  -v, --version    Show version information\n"
        "  -o FILE          Write Makefile to FILE\n"
        "  -f FILE          Read configuration from FILE\n"
        "  -c               Force C mode\n"
        "  -cpp             Force C++ mode\n"
        "  -d, --debug      Enable parser debugging\n"
        "  -q, --quiet      Suppress informational output\n"
        "  -n, --dry-run    Generate without writing output\n"
        "  -s, --syntax     Show configuration syntax\n"
        "  --clean          Remove generated Makefile\n"
        "\n",
        TMAKEGEN_VERSION);
}

static void tg_print_syntax(void)
{
    printf(
        "TinyDistro configuration syntax\n"
        "\n"
        "DEFINE: NAME = VALUE\n"
        "BUILD: source.c $ CC\n"
        "HEADER: tinysdk.h $ HD\n"
        "!END.\n"
        "\n"
        "Example:\n"
        "\n"
        "DEFINE: CC = C\n"
        "DEFINE: HD = HEADER\n"
        "DEFINE: TARGET = hello\n"
        "\n"
        "BUILD: main.c $ CC\n"
        "HEADER: tinysdk.h $ HD\n"
        "\n"
        "!END.\n"
        "\n");
}

static int tg_parse_options(int argc,
                            char **argv,
                            tg_options_t *options)
{
    int i;

    if (options == NULL)
        return TG_ERROR;

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

        if (strcmp(arg, "-s") == 0 ||
            strcmp(arg, "--syntax") == 0)
        {
            options->show_syntax = true;
            continue;
        }

        if (strcmp(arg, "-c") == 0)
        {
            options->force_c = true;
            continue;
        }

        if (strcmp(arg, "-cpp") == 0)
        {
            options->force_cpp = true;
            continue;
        }

        if (strcmp(arg, "--clean") == 0)
        {
            options->clean = true;
            continue;
        }

        if (strcmp(arg, "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                tg_error("-o requires an argument");
                return TG_USAGE;
            }

            options->output_file = argv[++i];
            continue;
        }

        if (strcmp(arg, "-f") == 0)
        {
            if (i + 1 >= argc)
            {
                tg_error("-f requires an argument");
                return TG_USAGE;
            }

            options->config_file = argv[++i];
            continue;
        }

        if (arg[0] == '-')
        {
            tg_error("unknown option: %s",
                     arg);
            return TG_USAGE;
        }

        options->config_file = arg;
    }

    if (options->force_c &&
        options->force_cpp)
    {
        tg_error("-c and -cpp cannot be used together");
        return TG_USAGE;
    }

    return TG_SUCCESS;
}

static int tg_remove_output(const char *filename)
{
    if (filename == NULL)
        return TG_ERROR;

    if (remove(filename) != 0)
    {
        if (errno == ENOENT)
            return TG_SUCCESS;

        tg_error("cannot remove '%s': %s",
                 filename,
                 strerror(errno));

        return TG_IO_ERROR;
    }

    return TG_SUCCESS;
}

int main(int argc, char **argv)
{
    tg_options_t options;
    static tg_config_t config;
    int result;

    tg_options_init(&options);

    result = tg_parse_options(argc,
                              argv,
                              &options);

    if (result != TG_SUCCESS)
        return result;

    if (options.show_help)
    {
        tg_print_help();
        return TG_SUCCESS;
    }

    if (options.show_version)
    {
        tg_print_version();
        return TG_SUCCESS;
    }

    if (options.show_syntax)
    {
        tg_print_syntax();
        return TG_SUCCESS;
    }

    if (options.clean)
    {
        return tg_remove_output(options.output_file);
    }

    tg_config_init(&config);

    tg_debug(&options,
             "configuration file: %s",
             options.config_file);

    tg_debug(&options,
             "output file: %s",
             options.output_file);

    result = tg_parse_file(&config,
                           options.config_file);

    if (result != TG_SUCCESS)
        return result;

    result = tg_validate_config(&config);

    if (result != TG_SUCCESS)
        return result;

    result = tg_prepare_build_rules(&config);

    if (result != TG_SUCCESS)
        return result;

    if (options.force_c)
        config.language = TG_LANG_C;

    if (options.force_cpp)
        config.language = TG_LANG_CPP;

    tg_info(&options,
            "configuration parsed successfully");

    tg_info(&options,
            "build rules: %zu",
            config.build_count);

    tg_info(&options,
            "header rules: %zu",
            config.header_count);

    tg_info(&options,
            "target: %s",
            config.target);

    if (options.dry_run)
    {
        tg_info(&options,
                "dry-run enabled");

        return TG_SUCCESS;
    }

    result = tg_generate_makefile(&config,
                                  options.output_file);

    if (result != TG_SUCCESS)
        return result;

    tg_info(&options,
            "generated %s",
            options.output_file);

    return TG_SUCCESS;
}
