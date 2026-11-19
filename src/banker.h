#ifndef BANKER_H
#define BANKER_H

#include <stdbool.h>

typedef struct {
    int customers;
    int resources;
    int *available;
    int *maximum;
    int *allocation;
    int *need;
    int *work;
    bool *finish;
} Banker;

typedef enum {
    REQUEST_GRANTED,
    REQUEST_EXCEEDS_MAXIMUM,
    REQUEST_NOT_ENOUGH,
    REQUEST_UNSAFE
} RequestResult;

typedef enum {
    RELEASE_DONE,
    RELEASE_EXCEEDS_ALLOCATION
} ReleaseResult;

bool banker_init(Banker *banker, int customers, int resources, const int *total, const int *maximum);
void banker_destroy(Banker *banker);
bool banker_is_safe(Banker *banker);
RequestResult banker_request(Banker *banker, int customer, const int *request);
ReleaseResult banker_release(Banker *banker, int customer, const int *release);
int *banker_row(const Banker *banker, int *matrix, int customer);

#endif
