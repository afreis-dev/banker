#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "banker.h"
#include "error.h"
#include "input.h"
#include "output.h"

#define CUSTOMER_FILE "customer.txt"
#define COMMAND_FILE "commands.txt"
#define RESULT_FILE "result.txt"

static void warn_oversized_claims(const CustomerTable *customers, const int *total)
{
    for (int i = 0; i < customers->count; i++) {
        for (int j = 0; j < customers->resources; j++) {
            int claim = customers->maximum[(size_t)i * (size_t)customers->resources + (size_t)j];
            if (claim > total[j]) {
                report_warning("customer %d claims %d instances of resource %d, but only %d exist",
                               i, claim, j, total[j]);
            }
        }
    }
}

static void execute_request(Banker *banker, FILE *out, int customer, const int *amounts)
{
    int length = banker->resources;

    switch (banker_request(banker, customer, amounts)) {
    case REQUEST_GRANTED:
        output_granted(out, customer, amounts, length);
        break;
    case REQUEST_EXCEEDS_MAXIMUM:
        output_exceeds_maximum(out, customer, amounts, length);
        break;
    case REQUEST_NOT_ENOUGH:
        output_not_enough(out, banker, customer, amounts);
        break;
    case REQUEST_UNSAFE:
        output_unsafe(out, customer, amounts, length);
        break;
    }
}

static void execute_release(Banker *banker, FILE *out, int customer, const int *amounts)
{
    int length = banker->resources;

    switch (banker_release(banker, customer, amounts)) {
    case RELEASE_DONE:
        output_released(out, customer, amounts, length);
        break;
    case RELEASE_EXCEEDS_ALLOCATION:
        output_exceeds_allocation(out, customer, amounts, length);
        break;
    }
}

static void execute_all(Banker *banker, const CommandList *commands, FILE *out)
{
    for (size_t i = 0; i < commands->count; i++) {
        const Command *command = &commands->items[i];
        const int *amounts = command_amounts(commands, i);

        switch (command->type) {
        case COMMAND_REQUEST:
            execute_request(banker, out, command->customer, amounts);
            break;
        case COMMAND_RELEASE:
            execute_release(banker, out, command->customer, amounts);
            break;
        case COMMAND_STATUS:
            output_state(out, banker);
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int status = EXIT_FAILURE;
    int resources = 0;
    int *total = NULL;
    CustomerTable customers = {0};
    CommandList commands = {0};
    Banker banker = {0};
    bool banker_ready = false;
    FILE *out = NULL;

    if (!parse_resources(argc, argv, &resources, &total)) {
        goto cleanup;
    }
    if (!load_customers(CUSTOMER_FILE, resources, &customers)) {
        goto cleanup;
    }
    if (!load_commands(COMMAND_FILE, resources, customers.count, &commands)) {
        goto cleanup;
    }

    warn_oversized_claims(&customers, total);

    if (!banker_init(&banker, customers.count, resources, total, customers.maximum)) {
        report_error("out of memory");
        goto cleanup;
    }
    banker_ready = true;

    out = fopen(RESULT_FILE, "w");
    if (out == NULL) {
        report_error("cannot create '%s': %s", RESULT_FILE, strerror(errno));
        goto cleanup;
    }

    execute_all(&banker, &commands, out);

    if (ferror(out)) {
        report_error("cannot write '%s'", RESULT_FILE);
        goto cleanup;
    }
    if (fclose(out) != 0) {
        out = NULL;
        report_error("cannot write '%s': %s", RESULT_FILE, strerror(errno));
        goto cleanup;
    }
    out = NULL;

    status = EXIT_SUCCESS;

cleanup:
    if (out != NULL) {
        fclose(out);
    }
    if (banker_ready) {
        banker_destroy(&banker);
    }
    command_list_destroy(&commands);
    customer_table_destroy(&customers);
    free(total);
    return status;
}
