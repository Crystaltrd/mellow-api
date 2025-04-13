#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge() */
#include <err.h> /* err(), warnx() */
#include <inttypes.h>
#include <stdlib.h> /* EXIT_FAILURE */
#include <string.h> /* memset() */
#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdbool.h>
#include <stdio.h>

struct kreq r;
struct kjsonreq req;
/*
 * The Permissions struct for a role, the order in this struct is the same as
 * how it should be interpreted in binary
 */
struct accperms {
    int numeric;
    bool admin;
    bool staff;
    bool manage_stock;
    bool manage_inventories;
    bool see_accounts;
    bool monitor_history;
    bool has_inventory;
};

struct accperms int_to_accperms(int perm) {
    struct accperms perms = {
        .numeric = perm,
        .admin = (perm & (1 << 6)),
        .staff = (perm & (1 << 5)),
        .manage_stock = (perm & (1 << 4)),
        .manage_inventories = (perm & (1 << 3)),
        .see_accounts = (perm & (1 << 2)),
        .monitor_history = (perm & (1 << 1)),
        .has_inventory = (perm & 1)
    };
    return perms;
}

enum statment {
    STMTS_SEARCH,
    STMTS_COUNT,
    STMTS_LOGIN,
    STMTS_SAVE,
    STMTS__MAX
};

int main() {

    return 0;
}

/* SELECT *
FROM BOOK,
     CATEGORY
WHERE category = categoryClass
  AND (
    instr(serialnum, (?)) > 0
        OR instr(type, (?)) > 0
        OR instr(categoryName, (?)) > 0
        OR instr(publisher, (?)) > 0
        OR instr(booktitle, (?)) > 0
        OR bookreleaseyear = (?))
LIMIT (?) OFFSET (? * (?));*/