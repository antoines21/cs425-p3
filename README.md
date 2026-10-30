# P3 - Subnetting and Forwarding

- Name: Antoine Sabatier
- Email: antoinesabatier@u.boisestate.edu
- Class: CS425-001

## Known Bugs or Issues

No known bugs. All 72 unit tests pass with 100% line and branch coverage of
`src/lab.c`. A few limits are worth knowing:

- `table_add()` does not reject a prefix over 32. `lab.h` says a prefix is 0 to
  32 and the failure list for `table_add()` does not include it, so a caller
  that builds a `route_t` by hand can get one in. Nothing that goes through
  `route_parse()` can, because it refuses those prefixes.
- The pointer from `table_lookup()` is only good until the next `table_add()`,
  because growing the table with `realloc()` can move the routes.
- The guide says an address with no route "prints `unreachable`" and that there
  is one line per address, so the program prints `8.8.8.8 unreachable` to
  stdout and exits 3.
- If the table can not be loaded, `lookup` exits 2 even when an address is also
  malformed, because no address can be looked up without a table.
- `make leak` and `make leak-test` abort on macOS with Apple clang, because
  that AddressSanitizer does not support `detect_leaks`. They need Linux.

## Experience

The bit operations were the short part. Much of the code is parsing, because a
parser that rejects everything in the guide has to look at each character:
`sscanf()` and `inet_aton()` both accept input that must be refused, and an
octet like `010` is the kind of thing that passes by accident.

The tests had to be aimed at specific mistakes. A first match lookup passes any
test whose table happens to be sorted longest first, so the example table is
added in every rotation, forwards and backwards. Deliberately breaking the code
(first match instead of longest, no `/0` guard, `/31` treated like any other
prefix) confirmed that the tests fail when they should.

## Design

Every address is a `uint32_t` in host byte order, and almost everything comes
from `cidr_mask()`: the network is `addr & mask`, the broadcast is
`addr | ~mask`, and a block contains `x` when `(x & mask) == (addr & mask)`.

Parsing is written by hand. `parse_decimal()` reads digits with no sign and no
leading zero and stops as soon as the value passes its limit, so a very long
number can not overflow. `ip_parse()` and `cidr_parse()` share it through
`parse_quad()`, and each one then checks that the string ends exactly where it
should.

### `/0`

`cidr_mask()` returns 0 for a prefix of 0 before it reaches the shift, because
`UINT32_MAX << 32` is undefined behavior. On x86 it would hand back all ones,
and the default route would only match `0.0.0.0`.

Two other places need 33 bits. `cidr_host_count()` computes 2^32 in a
`uint64_t` before subtracting 2, and `cidr_subnet()` shifts in 64 bits because
splitting a `/0` into 1 subnet would otherwise shift by 32 as well.

### `/31` and `/32`

`cidr_first_host()` and `cidr_last_host()` check for a prefix of 31 or more
first and return the network and broadcast addresses as they are, with no plus
or minus one. `cidr_host_count()` returns 2 for a `/31` and 1 for a `/32`
before it reaches the formula. The network and broadcast calculations need no
special case, since the mask already gives the right answer: both ends of a
`/31`, and the address itself for a `/32`.

A prefix over 32 is treated as 32 by `cidr_mask()` and `cidr_format()`, and by
the functions built on them. `cidr_subnet()` refuses it, since `lab.h` says the
new prefix can not pass 32.

### Longest prefix match

`fwd_table_t` is an array of `route_t` that starts with room for 8 routes and
doubles with `realloc()` when it fills up. `table_add()` scans it first and
refuses a block that is already there.

`table_lookup()` is a linear scan over every route. It never stops at the first
match: it keeps the matching route with the longest prefix seen so far and
returns that at the end. Since duplicate blocks are refused, two matches can
not tie on prefix length: two blocks of the same length that both contain an
address are the same block. So the answer does not depend on the order the
routes were added in.

The default route is not a special case. `0.0.0.0/0` has a mask of 0, so it
contains every address, and a prefix of 0 loses to any other match. A lookup
costs O(n) in the number of routes, which is fine for a table this size. A
router with a full Internet table would use a binary trie instead.

`table_load()` uses `getline()` so a line can be any length. It frees that
buffer on every path out, and a line that fails leaves the routes before it in
the table for `table_destroy()` to free. It tells a read error (`bad_line` is 0)
from a bad line (`bad_line` is the line number), and rejects a line containing
a `'\0'`, which would otherwise hide the rest of the line from `route_parse()`.
