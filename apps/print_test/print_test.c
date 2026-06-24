#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TIME_THRESHOLD_MS 200

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s <buffer_size_bytes>\n", program);
}

static int parse_buffer_size(const char *arg, size_t *buffer_size)
{
    char *endptr = NULL;
    unsigned long long value;

    errno = 0;
    value = strtoull(arg, &endptr, 10);
    if (errno != 0 || endptr == arg || *endptr != '\0' || value == 0) {
        return -1;
    }

    if (value > (unsigned long long)SIZE_MAX) {
        return -1;
    }

    *buffer_size = (size_t)value;
    return 0;
}

static long long get_current_timestamp_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(int argc, char **argv)
{
    size_t buffer_size;
    char *buffer;

    if (argc != 2 || parse_buffer_size(argv[1], &buffer_size) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    printf("------------ buffer_size : %zu \n", buffer_size);
    buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }

    memset(buffer, ' ', buffer_size - 1);
    buffer[buffer_size - 1] = '\0';

    while (1) {
        long long start_time;
        long long end_time;
        long long duration;

        start_time = get_current_timestamp_ms();

        printf("%s", buffer);

        end_time = get_current_timestamp_ms();
        duration = end_time - start_time;

        if (duration > TIME_THRESHOLD_MS) {
            printf("\n");
            printf(" [error: printf time  %lldms > 200ms]", duration);
        }

        printf("\n");
    }

    free(buffer);
    return 0;
}
