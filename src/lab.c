#define _POSIX_C_SOURCE 200809L // getline() on every libc, whatever -std says

#include "lab.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

struct fwd_table
{
  route_t *routes;
  size_t count;
  size_t capacity;
};

/* ------------------------------------------------------------------------
 * Parsing helpers
 * --------------------------------------------------------------------- */

static bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}

// Read a decimal number with no leading zero and no sign, leaving *cursor on
// the first character after it. max must be small enough that value * 10 + 9
// fits in a uint32_t.
static bool parse_decimal(const char **cursor, uint32_t max, uint32_t *value)
{
  const char *p = *cursor;
  uint32_t total = 0;

  if (!is_digit(*p) || (*p == '0' && is_digit(p[1])))
  {
    return false;
  }

  for (; is_digit(*p); p++)
  {
    total = total * 10 + (uint32_t) (*p - '0');
    if (total > max)
    {
      return false;
    }
  }

  *cursor = p;
  *value = total;
  return true;
}

// Read four octets separated by single periods, leaving *cursor on the first
// character after them.
static bool parse_quad(const char **cursor, uint32_t *addr)
{
  uint32_t value = 0;

  for (int i = 0; i < 4; i++)
  {
    uint32_t octet = 0;

    if (i > 0)
    {
      if (**cursor != '.')
      {
        return false;
      }
      (*cursor)++;
    }
    if (!parse_decimal(cursor, 255, &octet))
    {
      return false;
    }
    value = (value << 8) | octet;
  }

  *addr = value;
  return true;
}

static const char *skip_space(const char *p)
{
  while (isspace((unsigned char) *p))
  {
    p++;
  }
  return p;
}

// Only spaces and tabs separate fields; a newline in the middle of a line
// does not.
static const char *skip_blanks(const char *p)
{
  while (*p == ' ' || *p == '\t')
  {
    p++;
  }
  return p;
}

// Copy the run of non-whitespace at *cursor into out and move past it. Fails
// if the run is empty or does not fit in size bytes with its '\0'.
static bool take_field(const char **cursor, char *out, size_t size)
{
  const char *end = *cursor;
  size_t length;

  while (*end != '\0' && !isspace((unsigned char) *end))
  {
    end++;
  }

  length = (size_t) (end - *cursor);
  if (length == 0 || length >= size)
  {
    return false;
  }

  memcpy(out, *cursor, length);
  out[length] = '\0';
  *cursor = end;
  return true;
}

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

bool ip_parse(const char *text, uint32_t *addr)
{
  uint32_t value = 0;

  if (text == NULL || addr == NULL)
  {
    return false;
  }
  if (!parse_quad(&text, &value) || *text != '\0')
  {
    return false;
  }

  *addr = value;
  return true;
}

void ip_format(uint32_t addr, char out[IP_STR_LEN])
{
  snprintf(out, IP_STR_LEN, "%u.%u.%u.%u",
           (unsigned int) (addr >> 24),
           (unsigned int) ((addr >> 16) & 0xFF),
           (unsigned int) ((addr >> 8) & 0xFF),
           (unsigned int) (addr & 0xFF));
}

bool cidr_parse(const char *text, cidr_t *block)
{
  uint32_t addr = 0;
  uint32_t prefix = 0;

  if (text == NULL || block == NULL)
  {
    return false;
  }
  if (!parse_quad(&text, &addr) || *text != '/')
  {
    return false;
  }

  text++;
  if (!parse_decimal(&text, 32, &prefix) || *text != '\0')
  {
    return false;
  }

  block->addr = addr;
  block->prefix = (uint8_t) prefix;
  return true;
}

void cidr_format(cidr_t block, char out[CIDR_STR_LEN])
{
  char addr[IP_STR_LEN];
  unsigned int prefix = block.prefix > 32 ? 32 : block.prefix;

  ip_format(block.addr, addr);
  snprintf(out, CIDR_STR_LEN, "%s/%u", addr, prefix);
}

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

uint32_t cidr_mask(uint8_t prefix)
{
  if (prefix == 0)
  {
    return 0; // the shift below would be by 32, which is undefined
  }
  if (prefix > 32)
  {
    prefix = 32;
  }
  return UINT32_MAX << (32 - prefix);
}

uint32_t cidr_network(cidr_t block)
{
  return block.addr & cidr_mask(block.prefix);
}

uint32_t cidr_broadcast(cidr_t block)
{
  return block.addr | ~cidr_mask(block.prefix);
}

uint32_t cidr_first_host(cidr_t block)
{
  uint32_t network = cidr_network(block);

  return block.prefix >= 31 ? network : network + 1;
}

uint32_t cidr_last_host(cidr_t block)
{
  uint32_t broadcast = cidr_broadcast(block);

  return block.prefix >= 31 ? broadcast : broadcast - 1;
}

uint64_t cidr_host_count(cidr_t block)
{
  if (block.prefix >= 32)
  {
    return 1;
  }
  if (block.prefix == 31)
  {
    return 2;
  }
  return (UINT64_C(1) << (32 - block.prefix)) - 2;
}

bool cidr_contains(cidr_t block, uint32_t addr)
{
  uint32_t mask = cidr_mask(block.prefix);

  return (addr & mask) == (block.addr & mask);
}

bool cidr_is_network(cidr_t block)
{
  return block.addr == cidr_network(block);
}

bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out)
{
  uint32_t borrowed = 0;
  uint32_t prefix;

  if (out == NULL || !cidr_is_network(block))
  {
    return false;
  }
  if (n == 0 || (n & (n - 1)) != 0 || i >= n)
  {
    return false;
  }

  while ((n >> borrowed) > 1)
  {
    borrowed++;
  }

  prefix = block.prefix + borrowed;
  if (prefix > 32)
  {
    return false;
  }

  // 64 bits because splitting a /0 into 1 has a new prefix of 0, and shifting
  // a uint32_t by 32 is undefined
  out->addr = block.addr | (uint32_t) ((uint64_t) i << (32 - prefix));
  out->prefix = (uint8_t) prefix;
  return true;
}

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

bool route_parse(const char *line, route_t *route)
{
  char dest[CIDR_STR_LEN];
  char gateway[IP_STR_LEN];
  route_t parsed;

  if (line == NULL || route == NULL)
  {
    return false;
  }

  memset(&parsed, 0, sizeof(parsed));

  // A field too long for its buffer can not be valid, the buffers hold the
  // longest string that could be
  line = skip_space(line);
  if (!take_field(&line, dest, sizeof(dest)))
  {
    return false;
  }
  line = skip_blanks(line);
  if (!take_field(&line, gateway, sizeof(gateway)))
  {
    return false;
  }
  line = skip_blanks(line);
  if (!take_field(&line, parsed.dev, sizeof(parsed.dev)))
  {
    return false;
  }
  if (*skip_space(line) != '\0')
  {
    return false;
  }

  if (strcmp(dest, "default") != 0 &&
      (!cidr_parse(dest, &parsed.dest) || !cidr_is_network(parsed.dest)))
  {
    return false;
  }
  if (strcmp(gateway, "direct") != 0 && !ip_parse(gateway, &parsed.gateway))
  {
    return false;
  }

  *route = parsed;
  return true;
}

fwd_table_t *table_create(void)
{
  return calloc(1, sizeof(fwd_table_t));
}

void table_destroy(fwd_table_t *table)
{
  if (table == NULL)
  {
    return;
  }

  free(table->routes);
  free(table);
}

size_t table_count(const fwd_table_t *table)
{
  return table == NULL ? 0 : table->count;
}

bool table_add(fwd_table_t *table, const route_t *route)
{
  if (table == NULL || route == NULL || !cidr_is_network(route->dest))
  {
    return false;
  }

  for (size_t i = 0; i < table->count; i++)
  {
    const cidr_t *dest = &table->routes[i].dest;

    if (dest->addr == route->dest.addr && dest->prefix == route->dest.prefix)
    {
      return false;
    }
  }

  if (table->count == table->capacity)
  {
    size_t capacity = table->capacity == 0 ? INITIAL_CAPACITY : table->capacity * 2;
    route_t *routes = realloc(table->routes, capacity * sizeof(route_t));

    if (routes == NULL) // GCOVR_EXCL_START
    {
      return false; // Memory allocation failed
    } // GCOVR_EXCL_STOP

    table->routes = routes;
    table->capacity = capacity;
  }

  table->routes[table->count] = *route;
  table->count++;
  return true;
}

// Handle one line of a table file. length is what getline() returned.
static bool load_line(fwd_table_t *table, char *line, size_t length)
{
  char *comment;
  route_t route;

  // A '\0' in the file would hide the rest of the line from everything below
  if (strlen(line) != length)
  {
    return false;
  }

  comment = strchr(line, '#');
  if (comment != NULL)
  {
    *comment = '\0';
  }

  if (*skip_space(line) == '\0')
  {
    return true;
  }

  return route_parse(line, &route) && table_add(table, &route);
}

bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line)
{
  char *line = NULL;
  size_t size = 0;
  size_t number = 0;
  ssize_t length;
  bool ok = true;

  if (bad_line != NULL)
  {
    *bad_line = 0;
  }
  if (table == NULL || in == NULL)
  {
    return false;
  }

  while (ok && (length = getline(&line, &size, in)) != -1)
  {
    number++;
    ok = load_line(table, line, (size_t) length);
  }

  // getline() also returns -1 when the read fails, which no line is to blame for
  if (ok && !feof(in))
  {
    ok = false;
    number = 0;
  }

  free(line);

  if (!ok && bad_line != NULL)
  {
    *bad_line = number;
  }
  return ok;
}

const route_t *table_lookup(const fwd_table_t *table, uint32_t dest)
{
  const route_t *best = NULL;

  if (table == NULL)
  {
    return NULL;
  }

  // table_add() refuses duplicate blocks, so no two matches tie on prefix and
  // the result can not depend on the order of the routes
  for (size_t i = 0; i < table->count; i++)
  {
    const route_t *route = &table->routes[i];

    if (cidr_contains(route->dest, dest) &&
        (best == NULL || route->dest.prefix > best->dest.prefix))
    {
      best = route;
    }
  }

  return best;
}
