#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    COMMAND_REQUEST,
    COMMAND_RELEASE,
    COMMAND_STATUS
} CommandType;

typedef struct {
    CommandType type;
    int customer;
} Command;

typedef struct {
    Command *items;
    int *amounts;
    size_t count;
    size_t capacity;
    int resources;
} CommandList;

typedef struct {
    int count;
    int resources;
    int *maximum;
} CustomerTable;

bool parse_resources(int argc, char **argv, int *resources, int **total);
bool load_customers(const char *path, int resources, CustomerTable *table);
bool load_commands(const char *path, int resources, int customers, CommandList *list);
const int *command_amounts(const CommandList *list, size_t index);
void customer_table_destroy(CustomerTable *table);
void command_list_destroy(CommandList *list);

#endif
