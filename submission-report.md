# Submission Report

- Submission generated at 10/30/2026 at 22:36:54

- Machine info: Linux runnervm48cm3 7.0.0-1017-azure #17~24.04.1-Ubuntu SMP PREEMPT Thu Sep 24 18:46:57 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

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

---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug/lab.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/lab.c.o build/debug/main.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/lab.c -o build/release/lab.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/lab.c.o build/release/main.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/lab.c -o build/tests/lab.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/lab.c.o build/tests/main.c.o build/tests/harness/unity.c.o build/tests/lab-test.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug-test/lab.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/lab.c.o build/debug-test/main.c.o build/debug-test/harness/unity.c.o build/debug-test/lab-test.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
tests/lab-test.c:1073:test_ip_parse_valid:PASS
tests/lab-test.c:1074:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:1075:test_ip_parse_null:PASS
tests/lab-test.c:1076:test_ip_format:PASS
tests/lab-test.c:1077:test_ip_format_fits_the_longest_address_exactly:PASS
tests/lab-test.c:1078:test_cidr_parse_valid:PASS
tests/lab-test.c:1079:test_cidr_parse_keeps_host_bits:PASS
tests/lab-test.c:1080:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:1081:test_cidr_parse_null:PASS
tests/lab-test.c:1082:test_cidr_format:PASS
tests/lab-test.c:1083:test_cidr_format_uses_the_address_as_stored:PASS
tests/lab-test.c:1084:test_cidr_format_writes_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1085:test_cidr_format_fits_the_longest_block_exactly:PASS
tests/lab-test.c:1087:test_cidr_mask:PASS
tests/lab-test.c:1088:test_cidr_mask_treats_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1089:test_cidr_mask_every_prefix:PASS
tests/lab-test.c:1090:test_cidr_network:PASS
tests/lab-test.c:1091:test_cidr_broadcast:PASS
tests/lab-test.c:1092:test_cidr_first_host:PASS
tests/lab-test.c:1093:test_cidr_first_host_of_31_and_32_is_the_network:PASS
tests/lab-test.c:1094:test_cidr_last_host:PASS
tests/lab-test.c:1095:test_cidr_last_host_of_31_and_32_is_the_broadcast:PASS
tests/lab-test.c:1096:test_cidr_host_count:PASS
tests/lab-test.c:1097:test_cidr_host_count_of_0_needs_33_bits_on_the_way:PASS
tests/lab-test.c:1098:test_cidr_host_count_of_31_and_32:PASS
tests/lab-test.c:1099:test_cidr_contains:PASS
tests/lab-test.c:1100:test_cidr_contains_boundary_prefixes:PASS
tests/lab-test.c:1101:test_cidr_is_network:PASS
tests/lab-test.c:1102:test_cidr_subnet_splits_a_24_into_four:PASS
tests/lab-test.c:1103:test_cidr_subnet_of_one_is_the_block:PASS
tests/lab-test.c:1104:test_cidr_subnet_boundary_prefixes:PASS
tests/lab-test.c:1105:test_cidr_subnet_largest_legal_split:PASS
tests/lab-test.c:1106:test_cidr_subnet_rejects_a_prefix_longer_than_32:PASS
tests/lab-test.c:1107:test_cidr_subnet_rejects_bad_arguments:PASS
tests/lab-test.c:1109:test_route_parse_valid:PASS
tests/lab-test.c:1110:test_route_parse_default_and_direct:PASS
tests/lab-test.c:1111:test_route_parse_boundary_blocks:PASS
tests/lab-test.c:1112:test_route_parse_ignores_surrounding_whitespace:PASS
tests/lab-test.c:1113:test_route_parse_device_length:PASS
tests/lab-test.c:1114:test_route_parse_rejects_malformed:PASS
tests/lab-test.c:1115:test_route_parse_null:PASS
tests/lab-test.c:1116:test_table_create_is_empty:PASS
tests/lab-test.c:1117:test_table_destroy_null:PASS
tests/lab-test.c:1118:test_table_count:PASS
tests/lab-test.c:1119:test_table_add_rejects_null:PASS
tests/lab-test.c:1120:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:1121:test_table_add_rejects_a_second_route_to_the_same_block:PASS
tests/lab-test.c:1122:test_table_add_same_address_with_another_prefix_is_a_new_block:PASS
tests/lab-test.c:1123:test_table_add_stores_a_copy:PASS
tests/lab-test.c:1124:test_table_add_grows:PASS
tests/lab-test.c:1125:test_table_load_example:PASS
tests/lab-test.c:1126:test_table_load_skips_comments_and_blank_lines:PASS
tests/lab-test.c:1127:test_table_load_empty_file:PASS
tests/lab-test.c:1128:test_table_load_last_line_without_a_newline:PASS
tests/lab-test.c:1129:test_table_load_crlf_line_endings:PASS
tests/lab-test.c:1130:test_table_load_a_line_longer_than_any_fixed_buffer:PASS
tests/lab-test.c:1131:test_table_load_bad_line_in_the_middle:PASS
tests/lab-test.c:1132:test_table_load_bad_first_and_last_lines:PASS
tests/lab-test.c:1133:test_table_load_bad_line_with_null_bad_line:PASS
tests/lab-test.c:1134:test_table_load_rejects_a_route_table_add_refuses:PASS
tests/lab-test.c:1135:test_table_load_rejects_host_bits:PASS
tests/lab-test.c:1136:test_table_load_rejects_a_nul_byte_in_a_line:PASS
tests/lab-test.c:1137:test_table_load_null_arguments:PASS
tests/lab-test.c:1138:test_table_load_file_that_can_not_be_read:PASS
tests/lab-test.c:1139:test_table_lookup_several_match_and_the_longest_wins:PASS
tests/lab-test.c:1140:test_table_lookup_does_not_depend_on_insertion_order:PASS
tests/lab-test.c:1141:test_table_lookup_default_loses_to_anything_more_specific:PASS
tests/lab-test.c:1142:test_table_lookup_no_default_route:PASS
tests/lab-test.c:1143:test_table_lookup_31_and_32:PASS
tests/lab-test.c:1144:test_table_lookup_edge_addresses_have_host_routes:PASS
tests/lab-test.c:1145:test_table_lookup_null_and_empty:PASS
tests/lab-test.c:1146:test_table_lookup_result_stays_owned_by_the_table:PASS

-----------------------
72 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
tests/lab-test.c:1073:test_ip_parse_valid:PASS
tests/lab-test.c:1074:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:1075:test_ip_parse_null:PASS
tests/lab-test.c:1076:test_ip_format:PASS
tests/lab-test.c:1077:test_ip_format_fits_the_longest_address_exactly:PASS
tests/lab-test.c:1078:test_cidr_parse_valid:PASS
tests/lab-test.c:1079:test_cidr_parse_keeps_host_bits:PASS
tests/lab-test.c:1080:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:1081:test_cidr_parse_null:PASS
tests/lab-test.c:1082:test_cidr_format:PASS
tests/lab-test.c:1083:test_cidr_format_uses_the_address_as_stored:PASS
tests/lab-test.c:1084:test_cidr_format_writes_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1085:test_cidr_format_fits_the_longest_block_exactly:PASS
tests/lab-test.c:1087:test_cidr_mask:PASS
tests/lab-test.c:1088:test_cidr_mask_treats_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1089:test_cidr_mask_every_prefix:PASS
tests/lab-test.c:1090:test_cidr_network:PASS
tests/lab-test.c:1091:test_cidr_broadcast:PASS
tests/lab-test.c:1092:test_cidr_first_host:PASS
tests/lab-test.c:1093:test_cidr_first_host_of_31_and_32_is_the_network:PASS
tests/lab-test.c:1094:test_cidr_last_host:PASS
tests/lab-test.c:1095:test_cidr_last_host_of_31_and_32_is_the_broadcast:PASS
tests/lab-test.c:1096:test_cidr_host_count:PASS
tests/lab-test.c:1097:test_cidr_host_count_of_0_needs_33_bits_on_the_way:PASS
tests/lab-test.c:1098:test_cidr_host_count_of_31_and_32:PASS
tests/lab-test.c:1099:test_cidr_contains:PASS
tests/lab-test.c:1100:test_cidr_contains_boundary_prefixes:PASS
tests/lab-test.c:1101:test_cidr_is_network:PASS
tests/lab-test.c:1102:test_cidr_subnet_splits_a_24_into_four:PASS
tests/lab-test.c:1103:test_cidr_subnet_of_one_is_the_block:PASS
tests/lab-test.c:1104:test_cidr_subnet_boundary_prefixes:PASS
tests/lab-test.c:1105:test_cidr_subnet_largest_legal_split:PASS
tests/lab-test.c:1106:test_cidr_subnet_rejects_a_prefix_longer_than_32:PASS
tests/lab-test.c:1107:test_cidr_subnet_rejects_bad_arguments:PASS
tests/lab-test.c:1109:test_route_parse_valid:PASS
tests/lab-test.c:1110:test_route_parse_default_and_direct:PASS
tests/lab-test.c:1111:test_route_parse_boundary_blocks:PASS
tests/lab-test.c:1112:test_route_parse_ignores_surrounding_whitespace:PASS
tests/lab-test.c:1113:test_route_parse_device_length:PASS
tests/lab-test.c:1114:test_route_parse_rejects_malformed:PASS
tests/lab-test.c:1115:test_route_parse_null:PASS
tests/lab-test.c:1116:test_table_create_is_empty:PASS
tests/lab-test.c:1117:test_table_destroy_null:PASS
tests/lab-test.c:1118:test_table_count:PASS
tests/lab-test.c:1119:test_table_add_rejects_null:PASS
tests/lab-test.c:1120:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:1121:test_table_add_rejects_a_second_route_to_the_same_block:PASS
tests/lab-test.c:1122:test_table_add_same_address_with_another_prefix_is_a_new_block:PASS
tests/lab-test.c:1123:test_table_add_stores_a_copy:PASS
tests/lab-test.c:1124:test_table_add_grows:PASS
tests/lab-test.c:1125:test_table_load_example:PASS
tests/lab-test.c:1126:test_table_load_skips_comments_and_blank_lines:PASS
tests/lab-test.c:1127:test_table_load_empty_file:PASS
tests/lab-test.c:1128:test_table_load_last_line_without_a_newline:PASS
tests/lab-test.c:1129:test_table_load_crlf_line_endings:PASS
tests/lab-test.c:1130:test_table_load_a_line_longer_than_any_fixed_buffer:PASS
tests/lab-test.c:1131:test_table_load_bad_line_in_the_middle:PASS
tests/lab-test.c:1132:test_table_load_bad_first_and_last_lines:PASS
tests/lab-test.c:1133:test_table_load_bad_line_with_null_bad_line:PASS
tests/lab-test.c:1134:test_table_load_rejects_a_route_table_add_refuses:PASS
tests/lab-test.c:1135:test_table_load_rejects_host_bits:PASS
tests/lab-test.c:1136:test_table_load_rejects_a_nul_byte_in_a_line:PASS
tests/lab-test.c:1137:test_table_load_null_arguments:PASS
tests/lab-test.c:1138:test_table_load_file_that_can_not_be_read:PASS
tests/lab-test.c:1139:test_table_lookup_several_match_and_the_longest_wins:PASS
tests/lab-test.c:1140:test_table_lookup_does_not_depend_on_insertion_order:PASS
tests/lab-test.c:1141:test_table_lookup_default_loses_to_anything_more_specific:PASS
tests/lab-test.c:1142:test_table_lookup_no_default_route:PASS
tests/lab-test.c:1143:test_table_lookup_31_and_32:PASS
tests/lab-test.c:1144:test_table_lookup_edge_addresses_have_host_routes:PASS
tests/lab-test.c:1145:test_table_lookup_null_and_empty:PASS
tests/lab-test.c:1146:test_table_lookup_result_stays_owned_by_the_table:PASS

-----------------------
72 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                         Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/lab.c                                      203      203   100%
------------------------------------------------------------------------------
TOTAL                                          203      203   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
tests/lab-test.c:1073:test_ip_parse_valid:PASS
tests/lab-test.c:1074:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:1075:test_ip_parse_null:PASS
tests/lab-test.c:1076:test_ip_format:PASS
tests/lab-test.c:1077:test_ip_format_fits_the_longest_address_exactly:PASS
tests/lab-test.c:1078:test_cidr_parse_valid:PASS
tests/lab-test.c:1079:test_cidr_parse_keeps_host_bits:PASS
tests/lab-test.c:1080:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:1081:test_cidr_parse_null:PASS
tests/lab-test.c:1082:test_cidr_format:PASS
tests/lab-test.c:1083:test_cidr_format_uses_the_address_as_stored:PASS
tests/lab-test.c:1084:test_cidr_format_writes_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1085:test_cidr_format_fits_the_longest_block_exactly:PASS
tests/lab-test.c:1087:test_cidr_mask:PASS
tests/lab-test.c:1088:test_cidr_mask_treats_a_prefix_over_32_as_32:PASS
tests/lab-test.c:1089:test_cidr_mask_every_prefix:PASS
tests/lab-test.c:1090:test_cidr_network:PASS
tests/lab-test.c:1091:test_cidr_broadcast:PASS
tests/lab-test.c:1092:test_cidr_first_host:PASS
tests/lab-test.c:1093:test_cidr_first_host_of_31_and_32_is_the_network:PASS
tests/lab-test.c:1094:test_cidr_last_host:PASS
tests/lab-test.c:1095:test_cidr_last_host_of_31_and_32_is_the_broadcast:PASS
tests/lab-test.c:1096:test_cidr_host_count:PASS
tests/lab-test.c:1097:test_cidr_host_count_of_0_needs_33_bits_on_the_way:PASS
tests/lab-test.c:1098:test_cidr_host_count_of_31_and_32:PASS
tests/lab-test.c:1099:test_cidr_contains:PASS
tests/lab-test.c:1100:test_cidr_contains_boundary_prefixes:PASS
tests/lab-test.c:1101:test_cidr_is_network:PASS
tests/lab-test.c:1102:test_cidr_subnet_splits_a_24_into_four:PASS
tests/lab-test.c:1103:test_cidr_subnet_of_one_is_the_block:PASS
tests/lab-test.c:1104:test_cidr_subnet_boundary_prefixes:PASS
tests/lab-test.c:1105:test_cidr_subnet_largest_legal_split:PASS
tests/lab-test.c:1106:test_cidr_subnet_rejects_a_prefix_longer_than_32:PASS
tests/lab-test.c:1107:test_cidr_subnet_rejects_bad_arguments:PASS
tests/lab-test.c:1109:test_route_parse_valid:PASS
tests/lab-test.c:1110:test_route_parse_default_and_direct:PASS
tests/lab-test.c:1111:test_route_parse_boundary_blocks:PASS
tests/lab-test.c:1112:test_route_parse_ignores_surrounding_whitespace:PASS
tests/lab-test.c:1113:test_route_parse_device_length:PASS
tests/lab-test.c:1114:test_route_parse_rejects_malformed:PASS
tests/lab-test.c:1115:test_route_parse_null:PASS
tests/lab-test.c:1116:test_table_create_is_empty:PASS
tests/lab-test.c:1117:test_table_destroy_null:PASS
tests/lab-test.c:1118:test_table_count:PASS
tests/lab-test.c:1119:test_table_add_rejects_null:PASS
tests/lab-test.c:1120:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:1121:test_table_add_rejects_a_second_route_to_the_same_block:PASS
tests/lab-test.c:1122:test_table_add_same_address_with_another_prefix_is_a_new_block:PASS
tests/lab-test.c:1123:test_table_add_stores_a_copy:PASS
tests/lab-test.c:1124:test_table_add_grows:PASS
tests/lab-test.c:1125:test_table_load_example:PASS
tests/lab-test.c:1126:test_table_load_skips_comments_and_blank_lines:PASS
tests/lab-test.c:1127:test_table_load_empty_file:PASS
tests/lab-test.c:1128:test_table_load_last_line_without_a_newline:PASS
tests/lab-test.c:1129:test_table_load_crlf_line_endings:PASS
tests/lab-test.c:1130:test_table_load_a_line_longer_than_any_fixed_buffer:PASS
tests/lab-test.c:1131:test_table_load_bad_line_in_the_middle:PASS
tests/lab-test.c:1132:test_table_load_bad_first_and_last_lines:PASS
tests/lab-test.c:1133:test_table_load_bad_line_with_null_bad_line:PASS
tests/lab-test.c:1134:test_table_load_rejects_a_route_table_add_refuses:PASS
tests/lab-test.c:1135:test_table_load_rejects_host_bits:PASS
tests/lab-test.c:1136:test_table_load_rejects_a_nul_byte_in_a_line:PASS
tests/lab-test.c:1137:test_table_load_null_arguments:PASS
tests/lab-test.c:1138:test_table_load_file_that_can_not_be_read:PASS
tests/lab-test.c:1139:test_table_lookup_several_match_and_the_longest_wins:PASS
tests/lab-test.c:1140:test_table_lookup_does_not_depend_on_insertion_order:PASS
tests/lab-test.c:1141:test_table_lookup_default_loses_to_anything_more_specific:PASS
tests/lab-test.c:1142:test_table_lookup_no_default_route:PASS
tests/lab-test.c:1143:test_table_lookup_31_and_32:PASS
tests/lab-test.c:1144:test_table_lookup_edge_addresses_have_host_routes:PASS
tests/lab-test.c:1145:test_table_lookup_null_and_empty:PASS
tests/lab-test.c:1146:test_table_lookup_result_stays_owned_by_the_table:PASS

-----------------------
72 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### lab.c

```c

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

```

### lab.h

```c

#ifndef LAB_H
#define LAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Every address in this file is a uint32_t in host byte order, so the first
 * octet of 192.168.1.10 is the most significant byte: 0xC0A8010A.
 */

/** Room for the longest dotted quad, "255.255.255.255", and its '\0'. */
#define IP_STR_LEN 16

/** Room for the longest CIDR string, "255.255.255.255/32", and its '\0'. */
#define CIDR_STR_LEN 19

/** Room for an interface name; IFNAMSIZ on Linux, including the '\0'. */
#define DEV_NAME_LEN 16

/** An address block in CIDR notation: an address and a prefix length. */
typedef struct {
  uint32_t addr;  /**< any address in the block, host byte order */
  uint8_t prefix; /**< 0 to 32 */
} cidr_t;

/** One entry in a forwarding table. */
typedef struct {
  cidr_t dest;            /**< destination block; host bits must be zero */
  uint32_t gateway;       /**< next hop, or 0 when the block is directly connected */
  char dev[DEV_NAME_LEN]; /**< outgoing interface, for example "eth0" */
} route_t;

/** A forwarding table. Its layout is private to lab.c. */
typedef struct fwd_table fwd_table_t;

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

/**
 * @brief Parse a dotted quad such as "192.168.1.10".
 *
 * Exactly four decimal octets from 0 to 255 separated by single periods, with
 * nothing before, between or after them. An octet with a leading zero ("010")
 * is rejected, because inet_aton() reads it as octal and that is never what a
 * person typing it meant.
 *
 * @param text The string to parse.
 * @param addr Where the address is stored on success; untouched on failure.
 * @return true if text is a valid dotted quad, false otherwise (including NULL).
 */
bool ip_parse(const char *text, uint32_t *addr);

/**
 * @brief Format an address as a dotted quad.
 * @param addr The address, host byte order.
 * @param out A buffer of at least IP_STR_LEN bytes.
 */
void ip_format(uint32_t addr, char out[IP_STR_LEN]);

/**
 * @brief Parse a block in CIDR notation such as "192.168.1.0/24".
 *
 * The address follows the rules of ip_parse() and the prefix is a decimal
 * number from 0 to 32 with no leading zero. The slash and the prefix are
 * required. Host bits may be set ("132.178.227.11/25" is an interface
 * address), so use cidr_is_network() when they must not be.
 *
 * @param text The string to parse.
 * @param block Where the block is stored on success; untouched on failure.
 * @return true if text is valid CIDR notation, false otherwise (including NULL).
 */
bool cidr_parse(const char *text, cidr_t *block);

/**
 * @brief Format a block as "a.b.c.d/p", using the address exactly as stored.
 *
 * A prefix over 32 is written as 32, the way cidr_mask() treats it.
 *
 * @param block The block.
 * @param out A buffer of at least CIDR_STR_LEN bytes.
 */
void cidr_format(cidr_t block, char out[CIDR_STR_LEN]);

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

/**
 * @brief The subnet mask for a prefix length: /24 is 255.255.255.0.
 *
 * Careful with /0. Shifting a 32 bit value by 32 is undefined behavior in C.
 *
 * @param prefix 0 to 32; anything larger is treated as 32.
 * @return The mask, host byte order.
 */
uint32_t cidr_mask(uint8_t prefix);

/** @brief The lowest address in the block: the address with the host bits cleared. */
uint32_t cidr_network(cidr_t block);

/** @brief The highest address in the block: the address with the host bits set. */
uint32_t cidr_broadcast(cidr_t block);

/**
 * @brief The first address that can be given to a host.
 *
 * For /0 to /30 that is the network address plus one. A /31 is a point to
 * point link (RFC 3021) where both addresses are usable, and a /32 is a single
 * host, so for those two the first host is the network address.
 */
uint32_t cidr_first_host(cidr_t block);

/**
 * @brief The last address that can be given to a host.
 *
 * The broadcast address minus one for /0 to /30, and the broadcast address
 * itself for /31 and /32 (see cidr_first_host()).
 */
uint32_t cidr_last_host(cidr_t block);

/**
 * @brief How many addresses in the block can be given to hosts.
 *
 * 2^(32 - prefix) - 2 for /0 to /30, 2 for /31, and 1 for /32. For a /0 the
 * 2^32 in that formula needs 33 bits, more than a uint32_t holds, which is why
 * this returns a uint64_t.
 */
uint64_t cidr_host_count(cidr_t block);

/** @brief true if addr falls inside the block. */
bool cidr_contains(cidr_t block, uint32_t addr);

/** @brief true if the block's host bits are all zero, so it names a network. */
bool cidr_is_network(cidr_t block);

/**
 * @brief The i-th of n equally sized subnets of a block.
 *
 * Splitting a /24 into 4 gives four /26 blocks, and subnet 0 is the lowest.
 * Equal sizes are only possible when n is a power of two, and the new prefix
 * can not pass 32.
 *
 * @param block The block to split; it must be a network (cidr_is_network()).
 * @param n How many subnets, a power of two from 1 up.
 * @param i Which subnet, 0 to n - 1.
 * @param out Where the subnet is stored on success; untouched on failure.
 * @return false if out is NULL, the block is not a network, n is not a power
 *         of two, the split would need a prefix longer than 32, or i is out of
 *         range.
 */
bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out);

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

/**
 * @brief Parse one line of a forwarding table file.
 *
 * A line is three fields separated by spaces or tabs:
 *
 *     <destination> <gateway> <device>
 *
 * The destination is a block in CIDR notation with no host bits set, or the
 * word "default" for 0.0.0.0/0. The gateway is a dotted quad, or the word
 * "direct" for a directly connected block (stored as 0). The device is 1 to
 * DEV_NAME_LEN - 1 characters. Leading and trailing whitespace, including a
 * trailing newline, is ignored. Comments are not handled here.
 *
 * @param line The line.
 * @param route Where the route is stored on success; untouched on failure.
 * @return true if the line is a valid route, false otherwise (including NULL).
 */
bool route_parse(const char *line, route_t *route);

/**
 * @brief Create an empty forwarding table.
 * @return The table, which the caller frees with table_destroy(), or NULL if
 *         memory could not be allocated.
 */
fwd_table_t *table_create(void);

/** @brief Free a table and everything in it. NULL is allowed and does nothing. */
void table_destroy(fwd_table_t *table);

/** @brief The number of routes in the table; 0 for NULL. */
size_t table_count(const fwd_table_t *table);

/**
 * @brief Add a copy of a route to the table, growing it as needed.
 * @return false if either argument is NULL, the destination has host bits
 *         set, the table already holds a route to the same block, or memory
 *         could not be allocated. The table is unchanged on failure.
 */
bool table_add(fwd_table_t *table, const route_t *route);

/**
 * @brief Read a forwarding table file and add every route in it.
 *
 * Everything from a '#' to the end of a line is a comment, and a line that is
 * blank once the comment is removed is skipped. Every other line must be a
 * valid route (route_parse()) that table_add() accepts.
 *
 * @param table The table to add to.
 * @param in The open file to read.
 * @param bad_line If not NULL, set to the 1-based number of the first line
 *        that failed, or 0 if the arguments were NULL or the file could not
 *        be read.
 * @return true if every line loaded. On false, routes from the lines before
 *         the bad one stay in the table.
 */
bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line);

/**
 * @brief Longest prefix match: the route whose block contains dest and has the
 *        longest prefix.
 *
 * A default route (0.0.0.0/0) matches everything, so it is chosen only when
 * nothing more specific does. The order the routes were added in does not
 * matter.
 *
 * @return The matching route, which stays owned by the table, or NULL if no
 *         route matches (or table is NULL).
 */
const route_t *table_lookup(const fwd_table_t *table, uint32_t dest);

#endif // LAB_H

```

### main.c

```c

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

```

## Tests Files
### lab-test.c

```c

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

```

## Scripts Files
Report generated on 10/30/2026 at 22:36:56


---

## End of Report

SHA-256 Hash of the report: fc9852f553cd2be94c87ebb8dabf2f8727e4e0823f71579c2921ad7a95d7a34f

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: antoines21/cs425-p3
- The repository visibility is public.
- The workflow was triggered by antoines21
