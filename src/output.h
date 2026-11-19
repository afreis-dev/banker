#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>

#include "banker.h"

void output_granted(FILE *out, int customer, const int *amounts, int length);
void output_released(FILE *out, int customer, const int *amounts, int length);
void output_exceeds_maximum(FILE *out, int customer, const int *request, int length);
void output_not_enough(FILE *out, const Banker *banker, int customer, const int *request);
void output_unsafe(FILE *out, int customer, const int *request, int length);
void output_exceeds_allocation(FILE *out, int customer, const int *amounts, int length);
void output_state(FILE *out, const Banker *banker);

#endif
