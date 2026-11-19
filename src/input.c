#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "error.h"
#include "input.h"

#define SEPARATORS " \t\r\n\v\f"

static bool parse_int(const char *begin, const char *end, int *value)
{
    while (begin < end && isspace((unsigned char)*begin)) {
        begin++;
    }
    while (end > begin && isspace((unsigned char)end[-1])) {
        end--;
    }
    if (begin == end) {
        return false;
    }

    long long accumulator = 0;
    for (const char *cursor = begin; cursor < end; cursor++) {
        if (!isdigit((unsigned char)*cursor)) {
            return false;
        }
        accumulator = accumulator * 10 + (*cursor - '0');
        if (accumulator > INT_MAX) {
            return false;
        }
    }

    *value = (int)accumulator;
    return true;
}

static bool parse_token(const char *token, int *value)
{
    return parse_int(token, token + strlen(token), value);
}

static bool is_blank(const char *line)
{
    for (; *line != '\0'; line++) {
        if (!isspace((unsigned char)*line)) {
            return false;
        }
    }
    return true;
}

static int *grow_matrix(int *matrix, size_t rows, size_t width)
{
    if (width == 0 || rows > SIZE_MAX / sizeof(int) / width) {
        return NULL;
    }
    return realloc(matrix, rows * width * sizeof(int));
}

static FILE *open_input(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        report_error("cannot open '%s': %s", path, strerror(errno));
    }
    return file;
}

bool parse_resources(int argc, char **argv, int *resources, int **total)
{
    if (argc < 2) {
        report_error("missing arguments; usage: %s <instances of resource 1> [<instances of resource 2> ...]",
                     argc > 0 ? argv[0] : "banker");
        return false;
    }

    int count = argc - 1;
    int *values = malloc((size_t)count * sizeof(int));
    if (values == NULL) {
        report_error("out of memory");
        return false;
    }

    for (int i = 0; i < count; i++) {
        if (!parse_token(argv[i + 1], &values[i])) {
            report_error("invalid argument '%s': expected a non-negative integer number of instances", argv[i + 1]);
            free(values);
            return false;
        }
    }

    *resources = count;
    *total = values;
    return true;
}

static bool parse_customer_line(const char *line, int resources, int *values, const char *path, long number)
{
    const char *start = line;
    int index = 0;

    for (;;) {
        const char *comma = strchr(start, ',');
        const char *end = comma != NULL ? comma : start + strlen(start);
        int value;

        if (index == resources) {
            report_error("%s:%ld: more than %d values on the line, but the program received %d resource arguments",
                         path, number, resources, resources);
            return false;
        }
        if (!parse_int(start, end, &value)) {
            report_error("%s:%ld: invalid value, expected non-negative integers separated by commas", path, number);
            return false;
        }

        values[index++] = value;
        if (comma == NULL) {
            break;
        }
        start = comma + 1;
    }

    if (index != resources) {
        report_error("%s:%ld: found %d values on the line, but the program received %d resource arguments",
                     path, number, index, resources);
        return false;
    }
    return true;
}

bool load_customers(const char *path, int resources, CustomerTable *table)
{
    FILE *file = open_input(path);
    if (file == NULL) {
        return false;
    }

    char *line = NULL;
    size_t line_size = 0;
    int *maximum = NULL;
    size_t capacity = 0;
    size_t rows = 0;
    long number = 0;
    long blank_line = 0;
    bool ok = false;

    while (getline(&line, &line_size, file) != -1) {
        number++;

        if (is_blank(line)) {
            if (blank_line == 0) {
                blank_line = number;
            }
            continue;
        }
        if (blank_line != 0) {
            report_error("%s:%ld: blank line between customers", path, blank_line);
            goto done;
        }

        if (rows == capacity) {
            size_t grown_capacity = capacity != 0 ? capacity * 2 : 8;
            int *grown = grow_matrix(maximum, grown_capacity, (size_t)resources);
            if (grown == NULL) {
                report_error("out of memory");
                goto done;
            }
            maximum = grown;
            capacity = grown_capacity;
        }

        if (!parse_customer_line(line, resources, maximum + rows * (size_t)resources, path, number)) {
            goto done;
        }
        rows++;
    }

    if (ferror(file)) {
        report_error("cannot read '%s': %s", path, strerror(errno));
        goto done;
    }
    if (rows == 0) {
        report_error("'%s' has no customers", path);
        goto done;
    }
    if (rows > INT_MAX) {
        report_error("'%s' has too many customers", path);
        goto done;
    }

    table->count = (int)rows;
    table->resources = resources;
    table->maximum = maximum;
    maximum = NULL;
    ok = true;

done:
    free(line);
    free(maximum);
    fclose(file);
    return ok;
}

static bool append_command(CommandList *list, CommandType type, int customer, const int *amounts)
{
    if (list->count == list->capacity) {
        size_t capacity = list->capacity != 0 ? list->capacity * 2 : 16;
        Command *items = realloc(list->items, capacity * sizeof(Command));
        if (items == NULL) {
            report_error("out of memory");
            return false;
        }
        list->items = items;

        int *pool = grow_matrix(list->amounts, capacity, (size_t)list->resources);
        if (pool == NULL) {
            report_error("out of memory");
            return false;
        }
        list->amounts = pool;
        list->capacity = capacity;
    }

    int *slot = list->amounts + list->count * (size_t)list->resources;
    if (amounts != NULL) {
        memcpy(slot, amounts, (size_t)list->resources * sizeof(int));
    } else {
        memset(slot, 0, (size_t)list->resources * sizeof(int));
    }

    list->items[list->count].type = type;
    list->items[list->count].customer = customer;
    list->count++;
    return true;
}

static bool parse_command_line(char *line, CommandList *list, int customers, int *amounts,
                               const char *path, long number)
{
    char *save = NULL;
    char *name = strtok_r(line, SEPARATORS, &save);
    if (name == NULL) {
        return true;
    }

    if (strcmp(name, "*") == 0) {
        if (strtok_r(NULL, SEPARATORS, &save) != NULL) {
            report_error("%s:%ld: unexpected text after '*'", path, number);
            return false;
        }
        return append_command(list, COMMAND_STATUS, 0, NULL);
    }

    CommandType type;
    if (strcmp(name, "RQ") == 0) {
        type = COMMAND_REQUEST;
    } else if (strcmp(name, "RL") == 0) {
        type = COMMAND_RELEASE;
    } else {
        report_error("%s:%ld: unknown command '%s', expected RQ, RL or *", path, number, name);
        return false;
    }

    int customer;
    char *token = strtok_r(NULL, SEPARATORS, &save);
    if (token == NULL || !parse_token(token, &customer)) {
        report_error("%s:%ld: expected a customer number after '%s'", path, number, name);
        return false;
    }
    if (customer >= customers) {
        report_error("%s:%ld: customer %d does not exist, customers are numbered from 0 to %d",
                     path, number, customer, customers - 1);
        return false;
    }

    for (int j = 0; j < list->resources; j++) {
        token = strtok_r(NULL, SEPARATORS, &save);
        if (token == NULL) {
            report_error("%s:%ld: expected %d resource amounts after the customer number, found %d",
                         path, number, list->resources, j);
            return false;
        }
        if (!parse_token(token, &amounts[j])) {
            report_error("%s:%ld: invalid amount '%s', expected a non-negative integer", path, number, token);
            return false;
        }
    }
    if (strtok_r(NULL, SEPARATORS, &save) != NULL) {
        report_error("%s:%ld: more than %d resource amounts on the line", path, number, list->resources);
        return false;
    }

    return append_command(list, type, customer, amounts);
}

bool load_commands(const char *path, int resources, int customers, CommandList *list)
{
    memset(list, 0, sizeof(*list));
    list->resources = resources;

    FILE *file = open_input(path);
    if (file == NULL) {
        return false;
    }

    int *amounts = malloc((size_t)resources * sizeof(int));
    char *line = NULL;
    size_t line_size = 0;
    long number = 0;
    bool ok = false;

    if (amounts == NULL) {
        report_error("out of memory");
        goto done;
    }

    while (getline(&line, &line_size, file) != -1) {
        number++;
        if (!parse_command_line(line, list, customers, amounts, path, number)) {
            goto done;
        }
    }

    if (ferror(file)) {
        report_error("cannot read '%s': %s", path, strerror(errno));
        goto done;
    }
    if (list->count == 0) {
        report_error("'%s' has no commands", path);
        goto done;
    }
    ok = true;

done:
    free(line);
    free(amounts);
    fclose(file);
    if (!ok) {
        command_list_destroy(list);
    }
    return ok;
}

const int *command_amounts(const CommandList *list, size_t index)
{
    return list->amounts + index * (size_t)list->resources;
}

void customer_table_destroy(CustomerTable *table)
{
    free(table->maximum);
    memset(table, 0, sizeof(*table));
}

void command_list_destroy(CommandList *list)
{
    free(list->items);
    free(list->amounts);
    memset(list, 0, sizeof(*list));
}
