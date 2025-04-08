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
};

enum key_sels {
    KEY_SEL_PUBLISHERNAME = KEY_PG_INVENTORY + 1,
    KEY_SEL_AUTHORNAME,
    KEY_SEL_ACTIONAME,
    KEY_SEL_LANGCODE,
    KEY_SEL_TYPENAME,
    KEY_SEL_CAMPUSNAME,
    KEY_SEL_ROLENAME,
    KEY_SEL_CATEGORYCLASS,
    KEY_SEL_UUID,
    KEY_SEL_SERIALNUM,
};

enum key_mods {
    KEY_MOD_NAME = KEY_SEL_SERIALNUM + 1,
    KEY_MOD_PERMS,
    KEY_MOD_CLASS,
    KEY_MOD_PARENT,
    KEY_MOD_UUID,
    KEY_MOD_PW,
    KEY_MOD_CAMPUS,
    KEY_MOD_ROLE,
    KEY_MOD_FROZEN,
    KEY_MOD_SERIALNUM,
    KEY_MOD_TYPE,
    KEY_MOD_CATEGORY,
    KEY_MOD_PUBLISHER,
    KEY_MOD_YEAR,
    KEY_MOD_COVER,
    KEY_MOD_HITS,
    KEY_MOD_LANG,
    KEY_MOD_AUTHOR,
    KEY_MOD_STOCK,
    KEY_MOD_DURATION,
    KEY_MOD_DATE,
    KEY_MOD_EXTENDED,
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
    {kvalid_stringne, "select_publishername"},
    {kvalid_stringne, "select_authorname"},
    {kvalid_stringne, "select_actioname"},
    {kvalid_stringne, "select_langcode"},
    {kvalid_stringne, "select_typename"},
    {kvalid_stringne, "select_campusname"},
    {kvalid_stringne, "select_rolename"},
    {kvalid_stringne, "select_categoryclass"},
    {kvalid_stringne, "select_uuid"},
    {kvalid_stringne, "select_serialnum"},
    {kvalid_stringne, "mod_name"},
    {kvalid_int, "mod_perms"},
    {kvalid_stringne, "mod_class"},
    {kvalid_stringne, "mod_parent"},
    {kvalid_stringne, "mod_uuid"},
    {kvalid_stringne, "mod_pw"},
    {kvalid_stringne, "mod_campus"},
    {kvalid_stringne, "mod_role"},
    {kvalid_int, "mod_frozen"},
    {kvalid_stringne, "mod_serialnum"},
    {kvalid_stringne, "mod_type"},
    {kvalid_stringne, "mod_category"},
    {kvalid_stringne, "mod_publisher"},
    {kvalid_int, "mod_year"},
    {kvalid_string, "mod_cover"},
    {kvalid_int, "mod_hits"},
    {kvalid_stringne, "mod_lang"},
    {kvalid_stringne, "mod_author"},
    {kvalid_int, "mod_stock"},
    {kvalid_int, "mod_duration"},
    {kvalid_date, "mod_date"},
    {kvalid_int, "mod_extended"},
};

enum statement_comp {
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

};


static struct sqlbox_pstmt pstmts_switches[STMTS__MAX][8] = {
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
static enum key_mods switch_keys[STMTS__MAX][9] = {
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY_MOD_PERMS, KEY__MAX},
    {KEY_MOD_CLASS, KEY_MOD_NAME, KEY_MOD_PARENT, KEY__MAX},
    {KEY_MOD_UUID, KEY_MOD_NAME, KEY_MOD_PW, KEY_MOD_CAMPUS, KEY_MOD_ROLE, KEY_MOD_FROZEN, KEY__MAX},
    {
        KEY_MOD_SERIALNUM, KEY_MOD_TYPE, KEY_MOD_CATEGORY, KEY_MOD_PUBLISHER, KEY_MOD_NAME, KEY_MOD_YEAR, KEY_MOD_COVER,
        KEY_MOD_HITS, KEY__MAX
    },
    {KEY_MOD_SERIALNUM, KEY_MOD_LANG, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_MOD_AUTHOR, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_MOD_CAMPUS, KEY_MOD_STOCK, KEY__MAX},
    {KEY_MOD_UUID, KEY_MOD_SERIALNUM, KEY_MOD_DURATION, KEY_MOD_DATE, KEY_MOD_EXTENDED, KEY__MAX}
};
static struct sqlbox_pstmt pstmts_bottom[STMTS__MAX] = {
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

enum statement {
    STMT_EDIT,
    __STMT_SAVE__,
    __STMT_LOGIN__,
    STMT__MAX
};

static struct sqlbox_pstmt pstmts[STMT__MAX] = {
    {NULL},
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'EDIT',datetime('now','localtime'),(?))"
    }
};
static enum key_sels bottom_keys[STMTS__MAX][3] = {
    {KEY_SEL_PUBLISHERNAME, KEY__MAX},
    {KEY_SEL_AUTHORNAME, KEY__MAX},
    {KEY_SEL_ACTIONAME, KEY__MAX},
    {KEY_SEL_LANGCODE, KEY__MAX},
    {KEY_SEL_TYPENAME, KEY__MAX},
    {KEY_SEL_CAMPUSNAME, KEY__MAX},
    {KEY_SEL_ROLENAME, KEY__MAX},
    {KEY_SEL_CATEGORYCLASS, KEY__MAX},
    {KEY_SEL_UUID, KEY__MAX},
    {KEY_SEL_SERIALNUM, KEY__MAX},
    {KEY_SEL_SERIALNUM, KEY_SEL_LANGCODE, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_SEL_AUTHORNAME, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_SEL_CAMPUSNAME, KEY__MAX},
    {KEY_SEL_UUID, KEY_SEL_SERIALNUM, KEY__MAX}
};

enum khttp sanitize() {
    if (r.method != KMETHOD_GET) //TODO: SET TO PUT
        return KHTTP_405;
    if (r.page == PG__MAX) {
        enum key i;
        for (i = KEY_PG_PUBLISHER; i < KEY_SEL_PUBLISHERNAME && !(r.fieldmap[i]); i++) {
        }
        if (i == KEY_SEL_PUBLISHERNAME)
            return KHTTP_404;
    }
    return KHTTP_200;
}

enum khttp second_pass(enum statement_comp STMT) {
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; ++i) {
        if (!(r.fieldmap[bottom_keys[STMT][i]]))
            return KHTTP_400;
    }
    return KHTTP_200;
}

enum khttp third_pass(enum statement_comp STMT) {
    kasprintf(&pstmts[STMT_EDIT].stmt, "%s", pstmts_top[STMT].stmt);
    kasprintf(&pstmts[STMT_EDIT].stmt, "%s%s", pstmts[STMT_EDIT].stmt, pstmts_switches[STMT][0].stmt);
    for (int i = 1; switch_keys[STMT][i] != KEY__MAX; ++i) {
        if (r.fieldmap[switch_keys[STMT][i]]) {
            kasprintf(&pstmts[STMT_EDIT].stmt, "%s,%s", pstmts[STMT_EDIT].stmt, pstmts_switches[STMT][i].stmt);
        }
    }
    kasprintf(&pstmts[STMT_EDIT].stmt, "%s%s", pstmts[STMT_EDIT].stmt, pstmts_bottom[STMT].stmt);
    return KHTTP_200;
}

enum statement_comp get_stmts() {
    if (r.page != PG__MAX)
        return (enum statement_comp) r.page;
    enum key i;
    for (i = KEY_PG_PUBLISHER; i < KEY_SEL_PUBLISHERNAME && !(r.fieldmap[i]); i++) {
    }
    return (enum statement_comp) i;
}

int main() {
    enum khttp er;
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the PG__MAX if no page was found
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG__MAX) != KCGI_OK)
        return EXIT_FAILURE;
    if ((er = sanitize()) != KHTTP_200) goto cleanup;
    const enum statement_comp STMT = get_stmts();
    if ((er = second_pass(STMT)) != KHTTP_200) goto cleanup;
    if ((er = third_pass(STMT)) != KHTTP_200) goto cleanup;
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    khttp_puts(&r, pstmts[STMT_EDIT].stmt);

    khttp_free(&r);
    return 0;
cleanup:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    if (r.mime == KMIME_TEXT_HTML)
        khttp_puts(&r, "Could not service request. Second pass");
    khttp_free(&r);
    return 0;
}
