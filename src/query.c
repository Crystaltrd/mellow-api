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
    PG_SESSIONS,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "publisher", "author", "lang", "action", "doctype", "campus", "role", "category", "account", "book", "stock",
    "inventory", "history", "sessions"
};


/*
 * Pre-made SQL statements
 */
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
    STMTS_STOCK,
    STMTS_INVENTORY,
    STMTS_HISTORY,
    STMTS_SESSIONS,
    __STMT_STORE__,
    __STMT_BOOK_AUTHORED__,
    __STMT_BOOK_LANGUAGES__,
    __STMT_BOOK_STOCK__,
    STMTS__MAX
};

/*
 * Helper function to get the Statement for a specific page
 */
enum statement get_stmts() {
    return (enum statement) r.page;
}

static struct sqlbox_pstmt pstms_data_top[STMTS__MAX] = {
    {
        (char *)
        "SELECT publisherName "
        "FROM PUBLISHER "
        "LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName "
        "WHERE "
    },
    {
        (char *)
        "SELECT authorName "
        "FROM AUTHOR "
        "LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE "
    },
    {
        (char *)
        "SELECT langCode "
        "FROM LANG "
        "LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE "
    },
    {
        (char *)
        "SELECT actionName "
        "FROM ACTION "
        "WHERE "
    },
    {
        (char *)
        "SELECT typeName "
        "FROM DOCTYPE "
        "LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type "
        "WHERE "
    },
    {
        (char *)
        "SELECT campusName "
        "FROM CAMPUS "
        "LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus "
        "LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus "
        "WHERE "
    },
    {
        (char *)
        "SELECT roleName, perms "
        "FROM ROLE LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName "
        "WHERE "
    },
    {
        (char *)
        "SELECT categoryClass, categoryName, parentCategoryID "
        "FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
        "WHERE "
    },

    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen "
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
        "AND "
    },
    {
        (char *)
        "SELECT BOOK.serialnum, type, category,categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "STOCK,"
        "WHERE category = categoryClass "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND STOCK.serialnum = BOOK.serialnum "
        "AND "
    },
    {
        (char *)
        "SELECT STOCK.serialnum,campus,instock "
        "FROM STOCK,BOOK "
        "WHERE STOCK.serialnum = BOOK.serialnum "
        "AND "
    },
    {
        (char *)
        "SELECT UUID, serialnum, rentduration, rentdate, extended "
        "FROM INVENTORY "
        "WHERE "
    },
    {

        (char *)
        "SELECT UUID,UUID_ISSUER,serialnum,IP,action,actiondate,details "
        "FROM HISTORY "
        "WHERE "
    },
    {
        (char *)
        "SELECT account,sessionID,expiresAt "
        "FROM SESSIONS "
        "WHERE "
    },
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'QUERY',datetime('now','localtime'),(?))"
    },
    {
        (char *)
        "SELECT author "
        "FROM AUTHORED "
        "WHERE serialnum = (?)"
    },
    {
        (char *)
        "SELECT lang "
        "FROM LANGUAGES "
        "WHERE serialnum = (?)"
    },

    {
        (char *)
        "SELECT campus,instock "
        "FROM STOCK "
        "WHERE serialnum = (?)"
    },
};

enum key_switches {
    KEY_SWITCH_NAME,
    KEY_SWITCH_SERIALNUM,
    KEY_SWITCH_UUID,
    KEY_SWITCH_PERMS,
    KEY_SWITCH_ROOT, // MUTUALLY EXCLUSIVE
    KEY_SWITCH_PARENT, // MUTUALLY EXCLUSIVE
    KEY_SWITCH_CLASS,
    KEY_SWITCH_CAMPUS,
    KEY_SWITCH_ROLE,
    KEY_SWITCH_FROZEN,
    KEY_SWITCH_SESSIONID,
    KEY_SWITCH_LANG,
    KEY_SWITCH_AUTHOR,
    KEY_SWITCH_PUBLISHER,
    KEY_SWITCH_TYPE,
    KEY_SWITCH_UPPERYEAR,
    KEY_SWITCH_LOWERYEAR,
    KEY_SWITCH_NOTEMPTY,
    KEY_SWITCH_ISSUER,
    KEY_SWITCH_ACTION,
    KEY_SWITCH_UPPERDATE,
    KEY_SWITCH_LOWERDATE,
    KEY_ORDER_HITS,
    KEY_ORDER_NAME,
    KEY_ORDER_PERMS,
    KEY_ORDER_CLASS,
    KEY_ORDER_UUID,
    KEY_ORDER_SERIALNUM,
    KEY_ORDER_DATE,
    KEY_ORDER_STOCK,
    KEY_MANDATORY_GROUP_BY,
    KEY__SWITCH__MAX
};

static struct sqlbox_pstmt pstmts_switches[STMTS__MAX][10] = {
    {
        {(char *) "instr(publisherName,(?)) > 0"},
        {(char *) "serialnum = (?)"}
    },
    {
        {(char *) "instr(authorName,(?)) > 0"},
        {(char *) "A.serialnum = (?)"}
    },
    {
        {(char *) "instr(langCode,(?)) > 0"},
        {(char *) "A.serialnum = (?)"}
    },
    {
        {(char *) "instr(actionName,(?)) > 0"}
    },
    {
        {(char *) "instr(typeName,(?)) > 0"},
        {(char *) "serialnum = (?)"}
    },
    {
        {(char *) "instr(campusName,(?)) > 0"},
        {(char *) "serialnum = (?)"},
        {(char *) "UUID = (?)"}
    },
    {
        {(char *) "instr(roleName,(?)) > 0"},
        {(char *) "perms = (?)"},
        {(char *) "UUID = (?)"}
    },
    {
        {(char *) "parentCategoryID IS NULL"},
        {(char *) "parentCategoryID = (?)"},
        {(char *) "instr(categoryName,(?)) > 0"},
        {(char *) "categoryClass = (?)"},
        {(char *) "serialnum = (?)"},
    },
    {
        {(char *) "ACCOUNT.UUID = (?)"},
        {(char *) "instr(displayname, (?))> 0"},
        {(char *) "serialnum = (?)"},
        {(char *) "campus = (?)"},
        {(char *) "role = (?)"},
        {(char *) "frozen = (?)"},
        {(char *) "sessionID = (?)"},
    },
    {
        {(char *) "BOOK.serialnum = (?)"},
        {(char *) "instr(booktitle, (?))"},
        {(char *) "lang = (?)"},
        {(char *) "instr(author, (?)) > 0"},
        {(char *) "type = (?)"},
        {(char *) "instr(publisher, (?)) > 0"},
        {(char *) "campus = (?)"},
        {(char *) "UUID = (?)"},
        {(char *) "bookreleaseyear >= (?)"},
        {(char *) "bookreleaseyear <= (?)"},
    },
    {
        {(char *) "STOCK.serialnum = (?)"},
        {(char *) "campus = (?)"},
        {(char *) "instock > 0"},
    },
    {
        {(char *) "UUID = (?)"},
        {(char *) "serialnum = (?)"}
    },
    {
        {(char *) "UUID = (?)"},
        {(char *) "UUID_ISSUER = (?)"},
        {(char *) "serialnum = (?)"},
        {(char *) "action = (?)"},
        {(char *) "actiondate >= datetime((?),'unixepoch')"},
        {(char *) "actiondate <= datetime((?),'unixepoch')"},
    },
    {
        {(char *) "sessionID = (?)"},
        {(char *) "account = (?)"},
    }
};

static enum key_switches switch_keys[STMTS__MAX][11] = {

    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY_SWITCH_UUID, KEY__SWITCH__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_PERMS, KEY_SWITCH_UUID, KEY__SWITCH__MAX},
    {KEY_SWITCH_ROOT, KEY_SWITCH_PARENT, KEY_SWITCH_NAME, KEY_SWITCH_CLASS, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {
        KEY_SWITCH_UUID, KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY_SWITCH_CAMPUS, KEY_SWITCH_ROLE, KEY_SWITCH_FROZEN,
        KEY_SWITCH_SESSIONID, KEY__SWITCH__MAX
    },
    {
        KEY_SWITCH_SERIALNUM, KEY_SWITCH_NAME, KEY_SWITCH_LANG, KEY_SWITCH_AUTHOR, KEY_SWITCH_ROLE,
        KEY_SWITCH_PUBLISHER, KEY_SWITCH_CAMPUS, KEY_SWITCH_UUID, KEY_SWITCH_UPPERYEAR, KEY_SWITCH_LOWERYEAR,
        KEY__SWITCH__MAX
    },
    {KEY_SWITCH_SERIALNUM, KEY_SWITCH_CAMPUS, KEY_SWITCH_NOTEMPTY, KEY__SWITCH__MAX},
    {KEY_SWITCH_UUID, KEY_SWITCH_SERIALNUM, KEY__SWITCH__MAX},
    {
        KEY_SWITCH_UUID, KEY_SWITCH_ISSUER, KEY_SWITCH_SERIALNUM, KEY_SWITCH_ACTION, KEY_SWITCH_UPPERYEAR,
        KEY_SWITCH_LOWERDATE, KEY__SWITCH__MAX
    },
    {KEY_SWITCH_SESSIONID, KEY_SWITCH_UUID, KEY__SWITCH__MAX}
};


static enum key_switches bottom_keys[STMTS__MAX][8] = {
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },

    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_PERMS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },

    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_CLASS,
        KEY_ORDER_NAME,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_NAME,
        KEY_ORDER_PERMS,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_NAME,
        KEY_ORDER_DATE,
        KEY_ORDER_HITS,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_STOCK,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_DATE,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_DATE,
        KEY__SWITCH__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_DATE,
        KEY__SWITCH__MAX
    }
};
static struct sqlbox_pstmt pstmts_bottom[STMTS__MAX][5] = {
    {
        {(char *) "publisherName"},
        {(char *) "SUM(hits)"},
        {(char *) "publisherName"},
    },
    {
        {(char *) "authorName"},
        {(char *) "SUM(hits)"},
        {(char *) "authorName"},
    },
    {
        {(char *) "langCode"},
        {(char *) "SUM(hits)"},
        {(char *) "langCode"},
    },
    {
        {(char *) "typeName"},
        {(char *) "SUM(hits)"},
        {(char *) "typeName"},
    },
    {
        {(char *) "campusName "},
        {(char *) "campusName "},
    },
    {
        {(char *) "roleName, perms"},
        {(char *) "perms"},
        {(char *) "roleName"},
    },
    {
        {(char *) "categoryClass, categoryName, parentCategoryID"},
        {(char *) "SUM(hits)"},
        {(char *) "categoryClass"},
        {(char *) "categoryName"},
    },
    {

        {(char *) "ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen"},
        {(char *) "UUID"},
        {(char *) "displayname"},
        {(char *) "perms"},
    },
    {
        {
            (char *)
            "BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits"
        },
        {(char *) "serialnum"},
        {(char *) "booktitle"},
        {(char *) "bookreleaseyear"},
        {(char *) "hits"},
    },
    {
        {(char *) "STOCK.serialnum, campus, instock,hits"},
        {(char *) "BOOK.serialnum"},
        {(char *) "instock"},
    },
    {
        {(char *) "UUID, serialnum, rentduration, rentdate, extended"},
        {(char *) "UUID"},
        {(char *) "serialnum"},
        {(char *) "rentdate"},
    },
    {
        {(char *) "UUID, UUID_ISSUER, serialnum, action, actiondate"},
        {(char *) "UUID"},
        {(char *) "serialnum"},
        {(char *) "actiondate"},
    },
    {
        {(char *) "account,sessionID,expiresAt "},
        {(char *) "account"},
        {(char *) "expiresAt"},
    }
};


enum khttp sanitize() {
    if (r.method != KMETHOD_GET)
        return KHTTP_405;
    if (r.page == PG__MAX)
        return KHTTP_404;
    return KHTTP_200;
}

int main(void) {
    enum khttp er;
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the INVENTORY if no page was found
    if (khttp_parse(&r, 0, 0, pages, PG__MAX, PG_BOOK) != KCGI_OK)
        return EXIT_FAILURE;
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);

        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    const enum statement STMT = get_stmts();
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_body(&r);
    khttp_puts(&r, pstms_data_top[STMT].stmt);
    for (int i = 0; switch_keys[STMT][i] != KEY__SWITCH__MAX; i++) {
        khttp_puts(&r, pstmts_switches[STMT][i].stmt);
    }
    khttp_free(&r);
    return EXIT_SUCCESS;
}
