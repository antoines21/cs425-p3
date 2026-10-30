#include "lab.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef TEST
#define main main_exclude
#endif

#define EXIT_OK 0
#define EXIT_BAD_INPUT 1
#define EXIT_BAD_TABLE 2
#define EXIT_NO_ROUTE 3

static void print_usage(FILE *out)
{
    fputs("Usage: myapp info <address>/<prefix>\n"
          "       myapp split <network>/<prefix> <count>\n"
          "       myapp lookup <table-file> <address>...\n",
          out);
}

static void print_address(const char *label, uint32_t addr)
{
    char text[IP_STR_LEN];

    ip_format(addr, text);
    printf("%-10s %s\n", label, text);
}

static int run_info(const char *text)
{
    cidr_t block;

    if (!cidr_parse(text, &block)) {
        fprintf(stderr, "myapp: not a valid <address>/<prefix>: %s\n", text);
        return EXIT_BAD_INPUT;
    }

    print_address("address", block.addr);
    printf("%-10s /%u\n", "prefix", (unsigned int) block.prefix);
    print_address("netmask", cidr_mask(block.prefix));
    print_address("network", cidr_network(block));
    print_address("broadcast", cidr_broadcast(block));
    print_address("first host", cidr_first_host(block));
    print_address("last host", cidr_last_host(block));
    printf("%-10s %" PRIu64 "\n", "hosts", cidr_host_count(block));
    return EXIT_OK;
}

// Same rules as the numbers in an address: decimal digits only, no sign and no
// leading zero. Fails if the value does not fit in a uint32_t.
static bool parse_count(const char *text, uint32_t *count)
{
    uint64_t value = 0;

    if (*text == '\0' || (text[0] == '0' && text[1] != '\0')) {
        return false;
    }

    for (const char *p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        value = value * 10 + (uint64_t) (*p - '0');
        if (value > UINT32_MAX) {
            return false;
        }
    }

    *count = (uint32_t) value;
    return true;
}

static int run_split(const char *text, const char *count_text)
{
    cidr_t block;
    cidr_t subnet;
    uint32_t count;

    if (!cidr_parse(text, &block)) {
        fprintf(stderr, "myapp: not a valid <network>/<prefix>: %s\n", text);
        return EXIT_BAD_INPUT;
    }
    if (!cidr_is_network(block)) {
        fprintf(stderr, "myapp: %s has host bits set, so it is not a network\n", text);
        return EXIT_BAD_INPUT;
    }
    if (!parse_count(count_text, &count)) {
        fprintf(stderr, "myapp: not a valid count: %s\n", count_text);
        return EXIT_BAD_INPUT;
    }
    if (count == 0 || (count & (count - 1)) != 0) {
        fprintf(stderr, "myapp: count is not a power of two: %s\n", count_text);
        return EXIT_BAD_INPUT;
    }

    // Checked before the loop so a refused split prints nothing to stdout
    if (!cidr_subnet(block, count, 0, &subnet)) {
        fprintf(stderr, "myapp: can not split %s into %s subnets, the prefix would pass 32\n",
                text, count_text);
        return EXIT_BAD_INPUT;
    }

    for (uint32_t i = 0; i < count; i++) {
        char out[CIDR_STR_LEN];

        cidr_subnet(block, count, i, &subnet);
        cidr_format(subnet, out);
        puts(out);
    }
    return EXIT_OK;
}

static bool load_table(fwd_table_t *table, const char *path)
{
    FILE *in = fopen(path, "r");
    size_t bad_line = 0;
    bool loaded;

    if (in == NULL) {
        fprintf(stderr, "myapp: %s: %s\n", path, strerror(errno));
        return false;
    }

    loaded = table_load(table, in, &bad_line);
    fclose(in);

    if (!loaded && bad_line == 0) {
        fprintf(stderr, "myapp: %s: could not be read\n", path);
    } else if (!loaded) {
        fprintf(stderr, "myapp: %s:%zu: not a valid route\n", path, bad_line);
    }
    return loaded;
}

static void print_route(const char *text, const route_t *route)
{
    if (route->gateway != 0) {
        char gateway[IP_STR_LEN];

        ip_format(route->gateway, gateway);
        printf("%s via %s dev %s\n", text, gateway, route->dev);
    } else {
        printf("%s dev %s\n", text, route->dev);
    }
}

static int run_lookup(const char *path, char **addresses, int count)
{
    fwd_table_t *table = table_create();
    bool malformed = false;
    bool unreachable = false;

    if (table == NULL) {
        fprintf(stderr, "myapp: out of memory\n");
        return EXIT_BAD_TABLE;
    }
    if (!load_table(table, path)) {
        table_destroy(table);
        return EXIT_BAD_TABLE;
    }

    // One bad address does not stop the rest from being reported
    for (int i = 0; i < count; i++) {
        uint32_t addr;
        const route_t *route;

        if (!ip_parse(addresses[i], &addr)) {
            fprintf(stderr, "myapp: not a valid address: %s\n", addresses[i]);
            malformed = true;
            continue;
        }

        route = table_lookup(table, addr);
        if (route != NULL) {
            print_route(addresses[i], route);
        } else {
            printf("%s unreachable\n", addresses[i]);
            unreachable = true;
        }
    }

    table_destroy(table);

    if (malformed) {
        return EXIT_BAD_INPUT;
    }
    return unreachable ? EXIT_NO_ROUTE : EXIT_OK;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        print_usage(stdout);
        return EXIT_OK;
    }

    if (strcmp(argv[1], "info") == 0 && argc == 3) {
        return run_info(argv[2]);
    }
    if (strcmp(argv[1], "split") == 0 && argc == 4) {
        return run_split(argv[2], argv[3]);
    }
    if (strcmp(argv[1], "lookup") == 0 && argc >= 4) {
        return run_lookup(argv[2], &argv[3], argc - 3);
    }

    fprintf(stderr, "myapp: bad command line: %s with %d argument(s)\n", argv[1], argc - 2);
    print_usage(stderr);
    return EXIT_BAD_INPUT;
}
