#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "banker.h"

int *banker_row(const Banker *banker, int *matrix, int customer)
{
    return matrix + (size_t)customer * (size_t)banker->resources;
}

static bool vector_fits(const int *small, const int *large, int length)
{
    for (int j = 0; j < length; j++) {
        if (small[j] > large[j]) {
            return false;
        }
    }
    return true;
}

bool banker_init(Banker *banker, int customers, int resources, const int *total, const int *maximum)
{
    memset(banker, 0, sizeof(*banker));

    if (customers <= 0 || resources <= 0) {
        return false;
    }
    if ((size_t)customers > SIZE_MAX / sizeof(int) / (size_t)resources) {
        return false;
    }

    size_t cells = (size_t)customers * (size_t)resources;

    banker->customers = customers;
    banker->resources = resources;
    banker->available = malloc((size_t)resources * sizeof(int));
    banker->work = malloc((size_t)resources * sizeof(int));
    banker->maximum = malloc(cells * sizeof(int));
    banker->allocation = calloc(cells, sizeof(int));
    banker->need = malloc(cells * sizeof(int));
    banker->finish = malloc((size_t)customers * sizeof(bool));

    if (!banker->available || !banker->work || !banker->maximum ||
        !banker->allocation || !banker->need || !banker->finish) {
        banker_destroy(banker);
        return false;
    }

    memcpy(banker->available, total, (size_t)resources * sizeof(int));
    memcpy(banker->maximum, maximum, cells * sizeof(int));
    memcpy(banker->need, maximum, cells * sizeof(int));
    return true;
}

void banker_destroy(Banker *banker)
{
    free(banker->available);
    free(banker->work);
    free(banker->maximum);
    free(banker->allocation);
    free(banker->need);
    free(banker->finish);
    memset(banker, 0, sizeof(*banker));
}

bool banker_is_safe(Banker *banker)
{
    memcpy(banker->work, banker->available, (size_t)banker->resources * sizeof(int));
    for (int i = 0; i < banker->customers; i++) {
        banker->finish[i] = false;
    }

    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (int i = 0; i < banker->customers; i++) {
            if (banker->finish[i]) {
                continue;
            }
            if (!vector_fits(banker_row(banker, banker->need, i), banker->work, banker->resources)) {
                continue;
            }
            const int *held = banker_row(banker, banker->allocation, i);
            for (int j = 0; j < banker->resources; j++) {
                banker->work[j] += held[j];
            }
            banker->finish[i] = true;
            progressed = true;
        }
    }

    for (int i = 0; i < banker->customers; i++) {
        if (!banker->finish[i]) {
            return false;
        }
    }
    return true;
}

RequestResult banker_request(Banker *banker, int customer, const int *request)
{
    int *need = banker_row(banker, banker->need, customer);
    int *held = banker_row(banker, banker->allocation, customer);

    if (!vector_fits(request, need, banker->resources)) {
        return REQUEST_EXCEEDS_MAXIMUM;
    }
    if (!vector_fits(request, banker->available, banker->resources)) {
        return REQUEST_NOT_ENOUGH;
    }

    for (int j = 0; j < banker->resources; j++) {
        banker->available[j] -= request[j];
        held[j] += request[j];
        need[j] -= request[j];
    }

    if (banker_is_safe(banker)) {
        return REQUEST_GRANTED;
    }

    for (int j = 0; j < banker->resources; j++) {
        banker->available[j] += request[j];
        held[j] -= request[j];
        need[j] += request[j];
    }
    return REQUEST_UNSAFE;
}

ReleaseResult banker_release(Banker *banker, int customer, const int *release)
{
    int *need = banker_row(banker, banker->need, customer);
    int *held = banker_row(banker, banker->allocation, customer);

    if (!vector_fits(release, held, banker->resources)) {
        return RELEASE_EXCEEDS_ALLOCATION;
    }

    for (int j = 0; j < banker->resources; j++) {
        banker->available[j] += release[j];
        held[j] -= release[j];
        need[j] += release[j];
    }
    return RELEASE_DONE;
}
