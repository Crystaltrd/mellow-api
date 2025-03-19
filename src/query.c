#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge() */
#include <err.h> /* err(), warnx() */
#include <stdlib.h> /* EXIT_FAILURE */
#include <string.h> /* memset() */
#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdbool.h>
/*
 * The Permissions struct for a role, the order in this struct is the same as
 * how it should be interpreted in binary
 */
struct accperms {
    bool admin;
    bool staff;
    bool manage_books;
    bool manage_stock;
    bool return_book;
    bool rent_book;
    bool inventory;
};

/*
 * All possible sub-pages in the query endpoint with their corresponding names
 */
enum pg {
    PG_PUBLISHER,
    PG_AUTHOR,
    PG_LANG,
    PG_ACTION,
    PG_DOCTYPE,
    PG_CAMPUS,
    PG_ROLE,
    PG_CATEGORY,
    PG_ACCOUNT,
    PG_BOOK,
    PG_STOCK,
    PG_INVENTORY,
    PG_HISTORY,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "publisher", "author", "lang", "action", "doctype", "campus", "role", "category", "account", "book", "stock",
    "inventory", "history"
};

/*
 * All possible keys that can be passed to the backend, the first 11 ones are used as a fix for some hosting providers
 * not providing URL normalisation and require that the binaries end with .cgi
 */
enum key {
    KEY_PG_PUBLISHER,
    KEY_PG_AUTHOR,
    KEY_PG_LANG,
    KEY_PG_ACTION,
    KEY_PG_DOCTYPE,
    KEY_PG_CAMPUS,
    KEY_PG_ROLE,
    KEY_PG_CATEGORY,
    KEY_PG_ACCOUNT,
    KEY_PG_BOOK,
    KEY_PG_STOCK,
    KEY_PG_INVENTORY,
    KEY_PG_HISTORY,
    // FILTERS
    KEY_FILTER_BY_NAME,
    KEY_FILTER_BY_BOOK,
    KEY_FILTER_BY_POPULARITY,
    KEY_FILTER_BY_ACCOUNT,
    KEY_FILTER_BY_PERM,
    KEY_FILTER_BY_CLASS,
    KEY_FILTER_BY_PARENT,
    KEY_FILTER_CASCADE,
    KEY_FILTER_TREE,
    KEY_FILTER_GET_PARENTS,
    KEY_FILTER_BY_ID,
    KEY_FILTER_BY_CAMPUS,
    KEY_FILTER_BY_ROLE,
    KEY_FILTER_FROZEN,
    KEY_FILTER_BY_CATEGORY,
    KEY_FILTER_BY_LANG,
    KEY_FILTER_BY_AUTHOR,
    KEY_FILTER_BY_TYPE,
    KEY_FILTER_BY_PUBLISHER,
    KEY_FILTER_IGNORE_EMPTY,
    KEY_FILTER_FROM_YEAR,
    KEY_FILTER_TO_YEAR,
    KEY_FILTER_SELF,
    KEY_FILTER_BY_ISSUER,
    KEY_FILTER_BY_ACTION,
    KEY_FILTER_FROM_DATE,
    KEY_FILTER_TO_DATE,
    KEY__MAX
};


static const struct kvalid keys[KEY__MAX] = {
    {NULL, "publisher"},
    {NULL, "author"},
    {NULL, "lang"},
    {NULL, "action"},
    {NULL, "doctype"},
    {NULL, "campus"},
    {NULL, "role"},
    {NULL, "category"},
    {NULL, "account"},
    {NULL, "book"},
    {NULL, "stock"},
    {NULL, "inventory"},
    {NULL, "history"},
    // FILTERS
    {kvalid_string, "by_name"},
    {kvalid_string, "by_book"},
    {NULL, "by_popularity"},
    {kvalid_string, "by_account"},
    {kvalid_uint, "by_perm"},
    {kvalid_string, "by_class"},
    {kvalid_string, "by_parent"},
    {NULL, "cascade"},
    {NULL, "tree"},
    {NULL, "get_parents"},
    {kvalid_string, "by_ID"},
    {kvalid_string, "by_campus"},
    {kvalid_string, "by_role"},
    {kvalid_bit, "frozen"},
    {kvalid_string, "by_category"},
    {kvalid_string, "by_lang"},
    {kvalid_string, "by_author"},
    {kvalid_string, "by_type"},
    {kvalid_string, "by_publisher"},
    {kvalid_bit, "ignore_empty"},
    {kvalid_int, "from_year"},
    {kvalid_int, "to_year"},
    {NULL, "self"},
    {kvalid_string, "by_issuer"},
    {kvalid_string, "by_action"},
    {kvalid_date, "from_date"},
    {kvalid_date, "to_date"},
};


int main(void) {
    struct kreq r;
    struct kjsonreq req;
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the INVENTORY if invalid page
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG_INVENTORY) != KCGI_OK)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
