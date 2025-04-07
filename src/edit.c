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
#include <time.h>
#include <pwd.h>
#include <unistd.h>
struct kreq r;
struct kjsonreq req;


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
    PG__MAX
};


static const char *pages[PG__MAX] = {
    "publisher", "author", "lang", "action", "doctype", "campus", "role", "category", "account", "book", "stock",
    "inventory"
};

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
    KEY_PG_SESSIONS,
    KEY_NAME,
    KEY_PERMS,
    KEY_CLASS,
    KEY_PARENTID,
    KEY_UUID,
    KEY_PASSWORD,
    KEY_CAMPUS,
    KEY_ROLE,
    KEY_FROZEN,
    KEY_SERIALNUM,
    KEY_TYPE,
    KEY_CATEGORY,
    KEY_PUBLISHER,
    KEY_YEAR,
    KEY_COVER,
    KEY_HITS,
    KEY_LANG,
    KEY_AUTHOR,
    KEY_STOCK,
    KEY_DURATION,
    KEY_DATE,
    KEY_EXTENDED,
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
};

enum statement {
    STMTS_PUBLISHER,
    STMTS_AUTHOR,
    STMTS_LANG,
    STMTS_ACTION,
    STMTS_TYPE,
    STMTS_CAMPUS,
    STMTS_ROLE,
    STMTS_CATEGORY,
    STMTS_ACCOUNT,
    STMTS_BOOK,
    STMTS_LANGUAGES,
    STMTS_AUTHORED,
    STMTS_STOCK,
    STMTS_INVENTORY,
    __STMT_STORE__,
    STMTS__MAX
};

static const char *statement_string[STMTS__MAX] = {
    "publisher", "author", "lang", "action", "type", "campus", "role", "category", "account",
    "book", "booklang", "bookauthor", "stock", "inventory"
};
static struct sqlbox_pstmt pstmts_top[STMTS__MAX] = {
    {(char *) "UPDATE PUBLISHER SET "},
    {(char *) "UPDATE AUTHOR SET "},
    {(char *) "UPDATE ACTION SET "},
    {(char *) "UPDATE LANG SET "},
    {(char *) "UPDATE DOCTYPE SET "},
    {(char *) "UPDATE CAMPUS SET "},
    {(char *) "UPDATE ROLE SET "},
    {(char *) "UPDATE CATEGORY SET "},
    {(char *) "UPDATE ACCOUNT SET "},
    {(char *) "UPDATE BOOK SET "},
    {(char *) "UPDATE LANGUAGES SET "},
    {(char *) "UPDATE AUTHORED SET "},
    {(char *) "UPDATE STOCK SET "},
    {(char *) "UPDATE INVENTORY SET "},
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'EDIT',datetime('now','localtime'),(?))"
    }
};


static struct sqlbox_pstmt pstmts_switches[STMTS__MAX - 1][8] = {
    {{(char *) "publisherName = (?) "}},
    {{(char *) "authorName = (?) "}},
    {{(char *) "actionName = (?) "}},
    {{(char *) "langcode = (?) "}},
    {{(char *) "typeName = (?) "}},
    {{(char *) "campusName = (?) "}},
    {{(char *) "roleName = (?) "}, {(char *) "perms = (?) "}},
    {{(char *) "categoryClass = (?) "}, {(char *) "categoryName = (?) "}, {(char *) "parentCategoryID = (?) "}},
    {
        {(char *) "UUID = (?) "}, {(char *) "displayname = (?) "}, {(char *) "pwhash = (?) "},
        {(char *) "campus = (?) "}, {(char *) "role = (?) "}, {(char *) "frozen = (?) "}
    },
    {
        {(char *) "serialnum = (?)"}, {(char *) "type = (?) "}, {(char *) "category = (?) "},
        {(char *) "publisher = (?)"}, {(char *) "booktitle = (?)"}, {(char *) "bookreleaseyear = (?)"},
        {(char *) "bookcover = (?)"}, {(char *) "hits = (?)"}
    },
    {{(char *) "serialnum = (?) "}, {(char *) "lang = (?) "}},
    {{(char *) "serialnum = (?) "}, {(char *) "author = (?) "}},
    {{(char *) "serialnum = (?) "}, {(char *) "campus = (?) "}, {(char *) "instock = (?) "}},
    {
        {(char *) "UUID = (?) "}, {(char *) "serialnum = (?) "}, {(char *) "rentduration = (?) "},
        {(char *) "rentdate = (?)"}, {(char *) "extended = (?) "}
    }
};
static struct sqlbox_pstmt pstmts_bottom[STMTS__MAX - 1] = {
    {(char *) "WHERE publisherName = (?)"},
    {(char *) "WHERE authorName = (?)"},
    {(char *) "WHERE actionName = (?)"},
    {(char *) "WHERE langcode = (?) "},
    {(char *) "WHERE typeName = (?) "},
    {(char *) "WHERE campusName = (?) "},
    {(char *) "WHERE roleName = (?) "},
    {(char *) "WHERE categoryClass = (?) "},
    {(char *) "WHERE UUID = (?) "},
    {(char *) "WHERE serialnum = (?) "},
    {(char *) "WHERE serialnum = (?) AND lang = (?) "},
    {(char *) "WHERE serialnum = (?) AND author = (?) "},
    {(char *) "WHERE serialnum = (?) AND campus = (?) "},
    {(char *) "WHERE UUID = (?) AND serialnum = (?) "}
};

enum khttp sanitize() {
    if (r.method != KMETHOD_PUT)
        return KHTTP_405;
    if (r.page == PG__MAX)
        return KHTTP_404;
    return KHTTP_200;
}

enum statement get_stmts() {
    enum key i;
    for (i = KEY_PG_PUBLISHER; i < KEY_NAME && !(r.fieldmap[i]); i++) {
    }
    return i;
}

int main() {
    enum khttp er;
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the INVENTORY if no page was found
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG__MAX) != KCGI_OK)
        return EXIT_FAILURE;

    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    const enum statement STMT = get_stmts();
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    khttp_puts(&r, pstmts_top[STMT].stmt);
    khttp_free(&r);
    return 0;
}
