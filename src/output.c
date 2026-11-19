#include <stdio.h>

#include "output.h"

#define MAXIMUM_LABEL "MAXIMUM"
#define ALLOCATION_LABEL "ALLOCATION"
#define MINIMUM_FIRST_COLUMN 10
#define MINIMUM_SECOND_COLUMN 12
#define FIRST_COLUMN_MARGIN 4
#define SECOND_COLUMN_MARGIN 6
#define COLUMN_SEPARATOR "|   "

static void write_joined(FILE *out, const int *values, int length)
{
    for (int j = 0; j < length; j++) {
        if (j > 0) {
            fputc(' ', out);
        }
        fprintf(out, "%d", values[j]);
    }
}

static void write_each(FILE *out, const int *values, int length)
{
    for (int j = 0; j < length; j++) {
        fprintf(out, "%d ", values[j]);
    }
}

static void write_padding(FILE *out, int count)
{
    for (int k = 0; k < count; k++) {
        fputc(' ', out);
    }
}

static int digits(int value)
{
    int count = 1;
    while (value >= 10) {
        value /= 10;
        count++;
    }
    return count;
}

static int each_length(const int *values, int length)
{
    int total = 0;
    for (int j = 0; j < length; j++) {
        total += digits(values[j]) + 1;
    }
    return total;
}

void output_granted(FILE *out, int customer, const int *amounts, int length)
{
    fprintf(out, "Allocate to customer %d the resources ", customer);
    write_joined(out, amounts, length);
    fputc('\n', out);
}

void output_released(FILE *out, int customer, const int *amounts, int length)
{
    fprintf(out, "Release from customer %d the resources ", customer);
    write_joined(out, amounts, length);
    fputc('\n', out);
}

void output_exceeds_maximum(FILE *out, int customer, const int *request, int length)
{
    fprintf(out, "The customer %d request ", customer);
    write_joined(out, request, length);
    fputs(" was denied because exceed its maximum allocation\n", out);
}

void output_not_enough(FILE *out, const Banker *banker, int customer, const int *request)
{
    fputs("The resources ", out);
    write_each(out, banker->available, banker->resources);
    fprintf(out, "are not enough to customer %d request ", customer);
    write_each(out, request, banker->resources);
    fputc('\n', out);
}

void output_unsafe(FILE *out, int customer, const int *request, int length)
{
    fprintf(out, "The customer %d request ", customer);
    write_joined(out, request, length);
    fputs(" was denied because result in an unsafe state\n", out);
}

void output_exceeds_allocation(FILE *out, int customer, const int *amounts, int length)
{
    fprintf(out, "The customer %d release ", customer);
    write_joined(out, amounts, length);
    fputs(" was denied because exceed its current allocation\n", out);
}

void output_state(FILE *out, const Banker *banker)
{
    int resources = banker->resources;
    int first = MINIMUM_FIRST_COLUMN;
    int second = MINIMUM_SECOND_COLUMN;

    for (int i = 0; i < banker->customers; i++) {
        int first_needed = each_length(banker_row(banker, banker->maximum, i), resources) + FIRST_COLUMN_MARGIN;
        int second_needed = each_length(banker_row(banker, banker->allocation, i), resources) + SECOND_COLUMN_MARGIN;
        if (first_needed > first) {
            first = first_needed;
        }
        if (second_needed > second) {
            second = second_needed;
        }
    }

    fputs(MAXIMUM_LABEL, out);
    write_padding(out, first - (int)sizeof(MAXIMUM_LABEL) + 1);
    fputs(COLUMN_SEPARATOR, out);
    fputs(ALLOCATION_LABEL, out);
    write_padding(out, second - (int)sizeof(ALLOCATION_LABEL) + 1);
    fputs(COLUMN_SEPARATOR, out);
    fputs("NEED\n", out);

    for (int i = 0; i < banker->customers; i++) {
        const int *maximum = banker_row(banker, banker->maximum, i);
        const int *allocation = banker_row(banker, banker->allocation, i);
        const int *need = banker_row(banker, banker->need, i);

        write_each(out, maximum, resources);
        write_padding(out, first - each_length(maximum, resources));
        fputs(COLUMN_SEPARATOR, out);
        write_each(out, allocation, resources);
        write_padding(out, second - each_length(allocation, resources));
        fputs(COLUMN_SEPARATOR, out);
        write_each(out, need, resources);
        fputc('\n', out);
    }

    fputs("AVAILABLE ", out);
    write_each(out, banker->available, resources);
    fputc('\n', out);
}
