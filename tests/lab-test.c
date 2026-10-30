#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"

#define SENTINEL 0xDEADBEEFu
#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

// The table from the guide, one route per line
static const char *const EXAMPLE_ROUTES[] = {
  "default            132.178.227.1    eno1",
  "132.178.227.0/25   direct           eno1",
  "10.0.0.0/8         10.1.0.1         eth1",
  "10.1.0.0/16        direct           eth1",
  "10.1.2.0/24        direct           eth2",
  "10.1.2.128/25      10.1.2.1         eth2",
  "192.168.0.0/16     10.1.0.1         eth1",
};

static const char EXAMPLE_FILE[] =
  "# destination      gateway          device\n"
  "default            132.178.227.1    eno1\n"
  "132.178.227.0/25   direct           eno1\n"
  "10.0.0.0/8         10.1.0.1         eth1\n"
  "10.1.0.0/16        direct           eth1\n"
  "10.1.2.0/24        direct           eth2\n"
  "10.1.2.128/25      10.1.2.1         eth2   # a router behind eth2\n"
  "192.168.0.0/16     10.1.0.1         eth1\n";

void setUp(void) {
}

void tearDown(void) {
}

static uint32_t ip(const char *text) {
  uint32_t addr = 0;
  TEST_ASSERT_TRUE_MESSAGE(ip_parse(text, &addr), text);
  return addr;
}

static cidr_t cidr(const char *text) {
  cidr_t block = {0, 0};
  TEST_ASSERT_TRUE_MESSAGE(cidr_parse(text, &block), text);
  return block;
}

static void assert_ip(const char *expected, uint32_t actual) {
  char out[IP_STR_LEN];
  ip_format(actual, out);
  TEST_ASSERT_EQUAL_STRING(expected, out);
}

static void assert_cidr(const char *expected, cidr_t actual) {
  char out[CIDR_STR_LEN];
  cidr_format(actual, out);
  TEST_ASSERT_EQUAL_STRING(expected, out);
}

static route_t route(const char *line) {
  route_t parsed;
  TEST_ASSERT_TRUE_MESSAGE(route_parse(line, &parsed), line);
  return parsed;
}

static void add_route(fwd_table_t *table, const char *line) {
  route_t parsed = route(line);
  TEST_ASSERT_TRUE_MESSAGE(table_add(table, &parsed), line);
}

// A real FILE * holding text, positioned at the start
static FILE *file_with(const char *text, size_t length) {
  FILE *file = tmpfile();
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_size_t(length, fwrite(text, 1, length, file));
  rewind(file);
  return file;
}

static bool load_text(fwd_table_t *table, const char *text, size_t *bad_line) {
  FILE *file = file_with(text, strlen(text));
  bool ok = table_load(table, file, bad_line);
  fclose(file);
  return ok;
}

static void assert_lookup(const fwd_table_t *table, const char *addr, const char *dest,
                          const char *gateway, const char *dev) {
  const route_t *found = table_lookup(table, ip(addr));
  TEST_ASSERT_NOT_NULL_MESSAGE(found, addr);
  assert_cidr(dest, found->dest);
  TEST_ASSERT_EQUAL_HEX32_MESSAGE(ip(gateway), found->gateway, addr);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(dev, found->dev, addr);
}

static void assert_example_lookups(const fwd_table_t *table) {
  assert_lookup(table, "132.178.227.50", "132.178.227.0/25", "0.0.0.0", "eno1");
  assert_lookup(table, "132.178.227.200", "0.0.0.0/0", "132.178.227.1", "eno1");
  assert_lookup(table, "8.8.8.8", "0.0.0.0/0", "132.178.227.1", "eno1");
  assert_lookup(table, "10.200.0.1", "10.0.0.0/8", "10.1.0.1", "eth1");
  assert_lookup(table, "10.1.200.1", "10.1.0.0/16", "0.0.0.0", "eth1");
  assert_lookup(table, "10.1.2.5", "10.1.2.0/24", "0.0.0.0", "eth2");
  assert_lookup(table, "10.1.2.127", "10.1.2.0/24", "0.0.0.0", "eth2");
  assert_lookup(table, "10.1.2.128", "10.1.2.128/25", "10.1.2.1", "eth2");
  assert_lookup(table, "10.1.2.200", "10.1.2.128/25", "10.1.2.1", "eth2");
  assert_lookup(table, "192.168.77.1", "192.168.0.0/16", "10.1.0.1", "eth1");
  assert_lookup(table, "0.0.0.0", "0.0.0.0/0", "132.178.227.1", "eno1");
  assert_lookup(table, "255.255.255.255", "0.0.0.0/0", "132.178.227.1", "eno1");
}

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

void test_ip_parse_valid(void) {
  uint32_t addr = SENTINEL;

  TEST_ASSERT_TRUE(ip_parse("192.168.1.10", &addr));
  TEST_ASSERT_EQUAL_HEX32(0xC0A8010A, addr);

  TEST_ASSERT_TRUE(ip_parse("0.0.0.0", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x00000000, addr);

  TEST_ASSERT_TRUE(ip_parse("255.255.255.255", &addr));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, addr);

  TEST_ASSERT_TRUE(ip_parse("1.2.3.4", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x01020304, addr);

  TEST_ASSERT_TRUE(ip_parse("100.20.3.0", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x64140300, addr);
}

void test_ip_parse_rejects_malformed(void) {
  static const char *const bad[] = {
    // wrong number of octets
    "", "1", "1.2", "1.2.3", "1.2.3.4.5",
    // misplaced periods
    ".", "...", "1..2.3", "1.2..3", ".1.2.3", ".1.2.3.4", "1.2.3.", "1.2.3.4.",
    // octet out of range
    "256.1.1.1", "1.256.1.1", "1.1.256.1", "1.1.1.256", "1000.1.1.1",
    "99999999999.1.1.1", "4294967297.1.1.1",
    // leading zeros
    "010.0.0.1", "0.010.0.1", "0.0.010.0", "0.0.0.010", "00.0.0.0", "0.0.0.00", "01.2.3.4",
    // something before, between or after the octets
    " 1.2.3.4", "\t1.2.3.4", "+1.2.3.4", "-1.2.3.4", "1.2.3.4x", "1.2.3.4 ", "1.2.3.4\n",
    "1. 2.3.4", "1 .2.3.4", "1,2,3,4", "1.2.3.4/24", "x1.2.3.4",
    // not decimal
    "a.b.c.d", "0x1.2.3.4", "1.2.3.0x4",
  };

  for (size_t i = 0; i < COUNT_OF(bad); i++) {
    uint32_t addr = SENTINEL;
    TEST_ASSERT_FALSE_MESSAGE(ip_parse(bad[i], &addr), bad[i]);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(SENTINEL, addr, bad[i]);
  }
}

void test_ip_parse_null(void) {
  uint32_t addr = SENTINEL;

  TEST_ASSERT_FALSE(ip_parse(NULL, &addr));
  TEST_ASSERT_EQUAL_HEX32(SENTINEL, addr);
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4", NULL));
}

void test_ip_format(void) {
  char out[IP_STR_LEN];

  ip_format(0xC0A8010A, out);
  TEST_ASSERT_EQUAL_STRING("192.168.1.10", out);

  ip_format(0x00000000, out);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0", out);

  ip_format(0xFFFFFFFF, out);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255", out);

  ip_format(0x01020304, out);
  TEST_ASSERT_EQUAL_STRING("1.2.3.4", out);
}

void test_ip_format_fits_the_longest_address_exactly(void) {
  char out[IP_STR_LEN + 1];

  memset(out, 'x', sizeof(out));
  ip_format(0xFFFFFFFF, out);
  TEST_ASSERT_EQUAL_INT('\0', out[IP_STR_LEN - 1]);
  TEST_ASSERT_EQUAL_INT('x', out[IP_STR_LEN]);
}

void test_cidr_parse_valid(void) {
  cidr_t block = {SENTINEL, 99};

  TEST_ASSERT_TRUE(cidr_parse("192.168.1.0/24", &block));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80100, block.addr);
  TEST_ASSERT_EQUAL_UINT8(24, block.prefix);

  TEST_ASSERT_TRUE(cidr_parse("0.0.0.0/0", &block));
  TEST_ASSERT_EQUAL_HEX32(0x00000000, block.addr);
  TEST_ASSERT_EQUAL_UINT8(0, block.prefix);

  TEST_ASSERT_TRUE(cidr_parse("255.255.255.255/32", &block));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, block.addr);
  TEST_ASSERT_EQUAL_UINT8(32, block.prefix);

  TEST_ASSERT_TRUE(cidr_parse("10.0.0.1/31", &block));
  TEST_ASSERT_EQUAL_UINT8(31, block.prefix);

  TEST_ASSERT_TRUE(cidr_parse("1.2.3.4/9", &block));
  TEST_ASSERT_EQUAL_UINT8(9, block.prefix);
}

void test_cidr_parse_keeps_host_bits(void) {
  cidr_t block = cidr("132.178.227.11/25");

  assert_ip("132.178.227.11", block.addr);
  TEST_ASSERT_EQUAL_UINT8(25, block.prefix);
}

void test_cidr_parse_rejects_malformed(void) {
  static const char *const bad[] = {
    // bad address
    "", "/24", "1.2.3/24", "1.2.3.4.5/24", "1..2.3/24", "256.1.1.1/24", "010.0.0.1/24",
    "1.2.3.4x/24", " 1.2.3.4/24",
    // missing slash or prefix
    "1.2.3.4", "1.2.3.4/",
    // prefix out of range
    "1.2.3.4/33", "1.2.3.4/100", "1.2.3.4/255", "1.2.3.4/256", "1.2.3.4/4294967320",
    "1.2.3.4/-1",
    // leading zeros in the prefix
    "1.2.3.4/024", "1.2.3.4/00", "1.2.3.4/01", "1.2.3.4/032",
    // something around the prefix
    "1.2.3.4/+1", "1.2.3.4/ 24", "1.2.3.4 /24", "1.2.3.4/24 ", "1.2.3.4/24\n", "1.2.3.4/24x",
    "1.2.3.4//24", "1.2.3.4/24/24", "1.2.3.4/2.4", "1.2.3.4/x",
  };

  for (size_t i = 0; i < COUNT_OF(bad); i++) {
    cidr_t block = {SENTINEL, 99};
    TEST_ASSERT_FALSE_MESSAGE(cidr_parse(bad[i], &block), bad[i]);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(SENTINEL, block.addr, bad[i]);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(99, block.prefix, bad[i]);
  }
}

void test_cidr_parse_null(void) {
  cidr_t block = {SENTINEL, 99};

  TEST_ASSERT_FALSE(cidr_parse(NULL, &block));
  TEST_ASSERT_EQUAL_HEX32(SENTINEL, block.addr);
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/24", NULL));
}

void test_cidr_format(void) {
  char out[CIDR_STR_LEN];

  cidr_format((cidr_t){0xC0A80100, 24}, out);
  TEST_ASSERT_EQUAL_STRING("192.168.1.0/24", out);

  cidr_format((cidr_t){0x00000000, 0}, out);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0/0", out);

  cidr_format((cidr_t){0xFFFFFFFF, 32}, out);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255/32", out);
}

void test_cidr_format_uses_the_address_as_stored(void) {
  assert_cidr("132.178.227.11/25", cidr("132.178.227.11/25"));
}

void test_cidr_format_writes_a_prefix_over_32_as_32(void) {
  char out[CIDR_STR_LEN];

  cidr_format((cidr_t){0xFFFFFFFF, 33}, out);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255/32", out);

  cidr_format((cidr_t){0xFFFFFFFF, 255}, out);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255/32", out);
}

void test_cidr_format_fits_the_longest_block_exactly(void) {
  char out[CIDR_STR_LEN + 1];

  memset(out, 'x', sizeof(out));
  cidr_format((cidr_t){0xFFFFFFFF, 32}, out);
  TEST_ASSERT_EQUAL_INT('\0', out[CIDR_STR_LEN - 1]);
  TEST_ASSERT_EQUAL_INT('x', out[CIDR_STR_LEN]);
}

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

void test_cidr_mask(void) {
  TEST_ASSERT_EQUAL_HEX32(0x00000000, cidr_mask(0));
  TEST_ASSERT_EQUAL_HEX32(0x80000000, cidr_mask(1));
  TEST_ASSERT_EQUAL_HEX32(0xFF000000, cidr_mask(8));
  TEST_ASSERT_EQUAL_HEX32(0xFFFF0000, cidr_mask(16));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF00, cidr_mask(24));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF80, cidr_mask(25));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFC, cidr_mask(30));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFE, cidr_mask(31));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(32));
}

void test_cidr_mask_treats_a_prefix_over_32_as_32(void) {
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(33));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(64));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(255));
}

void test_cidr_mask_every_prefix(void) {
  uint64_t expected = 0;

  // Build each mask one bit at a time instead of with the shift under test
  for (uint8_t prefix = 0; prefix <= 32; prefix++) {
    TEST_ASSERT_EQUAL_HEX32((uint32_t) expected, cidr_mask(prefix));
    expected = (expected >> 1) | 0x80000000u;
  }
}

void test_cidr_network(void) {
  assert_ip("132.178.227.0", cidr_network(cidr("132.178.227.11/25")));
  assert_ip("132.178.227.128", cidr_network(cidr("132.178.227.200/25")));
  assert_ip("0.0.0.0", cidr_network(cidr("255.255.255.255/0")));
  assert_ip("10.0.0.4", cidr_network(cidr("10.0.0.5/31")));
  assert_ip("10.0.0.5", cidr_network(cidr("10.0.0.5/32")));
  assert_ip("255.255.255.255", cidr_network(cidr("255.255.255.255/32")));
  assert_ip("0.0.0.0", cidr_network(cidr("0.0.0.0/32")));
}

void test_cidr_broadcast(void) {
  assert_ip("132.178.227.127", cidr_broadcast(cidr("132.178.227.11/25")));
  assert_ip("132.178.227.255", cidr_broadcast(cidr("132.178.227.200/25")));
  assert_ip("255.255.255.255", cidr_broadcast(cidr("0.0.0.0/0")));
  assert_ip("10.0.0.5", cidr_broadcast(cidr("10.0.0.4/31")));
  assert_ip("10.0.0.5", cidr_broadcast(cidr("10.0.0.5/32")));
  assert_ip("255.255.255.255", cidr_broadcast(cidr("255.255.255.255/32")));
  assert_ip("0.0.0.0", cidr_broadcast(cidr("0.0.0.0/32")));
}

void test_cidr_first_host(void) {
  assert_ip("132.178.227.1", cidr_first_host(cidr("132.178.227.11/25")));
  assert_ip("0.0.0.1", cidr_first_host(cidr("0.0.0.0/0")));
  assert_ip("0.0.0.1", cidr_first_host(cidr("200.1.1.1/0")));
  assert_ip("10.0.0.5", cidr_first_host(cidr("10.0.0.4/30")));
}

void test_cidr_first_host_of_31_and_32_is_the_network(void) {
  assert_ip("10.0.0.4", cidr_first_host(cidr("10.0.0.4/31")));
  assert_ip("10.0.0.4", cidr_first_host(cidr("10.0.0.5/31")));
  assert_ip("10.0.0.5", cidr_first_host(cidr("10.0.0.5/32")));
  assert_ip("255.255.255.254", cidr_first_host(cidr("255.255.255.255/31")));
  assert_ip("255.255.255.255", cidr_first_host(cidr("255.255.255.255/32")));
  assert_ip("0.0.0.0", cidr_first_host(cidr("0.0.0.0/32")));
}

void test_cidr_last_host(void) {
  assert_ip("132.178.227.126", cidr_last_host(cidr("132.178.227.11/25")));
  assert_ip("255.255.255.254", cidr_last_host(cidr("0.0.0.0/0")));
  assert_ip("10.0.0.6", cidr_last_host(cidr("10.0.0.4/30")));
}

void test_cidr_last_host_of_31_and_32_is_the_broadcast(void) {
  assert_ip("10.0.0.5", cidr_last_host(cidr("10.0.0.4/31")));
  assert_ip("10.0.0.5", cidr_last_host(cidr("10.0.0.5/31")));
  assert_ip("10.0.0.5", cidr_last_host(cidr("10.0.0.5/32")));
  assert_ip("0.0.0.1", cidr_last_host(cidr("0.0.0.0/31")));
  assert_ip("0.0.0.0", cidr_last_host(cidr("0.0.0.0/32")));
  assert_ip("255.255.255.255", cidr_last_host(cidr("255.255.255.255/32")));
}

void test_cidr_host_count(void) {
  TEST_ASSERT_EQUAL_UINT64(126, cidr_host_count(cidr("132.178.227.11/25")));
  TEST_ASSERT_EQUAL_UINT64(254, cidr_host_count(cidr("192.168.1.0/24")));
  TEST_ASSERT_EQUAL_UINT64(2, cidr_host_count(cidr("10.0.0.0/30")));
  TEST_ASSERT_EQUAL_UINT64(2147483646ULL, cidr_host_count(cidr("0.0.0.0/1")));
}

void test_cidr_host_count_of_0_needs_33_bits_on_the_way(void) {
  TEST_ASSERT_EQUAL_UINT64(4294967294ULL, cidr_host_count(cidr("0.0.0.0/0")));
}

void test_cidr_host_count_of_31_and_32(void) {
  TEST_ASSERT_EQUAL_UINT64(2, cidr_host_count(cidr("10.0.0.4/31")));
  TEST_ASSERT_EQUAL_UINT64(1, cidr_host_count(cidr("10.0.0.5/32")));
}

void test_cidr_contains(void) {
  cidr_t block = cidr("132.178.227.11/25");

  TEST_ASSERT_TRUE(cidr_contains(block, ip("132.178.227.50")));
  TEST_ASSERT_TRUE(cidr_contains(block, ip("132.178.227.0")));
  TEST_ASSERT_TRUE(cidr_contains(block, ip("132.178.227.127")));
  TEST_ASSERT_FALSE(cidr_contains(block, ip("132.178.227.128")));
  TEST_ASSERT_FALSE(cidr_contains(block, ip("132.178.227.200")));
  TEST_ASSERT_FALSE(cidr_contains(block, ip("132.178.226.255")));
}

void test_cidr_contains_boundary_prefixes(void) {
  cidr_t all = cidr("0.0.0.0/0");
  cidr_t link = cidr("10.0.0.4/31");
  cidr_t host = cidr("10.0.0.5/32");

  TEST_ASSERT_TRUE(cidr_contains(all, ip("0.0.0.0")));
  TEST_ASSERT_TRUE(cidr_contains(all, ip("8.8.8.8")));
  TEST_ASSERT_TRUE(cidr_contains(all, ip("255.255.255.255")));

  TEST_ASSERT_TRUE(cidr_contains(link, ip("10.0.0.4")));
  TEST_ASSERT_TRUE(cidr_contains(link, ip("10.0.0.5")));
  TEST_ASSERT_FALSE(cidr_contains(link, ip("10.0.0.3")));
  TEST_ASSERT_FALSE(cidr_contains(link, ip("10.0.0.6")));

  TEST_ASSERT_TRUE(cidr_contains(host, ip("10.0.0.5")));
  TEST_ASSERT_FALSE(cidr_contains(host, ip("10.0.0.4")));
  TEST_ASSERT_FALSE(cidr_contains(host, ip("10.0.0.6")));
}

void test_cidr_is_network(void) {
  TEST_ASSERT_TRUE(cidr_is_network(cidr("192.168.1.0/24")));
  TEST_ASSERT_FALSE(cidr_is_network(cidr("192.168.1.5/24")));
  TEST_ASSERT_TRUE(cidr_is_network(cidr("132.178.227.128/25")));
  TEST_ASSERT_FALSE(cidr_is_network(cidr("132.178.227.11/25")));

  TEST_ASSERT_TRUE(cidr_is_network(cidr("0.0.0.0/0")));
  TEST_ASSERT_FALSE(cidr_is_network(cidr("0.0.0.1/0")));
  TEST_ASSERT_FALSE(cidr_is_network(cidr("128.0.0.0/0")));
  TEST_ASSERT_TRUE(cidr_is_network(cidr("10.0.0.4/31")));
  TEST_ASSERT_FALSE(cidr_is_network(cidr("10.0.0.5/31")));
  TEST_ASSERT_TRUE(cidr_is_network(cidr("10.0.0.5/32")));
  TEST_ASSERT_TRUE(cidr_is_network(cidr("255.255.255.255/32")));
}

void test_cidr_subnet_splits_a_24_into_four(void) {
  static const char *const expected[] = {
    "192.168.1.0/26", "192.168.1.64/26", "192.168.1.128/26", "192.168.1.192/26",
  };
  cidr_t block = cidr("192.168.1.0/24");

  for (uint32_t i = 0; i < COUNT_OF(expected); i++) {
    cidr_t out;
    TEST_ASSERT_TRUE(cidr_subnet(block, 4, i, &out));
    assert_cidr(expected[i], out);
  }
}

void test_cidr_subnet_of_one_is_the_block(void) {
  cidr_t out;

  TEST_ASSERT_TRUE(cidr_subnet(cidr("192.168.1.0/24"), 1, 0, &out));
  assert_cidr("192.168.1.0/24", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("0.0.0.0/0"), 1, 0, &out));
  assert_cidr("0.0.0.0/0", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("255.255.255.255/32"), 1, 0, &out));
  assert_cidr("255.255.255.255/32", out);
}

void test_cidr_subnet_boundary_prefixes(void) {
  cidr_t out;

  TEST_ASSERT_TRUE(cidr_subnet(cidr("0.0.0.0/0"), 2, 0, &out));
  assert_cidr("0.0.0.0/1", out);
  TEST_ASSERT_TRUE(cidr_subnet(cidr("0.0.0.0/0"), 2, 1, &out));
  assert_cidr("128.0.0.0/1", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("10.0.0.4/30"), 4, 3, &out));
  assert_cidr("10.0.0.7/32", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("10.0.0.4/31"), 2, 1, &out));
  assert_cidr("10.0.0.5/32", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("255.255.255.254/31"), 2, 1, &out));
  assert_cidr("255.255.255.255/32", out);
}

void test_cidr_subnet_largest_legal_split(void) {
  uint32_t n = 0x80000000u;
  cidr_t out;

  TEST_ASSERT_TRUE(cidr_subnet(cidr("0.0.0.0/0"), n, n - 1, &out));
  assert_cidr("255.255.255.254/31", out);

  TEST_ASSERT_TRUE(cidr_subnet(cidr("128.0.0.0/1"), n, n - 1, &out));
  assert_cidr("255.255.255.255/32", out);
}

void test_cidr_subnet_rejects_a_prefix_longer_than_32(void) {
  cidr_t out = {SENTINEL, 99};

  TEST_ASSERT_FALSE(cidr_subnet(cidr("10.0.0.5/32"), 2, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(cidr("10.0.0.4/31"), 4, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(cidr("192.168.1.0/24"), 512, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(cidr("192.0.0.0/2"), 0x80000000u, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet((cidr_t){0, 33}, 1, 0, &out));

  TEST_ASSERT_EQUAL_HEX32(SENTINEL, out.addr);
  TEST_ASSERT_EQUAL_UINT8(99, out.prefix);
}

void test_cidr_subnet_rejects_bad_arguments(void) {
  cidr_t block = cidr("192.168.1.0/24");
  cidr_t out = {SENTINEL, 99};

  // host bits set
  TEST_ASSERT_FALSE(cidr_subnet(cidr("192.168.1.5/24"), 4, 0, &out));
  // n is not a power of two
  TEST_ASSERT_FALSE(cidr_subnet(block, 0, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(block, 3, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(block, 6, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(block, 0xFFFFFFFFu, 0, &out));
  // i out of range
  TEST_ASSERT_FALSE(cidr_subnet(block, 4, 4, &out));
  TEST_ASSERT_FALSE(cidr_subnet(block, 4, 0xFFFFFFFFu, &out));
  TEST_ASSERT_FALSE(cidr_subnet(block, 1, 1, &out));
  // out is NULL
  TEST_ASSERT_FALSE(cidr_subnet(block, 4, 0, NULL));

  TEST_ASSERT_EQUAL_HEX32(SENTINEL, out.addr);
  TEST_ASSERT_EQUAL_UINT8(99, out.prefix);
}

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

void test_route_parse_valid(void) {
  route_t parsed = route("10.0.0.0/8 10.1.0.1 eth1");

  assert_cidr("10.0.0.0/8", parsed.dest);
  assert_ip("10.1.0.1", parsed.gateway);
  TEST_ASSERT_EQUAL_STRING("eth1", parsed.dev);
}

void test_route_parse_default_and_direct(void) {
  route_t parsed = route("default 132.178.227.1 eno1");

  assert_cidr("0.0.0.0/0", parsed.dest);
  assert_ip("132.178.227.1", parsed.gateway);

  parsed = route("132.178.227.0/25 direct eno1");
  assert_cidr("132.178.227.0/25", parsed.dest);
  TEST_ASSERT_EQUAL_HEX32(0, parsed.gateway);

  parsed = route("default direct eth0");
  assert_cidr("0.0.0.0/0", parsed.dest);
  TEST_ASSERT_EQUAL_HEX32(0, parsed.gateway);

  parsed = route("0.0.0.0/0 0.0.0.0 eth0");
  assert_cidr("0.0.0.0/0", parsed.dest);
  TEST_ASSERT_EQUAL_HEX32(0, parsed.gateway);
}

void test_route_parse_boundary_blocks(void) {
  assert_cidr("255.255.255.255/32", route("255.255.255.255/32 255.255.255.255 eth0").dest);
  assert_cidr("10.0.0.4/31", route("10.0.0.4/31 direct eth0").dest);
}

void test_route_parse_ignores_surrounding_whitespace(void) {
  static const char *const lines[] = {
    "10.0.0.0/8 10.1.0.1 eth1\n",
    "10.0.0.0/8 10.1.0.1 eth1\r\n",
    "   10.0.0.0/8 10.1.0.1 eth1",
    "\t10.0.0.0/8 10.1.0.1 eth1",
    "10.0.0.0/8 10.1.0.1 eth1   ",
    "10.0.0.0/8 10.1.0.1 eth1\t\n",
    "  \t 10.0.0.0/8 10.1.0.1 eth1 \t \n",
    "10.0.0.0/8\t10.1.0.1\teth1",
    "10.0.0.0/8 \t  10.1.0.1 \t  eth1",
  };

  for (size_t i = 0; i < COUNT_OF(lines); i++) {
    route_t parsed = route(lines[i]);
    assert_cidr("10.0.0.0/8", parsed.dest);
    assert_ip("10.1.0.1", parsed.gateway);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("eth1", parsed.dev, lines[i]);
  }
}

void test_route_parse_device_length(void) {
  TEST_ASSERT_EQUAL_STRING("e", route("10.0.0.0/8 direct e").dev);
  TEST_ASSERT_EQUAL_STRING("eno12399np0", route("10.0.0.0/8 direct eno12399np0").dev);
  // DEV_NAME_LEN - 1 characters is the longest that fits
  TEST_ASSERT_EQUAL_STRING("abcdefghijklmno", route("10.0.0.0/8 direct abcdefghijklmno").dev);
}

void test_route_parse_rejects_malformed(void) {
  static const char *const bad[] = {
    // wrong number of fields
    "", " ", "\n", "10.0.0.0/8", "10.0.0.0/8 direct", "10.0.0.0/8 direct ",
    "10.0.0.0/8 direct eth0 extra", "default", "default direct",
    // bad destination
    "10.0.0.5/8 direct eth0", "10.0.0.0 direct eth0", "10.0.0.0/33 direct eth0",
    "010.0.0.0/8 direct eth0", "Default direct eth0", "DEFAULT direct eth0",
    "defaults direct eth0", "0.0.0.1/0 direct eth0", "/8 direct eth0",
    "255.255.255.255/255.255.255.255 direct eth0",
    // bad gateway
    "10.0.0.0/8 10.1.0 eth0", "10.0.0.0/8 256.0.0.1 eth0", "10.0.0.0/8 Direct eth0",
    "10.0.0.0/8 directly eth0", "10.0.0.0/8 010.1.0.1 eth0", "10.0.0.0/8 10.1.0.1/24 eth0",
    "10.0.0.0/8 255.255.255.255.255 eth0",
    // device too long
    "10.0.0.0/8 direct abcdefghijklmnop",
    "10.0.0.0/8 direct abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
    // only spaces and tabs separate fields
    "10.0.0.0/8\ndirect eth0", "10.0.0.0/8 direct\neth0", "10.0.0.0/8\rdirect eth0",
    "10.0.0.0/8 direct eth0\nextra",
    // comments are table_load()'s job
    "10.0.0.0/8 direct eth0 # comment", "10.0.0.0/8 direct eth0 #", "# comment",
  };

  for (size_t i = 0; i < COUNT_OF(bad); i++) {
    route_t parsed;
    route_t untouched;

    memset(&parsed, 0x5A, sizeof(parsed));
    memcpy(&untouched, &parsed, sizeof(parsed));
    TEST_ASSERT_FALSE_MESSAGE(route_parse(bad[i], &parsed), bad[i]);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&untouched, &parsed, sizeof(parsed), bad[i]);
  }
}

void test_route_parse_null(void) {
  route_t parsed;

  TEST_ASSERT_FALSE(route_parse(NULL, &parsed));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 direct eth0", NULL));
}

void test_table_create_is_empty(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_NOT_NULL(table);
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));
  TEST_ASSERT_NULL(table_lookup(table, ip("8.8.8.8")));
  table_destroy(table);
}

void test_table_destroy_null(void) {
  table_destroy(NULL);
}

void test_table_count(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_EQUAL_size_t(0, table_count(NULL));

  add_route(table, "10.0.0.0/8 direct eth0");
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));
  add_route(table, "default 10.0.0.1 eth0");
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  table_destroy(table);
}

void test_table_add_rejects_null(void) {
  fwd_table_t *table = table_create();
  route_t parsed = route("10.0.0.0/8 direct eth0");

  TEST_ASSERT_FALSE(table_add(NULL, &parsed));
  TEST_ASSERT_FALSE(table_add(table, NULL));
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  table_destroy(table);
}

void test_table_add_rejects_host_bits(void) {
  fwd_table_t *table = table_create();
  route_t parsed = route("10.0.0.0/8 direct eth0");

  parsed.dest.addr = ip("10.0.0.1");
  TEST_ASSERT_FALSE(table_add(table, &parsed));
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  table_destroy(table);
}

void test_table_add_rejects_a_second_route_to_the_same_block(void) {
  fwd_table_t *table = table_create();
  route_t second = route("10.0.0.0/8 10.9.9.9 eth9");

  add_route(table, "10.0.0.0/8 direct eth0");
  add_route(table, "default 10.0.0.1 eth0");

  TEST_ASSERT_FALSE(table_add(table, &second));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  // the first route is the one that stays
  assert_lookup(table, "10.1.1.1", "10.0.0.0/8", "0.0.0.0", "eth0");

  second = route("0.0.0.0/0 10.9.9.9 eth9");
  TEST_ASSERT_FALSE(table_add(table, &second));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  table_destroy(table);
}

void test_table_add_same_address_with_another_prefix_is_a_new_block(void) {
  fwd_table_t *table = table_create();

  add_route(table, "10.0.0.0/8 direct eth0");
  add_route(table, "10.0.0.0/9 direct eth1");
  add_route(table, "10.0.0.0/32 direct eth2");
  TEST_ASSERT_EQUAL_size_t(3, table_count(table));

  table_destroy(table);
}

void test_table_add_stores_a_copy(void) {
  fwd_table_t *table = table_create();
  route_t parsed = route("10.0.0.0/8 10.1.0.1 eth0");

  TEST_ASSERT_TRUE(table_add(table, &parsed));
  memset(&parsed, 0, sizeof(parsed));

  assert_lookup(table, "10.1.1.1", "10.0.0.0/8", "10.1.0.1", "eth0");

  table_destroy(table);
}

void test_table_add_grows(void) {
  enum { ROUTES = 1000 };
  fwd_table_t *table = table_create();

  for (uint32_t i = 0; i < ROUTES; i++) {
    route_t parsed = route("0.0.0.0/32 direct eth0");

    parsed.dest.addr = 0x0A000000u + i;
    snprintf(parsed.dev, DEV_NAME_LEN, "eth%u", (unsigned int) i);
    TEST_ASSERT_TRUE(table_add(table, &parsed));
    TEST_ASSERT_EQUAL_size_t(i + 1, table_count(table));
  }

  for (uint32_t i = 0; i < ROUTES; i++) {
    char dev[DEV_NAME_LEN];
    const route_t *found = table_lookup(table, 0x0A000000u + i);

    snprintf(dev, DEV_NAME_LEN, "eth%u", (unsigned int) i);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING(dev, found->dev);
  }

  table_destroy(table);
}

void test_table_load_example(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;

  TEST_ASSERT_TRUE(load_text(table, EXAMPLE_FILE, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);
  TEST_ASSERT_EQUAL_size_t(7, table_count(table));
  assert_example_lookups(table);

  table_destroy(table);
}

void test_table_load_skips_comments_and_blank_lines(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;
  const char *text =
    "\n"
    "# a comment\n"
    "   # an indented comment\n"
    "   \t  \n"
    "#10.0.0.0/8 direct eth0\n"
    "10.1.0.0/16 direct eth1# no space before the comment\n"
    "\r\n"
    "#\n";

  TEST_ASSERT_TRUE(load_text(table, text, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));
  assert_lookup(table, "10.1.1.1", "10.1.0.0/16", "0.0.0.0", "eth1");

  table_destroy(table);
}

void test_table_load_empty_file(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;

  TEST_ASSERT_TRUE(load_text(table, "", &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  table_destroy(table);
}

void test_table_load_last_line_without_a_newline(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_TRUE(load_text(table, "10.0.0.0/8 direct eth0\n10.1.0.0/16 direct eth1", NULL));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  table_destroy(table);
}

void test_table_load_crlf_line_endings(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_TRUE(load_text(table, "10.0.0.0/8 direct eth0\r\ndefault direct eth1\r\n", NULL));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));
  assert_lookup(table, "10.1.1.1", "10.0.0.0/8", "0.0.0.0", "eth0");

  table_destroy(table);
}

void test_table_load_a_line_longer_than_any_fixed_buffer(void) {
  enum { LENGTH = 100000 };
  fwd_table_t *table = table_create();
  const char *head = "10.0.0.0/8 direct eth0 # ";
  const char *tail = "\ndefault direct eth1\n";
  FILE *file = tmpfile();

  TEST_ASSERT_NOT_NULL(file);
  fputs(head, file);
  for (int i = 0; i < LENGTH; i++) {
    fputc('x', file);
  }
  fputs(tail, file);
  rewind(file);

  TEST_ASSERT_TRUE(table_load(table, file, NULL));
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  fclose(file);
  table_destroy(table);
}

void test_table_load_bad_line_in_the_middle(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;
  const char *text =
    "# header\n"
    "10.0.0.0/8 direct eth0\n"
    "\n"
    "not a route\n"
    "10.1.0.0/16 direct eth1\n"
    "also not a route\n";

  TEST_ASSERT_FALSE(load_text(table, text, &bad_line));
  TEST_ASSERT_EQUAL_size_t(4, bad_line);

  // the routes before the bad line stay, the ones after it never loaded
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));
  assert_lookup(table, "10.1.1.1", "10.0.0.0/8", "0.0.0.0", "eth0");

  table_destroy(table);
}

void test_table_load_bad_first_and_last_lines(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;

  TEST_ASSERT_FALSE(load_text(table, "junk\n10.0.0.0/8 direct eth0\n", &bad_line));
  TEST_ASSERT_EQUAL_size_t(1, bad_line);
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  TEST_ASSERT_FALSE(load_text(table, "10.0.0.0/8 direct eth0\njunk", &bad_line));
  TEST_ASSERT_EQUAL_size_t(2, bad_line);
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));

  table_destroy(table);
}

void test_table_load_bad_line_with_null_bad_line(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_FALSE(load_text(table, "10.0.0.0/8 direct eth0\njunk\n", NULL));
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));

  table_destroy(table);
}

void test_table_load_rejects_a_route_table_add_refuses(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;
  const char *text =
    "10.0.0.0/8 direct eth0\n"
    "10.1.0.0/16 direct eth1\n"
    "10.0.0.0/8 10.1.0.1 eth2\n";

  TEST_ASSERT_FALSE(load_text(table, text, &bad_line));
  TEST_ASSERT_EQUAL_size_t(3, bad_line);
  TEST_ASSERT_EQUAL_size_t(2, table_count(table));

  table_destroy(table);
}

void test_table_load_rejects_host_bits(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;

  TEST_ASSERT_FALSE(load_text(table, "132.178.227.11/25 direct eno1\n", &bad_line));
  TEST_ASSERT_EQUAL_size_t(1, bad_line);
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  table_destroy(table);
}

void test_table_load_rejects_a_nul_byte_in_a_line(void) {
  static const char text[] = "10.0.0.0/8 direct eth0\n10.1.0.0/16 direct eth1\0junk\n";
  fwd_table_t *table = table_create();
  FILE *file = file_with(text, sizeof(text) - 1);
  size_t bad_line = 99;

  TEST_ASSERT_FALSE(table_load(table, file, &bad_line));
  TEST_ASSERT_EQUAL_size_t(2, bad_line);
  TEST_ASSERT_EQUAL_size_t(1, table_count(table));

  fclose(file);
  table_destroy(table);
}

void test_table_load_null_arguments(void) {
  fwd_table_t *table = table_create();
  FILE *file = file_with(EXAMPLE_FILE, strlen(EXAMPLE_FILE));
  size_t bad_line = 99;

  TEST_ASSERT_FALSE(table_load(NULL, file, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);

  bad_line = 99;
  TEST_ASSERT_FALSE(table_load(table, NULL, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);

  TEST_ASSERT_FALSE(table_load(NULL, NULL, NULL));
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  fclose(file);
  table_destroy(table);
}

void test_table_load_file_that_can_not_be_read(void) {
  fwd_table_t *table = table_create();
  size_t bad_line = 99;
  FILE *write_only = fopen("/dev/null", "w");

  TEST_ASSERT_NOT_NULL(write_only);
  TEST_ASSERT_FALSE(table_load(table, write_only, &bad_line));
  TEST_ASSERT_EQUAL_size_t(0, bad_line);
  TEST_ASSERT_FALSE(table_load(table, write_only, NULL));
  TEST_ASSERT_EQUAL_size_t(0, table_count(table));

  fclose(write_only);
  table_destroy(table);
}

void test_table_lookup_several_match_and_the_longest_wins(void) {
  fwd_table_t *table = table_create();

  add_route(table, "10.0.0.0/8 10.0.0.1 eth8");
  add_route(table, "10.1.0.0/16 10.0.0.2 eth16");
  add_route(table, "10.1.2.0/24 10.0.0.3 eth24");
  add_route(table, "10.1.2.128/25 10.0.0.4 eth25");

  // inside all four
  assert_lookup(table, "10.1.2.200", "10.1.2.128/25", "10.0.0.4", "eth25");
  // one less specific each time
  assert_lookup(table, "10.1.2.100", "10.1.2.0/24", "10.0.0.3", "eth24");
  assert_lookup(table, "10.1.3.1", "10.1.0.0/16", "10.0.0.2", "eth16");
  assert_lookup(table, "10.2.0.1", "10.0.0.0/8", "10.0.0.1", "eth8");

  table_destroy(table);
}

void test_table_lookup_does_not_depend_on_insertion_order(void) {
  size_t count = COUNT_OF(EXAMPLE_ROUTES);

  // Every rotation of the table, forwards and backwards, so each route gets a
  // turn at the front and at the back
  for (size_t rotation = 0; rotation < count; rotation++) {
    fwd_table_t *forwards = table_create();
    fwd_table_t *backwards = table_create();

    for (size_t i = 0; i < count; i++) {
      add_route(forwards, EXAMPLE_ROUTES[(rotation + i) % count]);
      add_route(backwards, EXAMPLE_ROUTES[(rotation + count - i) % count]);
    }

    assert_example_lookups(forwards);
    assert_example_lookups(backwards);

    table_destroy(forwards);
    table_destroy(backwards);
  }
}

void test_table_lookup_default_loses_to_anything_more_specific(void) {
  fwd_table_t *table = table_create();

  add_route(table, "default 10.0.0.1 eth0");
  assert_lookup(table, "8.8.8.8", "0.0.0.0/0", "10.0.0.1", "eth0");
  assert_lookup(table, "0.0.0.0", "0.0.0.0/0", "10.0.0.1", "eth0");
  assert_lookup(table, "255.255.255.255", "0.0.0.0/0", "10.0.0.1", "eth0");

  add_route(table, "0.0.0.0/1 direct eth1");
  assert_lookup(table, "8.8.8.8", "0.0.0.0/1", "0.0.0.0", "eth1");
  assert_lookup(table, "200.0.0.1", "0.0.0.0/0", "10.0.0.1", "eth0");

  table_destroy(table);
}

void test_table_lookup_no_default_route(void) {
  fwd_table_t *table = table_create();

  add_route(table, "10.0.0.0/8 direct eth0");
  add_route(table, "192.168.0.0/16 direct eth1");

  TEST_ASSERT_NULL(table_lookup(table, ip("8.8.8.8")));
  TEST_ASSERT_NULL(table_lookup(table, ip("9.255.255.255")));
  TEST_ASSERT_NULL(table_lookup(table, ip("11.0.0.0")));
  TEST_ASSERT_NULL(table_lookup(table, ip("0.0.0.0")));
  TEST_ASSERT_NULL(table_lookup(table, ip("255.255.255.255")));
  TEST_ASSERT_NOT_NULL(table_lookup(table, ip("10.0.0.0")));
  TEST_ASSERT_NOT_NULL(table_lookup(table, ip("10.255.255.255")));

  table_destroy(table);
}

void test_table_lookup_31_and_32(void) {
  fwd_table_t *table = table_create();

  add_route(table, "10.0.0.0/24 direct eth0");
  add_route(table, "10.0.0.4/31 direct eth31");
  add_route(table, "10.0.0.5/32 direct eth32");

  assert_lookup(table, "10.0.0.3", "10.0.0.0/24", "0.0.0.0", "eth0");
  assert_lookup(table, "10.0.0.4", "10.0.0.4/31", "0.0.0.0", "eth31");
  assert_lookup(table, "10.0.0.5", "10.0.0.5/32", "0.0.0.0", "eth32");
  assert_lookup(table, "10.0.0.6", "10.0.0.0/24", "0.0.0.0", "eth0");

  table_destroy(table);
}

void test_table_lookup_edge_addresses_have_host_routes(void) {
  fwd_table_t *table = table_create();

  add_route(table, "default direct eth0");
  add_route(table, "0.0.0.0/32 direct eth1");
  add_route(table, "255.255.255.255/32 direct eth2");

  assert_lookup(table, "0.0.0.0", "0.0.0.0/32", "0.0.0.0", "eth1");
  assert_lookup(table, "255.255.255.255", "255.255.255.255/32", "0.0.0.0", "eth2");
  assert_lookup(table, "0.0.0.1", "0.0.0.0/0", "0.0.0.0", "eth0");

  table_destroy(table);
}

void test_table_lookup_null_and_empty(void) {
  fwd_table_t *table = table_create();

  TEST_ASSERT_NULL(table_lookup(NULL, ip("8.8.8.8")));
  TEST_ASSERT_NULL(table_lookup(table, ip("8.8.8.8")));

  table_destroy(table);
}

void test_table_lookup_result_stays_owned_by_the_table(void) {
  fwd_table_t *table = table_create();

  add_route(table, "10.0.0.0/8 direct eth0");
  TEST_ASSERT_EQUAL_PTR(table_lookup(table, ip("10.1.1.1")), table_lookup(table, ip("10.2.2.2")));

  table_destroy(table);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ip_parse_valid);
  RUN_TEST(test_ip_parse_rejects_malformed);
  RUN_TEST(test_ip_parse_null);
  RUN_TEST(test_ip_format);
  RUN_TEST(test_ip_format_fits_the_longest_address_exactly);
  RUN_TEST(test_cidr_parse_valid);
  RUN_TEST(test_cidr_parse_keeps_host_bits);
  RUN_TEST(test_cidr_parse_rejects_malformed);
  RUN_TEST(test_cidr_parse_null);
  RUN_TEST(test_cidr_format);
  RUN_TEST(test_cidr_format_uses_the_address_as_stored);
  RUN_TEST(test_cidr_format_writes_a_prefix_over_32_as_32);
  RUN_TEST(test_cidr_format_fits_the_longest_block_exactly);

  RUN_TEST(test_cidr_mask);
  RUN_TEST(test_cidr_mask_treats_a_prefix_over_32_as_32);
  RUN_TEST(test_cidr_mask_every_prefix);
  RUN_TEST(test_cidr_network);
  RUN_TEST(test_cidr_broadcast);
  RUN_TEST(test_cidr_first_host);
  RUN_TEST(test_cidr_first_host_of_31_and_32_is_the_network);
  RUN_TEST(test_cidr_last_host);
  RUN_TEST(test_cidr_last_host_of_31_and_32_is_the_broadcast);
  RUN_TEST(test_cidr_host_count);
  RUN_TEST(test_cidr_host_count_of_0_needs_33_bits_on_the_way);
  RUN_TEST(test_cidr_host_count_of_31_and_32);
  RUN_TEST(test_cidr_contains);
  RUN_TEST(test_cidr_contains_boundary_prefixes);
  RUN_TEST(test_cidr_is_network);
  RUN_TEST(test_cidr_subnet_splits_a_24_into_four);
  RUN_TEST(test_cidr_subnet_of_one_is_the_block);
  RUN_TEST(test_cidr_subnet_boundary_prefixes);
  RUN_TEST(test_cidr_subnet_largest_legal_split);
  RUN_TEST(test_cidr_subnet_rejects_a_prefix_longer_than_32);
  RUN_TEST(test_cidr_subnet_rejects_bad_arguments);

  RUN_TEST(test_route_parse_valid);
  RUN_TEST(test_route_parse_default_and_direct);
  RUN_TEST(test_route_parse_boundary_blocks);
  RUN_TEST(test_route_parse_ignores_surrounding_whitespace);
  RUN_TEST(test_route_parse_device_length);
  RUN_TEST(test_route_parse_rejects_malformed);
  RUN_TEST(test_route_parse_null);
  RUN_TEST(test_table_create_is_empty);
  RUN_TEST(test_table_destroy_null);
  RUN_TEST(test_table_count);
  RUN_TEST(test_table_add_rejects_null);
  RUN_TEST(test_table_add_rejects_host_bits);
  RUN_TEST(test_table_add_rejects_a_second_route_to_the_same_block);
  RUN_TEST(test_table_add_same_address_with_another_prefix_is_a_new_block);
  RUN_TEST(test_table_add_stores_a_copy);
  RUN_TEST(test_table_add_grows);
  RUN_TEST(test_table_load_example);
  RUN_TEST(test_table_load_skips_comments_and_blank_lines);
  RUN_TEST(test_table_load_empty_file);
  RUN_TEST(test_table_load_last_line_without_a_newline);
  RUN_TEST(test_table_load_crlf_line_endings);
  RUN_TEST(test_table_load_a_line_longer_than_any_fixed_buffer);
  RUN_TEST(test_table_load_bad_line_in_the_middle);
  RUN_TEST(test_table_load_bad_first_and_last_lines);
  RUN_TEST(test_table_load_bad_line_with_null_bad_line);
  RUN_TEST(test_table_load_rejects_a_route_table_add_refuses);
  RUN_TEST(test_table_load_rejects_host_bits);
  RUN_TEST(test_table_load_rejects_a_nul_byte_in_a_line);
  RUN_TEST(test_table_load_null_arguments);
  RUN_TEST(test_table_load_file_that_can_not_be_read);
  RUN_TEST(test_table_lookup_several_match_and_the_longest_wins);
  RUN_TEST(test_table_lookup_does_not_depend_on_insertion_order);
  RUN_TEST(test_table_lookup_default_loses_to_anything_more_specific);
  RUN_TEST(test_table_lookup_no_default_route);
  RUN_TEST(test_table_lookup_31_and_32);
  RUN_TEST(test_table_lookup_edge_addresses_have_host_routes);
  RUN_TEST(test_table_lookup_null_and_empty);
  RUN_TEST(test_table_lookup_result_stays_owned_by_the_table);

  return UNITY_END();
}
