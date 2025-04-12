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
    KEY_PG_SESSIONS,
    // FILTERS
    KEY_PAGE,
    KEY_LIMIT,
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
    KEY_FILTER_ME,
    KEY_FILTER_BY_ISSUER,
    KEY_FILTER_BY_ACTION,
    KEY_FILTER_FROM_DATE,
    KEY_FILTER_TO_DATE,
    KEY_FILTER_BY_SESSION,
    COOKIE_SESSIONID,
    KEY_PERMS_DETAILS,
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
    {NULL, "sessions"},
    // FILTERS
    {kvalid_uint, "page"},
    {kvalid_uint, "limit"},
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
    {NULL, "me"},
    {kvalid_string, "by_issuer"},
    {kvalid_string, "by_action"},
    {kvalid_date, "from_date"},
    {kvalid_date, "to_date"},
    {kvalid_int, "by_session"},
    {kvalid_stringne, "sessionID"},
    {NULL, "details"},
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
    STMTS_CATEGORY_CASCADE,
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

static const char *statement_string[STMTS__MAX] = {
    "publisher", "author", "lang", "action", "type", "campus", "role", "category", "category_cascade", "account",
    "book", "stock", "inventory", "history", "sessions"
};
static const char *rows[STMTS__MAX][9] = {
    {"publisherName"},
    {"authorName"},
    {"langcode"},
    {"actionName"},
    {"typeName"},
    {"campusName"},
    {"roleName", "perms"},
    {"categoryClass", "categoryName", "parentCategoryID"},
    {"categoryClass", "categoryName", "parentCategoryID"},
    {"UUID", "displayname", "pwhash", "campus", "role", "perm", "frozen"},
    {"serialnum", "type", "category", "categoryName", "publisher", "booktitle", "bookreleaseyear", "bookcover", "hits"},
    {"serialnum", "campus", "instock"},
    {"UUID", "serialnum", "rentduration", "rentdate", "extended"},
    {"UUID", "UUID_ISSUER", "serialnum", "IP", "action", "actiondate", "details"},
    {"account", "sessionID", "expiresAt"}
};
/*
 * Helper function to get the Statement for a specific page
 */
enum statement get_stmts() {
    enum key i;
    for (i = KEY_PG_PUBLISHER; i < KEY_PAGE && !(r.fieldmap[i]); i++) {
    }
    if (i == KEY_PAGE)
        i = r.page;
    if (i < PG_CATEGORY) {
        return (enum statement) i;
    } else if (i == PG_CATEGORY) {
        if (r.fieldmap[KEY_FILTER_CASCADE])
            return STMTS_CATEGORY_CASCADE;
        return STMTS_CATEGORY;
    }
    return i + 1;
}

/*
 * Array of sources(databases) and their access mode, we only have one which is our central database.
 * And since this microservice handles querying only, we open it as Read-Only
 */
struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx_data;
struct sqlbox_cfg cfg_data;
struct sqlbox *boxctx_count;
struct sqlbox_cfg cfg_count;
size_t dbid_data; // Database associated with a config and a context for query results
size_t dbid_count; // Database associated with a config and a context for the number of results
static struct sqlbox_pstmt pstmts_data[STMTS__MAX] = {
    {
        (char *)
        "SELECT publisherName "
        "FROM PUBLISHER "
        "LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(publisherName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY publisherName "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), publisherName) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT authorName "
        "FROM AUTHOR "
        "LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(authorName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) "
        "GROUP BY authorName ORDER BY IIF((?) = 'POPULAR', SUM(hits), authorName) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT langCode "
        "FROM LANG "
        "LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(langCode, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) "
        "GROUP BY langCode "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), langCode) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT actionName "
        "FROM ACTION "
        "WHERE instr(actionName,(?)) > 0 "
        "ORDER BY actionName "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT typeName "
        "FROM DOCTYPE "
        "LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(typeName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY typeName "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), typeName) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT campusName "
        "FROM CAMPUS "
        "LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus "
        "LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(campusName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "GROUP BY campusName "
        "ORDER BY campusName "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT roleName, perms "
        "FROM ROLE LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(roleName, (?)) > 0) "
        "AND ((?) = 'IGNORE_PERMS' OR perms = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "GROUP BY roleName, perms "
        "ORDER BY perms DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT categoryClass, categoryName, parentCategoryID "
        "FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
        "WHERE IIF((?) = 'ROOT',parentCategoryID IS NULL,TRUE) "
        "AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) "
        "AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) "
        "AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY categoryClass, categoryName, parentCategoryID "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), categoryClass) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "WITH RECURSIVE CategoryCascade AS (SELECT categoryName,categoryClass, parentCategoryID "
        "FROM CATEGORY "
        "LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
        "WHERE IIF((?) = 'ROOT',parentCategoryID IS NULL,TRUE) "
        "AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) "
        "AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) "
        "AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY categoryClass,categoryName,parentCategoryID "
        "UNION ALL "
        "SELECT c.categoryName,c.categoryClass, c.parentCategoryID "
        "FROM CATEGORY c "
        "INNER JOIN CategoryCascade ct ON IIF((?) = 'GET_PARENTS',"
        "c.categoryClass = ct.parentCategoryID,"
        "c.parentCategoryID = ct.categoryClass)) "
        "SELECT CATEGORY.categoryClass, CATEGORY.categoryName,CATEGORY.parentCategoryID "
        "FROM CATEGORY, CategoryCascade "
        "WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass "
        "GROUP BY CATEGORY.categoryClass, CATEGORY.categoryName, CATEGORY.parentCategoryID "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen "
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
        "AND ((?) = 'IGNORE_ID' OR ACCOUNT.UUID = (?)) "
        "AND ((?) = 'IGNORE_NAME' OR instr(displayname, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND ((?) = 'IGNORE_ROLE' OR role = (?)) "
        "AND ((?) = 'IGNORE_FREEZE' OR frozen = (?)) "
        "AND ((?) = 'IGNORE_SESSION' OR sessionID = (?)) "
        "GROUP BY ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen "
        "ORDER BY displayname "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID "
        "FROM CATEGORY "
        "WHERE IIF((?) = 'ROOT',"
        "parentCategoryID IS NULL,"
        "categoryClass = (?)) "
        "UNION ALL "
        "SELECT c.categoryClass, c.parentCategoryID "
        "FROM CATEGORY c "
        "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) "
        "SELECT BOOK.serialnum, type, category,categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "STOCK,"
        "CategoryCascade "
        "WHERE category = CategoryCascade.categoryClass "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND STOCK.serialnum = BOOK.serialnum "
        "AND CATEGORY.categoryClass = BOOK.category "
        "AND ((?) = 'IGNORE_ID' OR BOOK.serialnum = (?)) "
        "AND ((?) = 'IGNORE_NAME' OR instr(booktitle, (?))) "
        "AND ((?) = 'IGNORE_LANG' OR lang = (?)) "
        "AND ((?) = 'IGNORE_AUTHOR' OR instr(author, (?)) > 0) "
        "AND ((?) = 'IGNORE_TYPE' OR type = (?)) "
        "AND ((?) = 'IGNORE_PUBLISHER' OR instr(publisher, (?)) > 0) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_FROM_DATE' OR bookreleaseyear >= (?)) "
        "AND ((?) = 'IGNORE_TO_DATE' OR bookreleaseyear <= (?)) "
        "GROUP BY BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "ORDER BY IIF((?) = 'POPULAR', hits, booktitle) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT STOCK.serialnum,campus,instock "
        "FROM STOCK,BOOK "
        "WHERE STOCK.serialnum = BOOK.serialnum "
        "AND ((?) = 'IGNORE_BOOK' OR STOCK.serialnum = (?)) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND IIF((?) = 'AVAILABLE', instock > 0, TRUE) "
        "GROUP BY STOCK.serialnum, campus, instock,hits "
        "ORDER BY IIF((?) = 'POPULAR', hits, instock) DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT UUID, serialnum, rentduration, rentdate, extended "
        "FROM INVENTORY "
        "WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY UUID, serialnum, rentduration, rentdate, extended "
        "ORDER BY rentdate DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {

        (char *)
        "SELECT UUID,UUID_ISSUER,serialnum,IP,action,actiondate,details "
        "FROM HISTORY "
        "WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_ISSUER' OR UUID_ISSUER = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_ACTION' OR action = (?)) "
        "AND ((?) = 'IGNORE_FROM_DATE' OR actiondate >= datetime((?),'unixepoch')) "
        "AND ((?) = 'IGNORE_TO_DATE' OR actiondate <= datetime((?),'unixepoch')) "
        "GROUP BY UUID, UUID_ISSUER, serialnum, action, actiondate "
        "ORDER BY actiondate DESC "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT account,sessionID,expiresAt "
        "FROM SESSIONS "
        "WHERE ((?) = 'IGNORE_ID' OR sessionID = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR account = (?)) "
        "GROUP BY account,sessionID,expiresAt "
        "ORDER BY expiresAt DESC "
        "LIMIT (?) OFFSET (? * (?))"
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


static struct sqlbox_pstmt pstmts_count[STMTS__MAX] = {
    {
        (char *)
        "SELECT COUNT(DISTINCT publisherName) "
        "FROM PUBLISHER "
        "LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(publisherName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), publisherName) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT authorName) "
        "FROM AUTHOR "
        "LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(authorName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), authorName) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT langCode) "
        "FROM LANG "
        "LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(langCode, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), langCode) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT actionName) "
        "FROM ACTION "
        "WHERE instr(actionName,(?)) > 0 "
        "LIMIT (?) OFFSET (? * (?))"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT typeName) "
        "FROM DOCTYPE "
        "LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(typeName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), typeName) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT campusName) "
        "FROM CAMPUS "
        "LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus "
        "LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(campusName, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT roleName) "
        "FROM ROLE "
        "LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName "
        "WHERE ((?) = 'IGNORE_NAME' OR instr(roleName, (?)) > 0) "
        "AND ((?) = 'IGNORE_PERMS' OR perms = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT categoryClass) "
        "FROM CATEGORY "
        "LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
        "WHERE IIF((?) = 'ROOT',parentCategoryID IS NULL,TRUE) "
        "AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) "
        "AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) "
        "AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "ORDER BY IIF((?) = 'POPULAR', SUM(hits), categoryClass) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        " WITH RECURSIVE CategoryCascade AS (SELECT categoryName,categoryClass, parentCategoryID "
        "FROM CATEGORY "
        "LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
        "WHERE IIF((?) = 'ROOT',parentCategoryID IS NULL,TRUE) "
        "AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) "
        "AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) "
        "AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "GROUP BY categoryClass,categoryName,parentCategoryID "
        "UNION ALL "
        "SELECT c.categoryName,c.categoryClass, c.parentCategoryID "
        "FROM CATEGORY c "
        "INNER JOIN CategoryCascade ct ON IIF((?) = 'GET_PARENTS',"
        "c.categoryClass = ct.parentCategoryID,"
        "c.parentCategoryID = ct.categoryClass)) "
        "SELECT COUNT(DISTINCT CATEGORY.categoryClass) "
        "FROM CATEGORY, CategoryCascade "
        "WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT ACCOUNT.UUID) "
        "FROM ROLE,ACCOUNT "
        "LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
        "AND ((?) = 'IGNORE_ID' OR ACCOUNT.UUID = (?)) "
        "AND ((?) = 'IGNORE_NAME' OR instr(displayname, (?)) > 0) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND ((?) = 'IGNORE_ROLE' OR role = (?)) "
        "AND ((?) = 'IGNORE_FREEZE' OR frozen = (?)) "
        "AND ((?) = 'IGNORE_SESSION' OR sessionID = (?)) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID "
        "FROM CATEGORY "
        "WHERE IIF((?) = 'ROOT',parentCategoryID IS NULL,categoryClass = (?)) "
        "UNION ALL "
        "SELECT c.categoryClass, c.parentCategoryID "
        "FROM CATEGORY c "
        "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) "
        "SELECT COUNT(DISTINCT BOOK.serialnum) "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "STOCK,"
        "CategoryCascade "
        "WHERE category = CategoryCascade.categoryClass "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND STOCK.serialnum = BOOK.serialnum "
        "AND CATEGORY.categoryClass = BOOK.category "
        "AND ((?) = 'IGNORE_ID' OR BOOK.serialnum = (?)) "
        "AND ((?) = 'IGNORE_NAME' OR instr(booktitle, (?))) "
        "AND ((?) = 'IGNORE_LANG' OR lang = (?)) "
        "AND ((?) = 'IGNORE_AUTHOR' OR instr(author, (?)) > 0) "
        "AND ((?) = 'IGNORE_TYPE' OR type = (?)) "
        "AND ((?) = 'IGNORE_PUBLISHER' OR instr(publisher, (?)) > 0) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_FROM_DATE' OR bookreleaseyear >= (?)) "
        "AND ((?) = 'IGNORE_TO_DATE' OR bookreleaseyear <= (?)) "
        "ORDER BY IIF((?) = 'POPULAR', hits, booktitle) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT COALESCE(STOCK.serialnum, campus)) "
        "FROM STOCK,"
        "BOOK "
        "WHERE STOCK.serialnum = BOOK.serialnum "
        "AND ((?) = 'IGNORE_BOOK' OR STOCK.serialnum = (?)) "
        "AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) "
        "AND IIF((?) = 'AVAILABLE', instock > 0, TRUE) "
        "ORDER BY IIF((?) = 'POPULAR', hits, instock) DESC "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT COALESCE(UUID, serialnum, rentduration, rentdate, extended)) "
        "FROM INVENTORY "
        "WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {

        (char *)
        "SELECT COUNT(DISTINCT COALESCE(UUID,UUID_ISSUER,serialnum,action,actiondate,details)) "
        "FROM HISTORY "
        "WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) "
        "AND ((?) = 'IGNORE_ISSUER' OR UUID_ISSUER = (?)) "
        "AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) "
        "AND ((?) = 'IGNORE_ACTION' OR action = (?)) "
        "AND ((?) = 'IGNORE_FROM_DATE' OR actiondate >= datetime((?),'unixepoch')) "
        "AND ((?) = 'IGNORE_TO_DATE' OR actiondate <= datetime((?),'unixepoch')) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT COALESCE(account,sessionID,expiresAt)) "
        "FROM SESSIONS "
        "WHERE ((?) = 'IGNORE_ID' OR sessionID = (?)) "
        "AND ((?) = 'IGNORE_ACCOUNT' OR account = (?)) "
        "LIMIT (?) OFFSET (? * (?) * 0)"
    }

};

struct sqlbox_parm *parms; //Array of statement parameters
size_t parmsz;
/*
 * Allocates the context and source for the current operations
 */
void alloc_ctx_cfg() {
    memset(&cfg_data, 0, sizeof(struct sqlbox_cfg));
    cfg_data.msg.func_short = warnx;
    cfg_data.srcs.srcsz = 1;
    cfg_data.srcs.srcs = srcs;
    cfg_data.stmts.stmtsz = STMTS__MAX;
    cfg_data.stmts.stmts = pstmts_data;
    if ((boxctx_data = sqlbox_alloc(&cfg_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_data = sqlbox_open(boxctx_data, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");

    memset(&cfg_count, 0, sizeof(struct sqlbox_cfg));
    cfg_count.msg.func_short = warnx;
    cfg_count.srcs.srcsz = 1;
    cfg_count.srcs.srcs = srcs;
    cfg_count.stmts.stmtsz = STMTS__MAX - 4; // 3 is the number of reserved statements, without count
    cfg_count.stmts.stmts = pstmts_count;
    if ((boxctx_count = sqlbox_alloc(&cfg_count)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_data = sqlbox_open(boxctx_count, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}


/*
 * A struct holding the UUID and Permissions of the user issuing the call
 */
struct usr {
    char *UUID;
    char *disp_name;
    char *campus;
    char *role;
    char *sessionID;
    struct accperms perms;
    bool authenticated;
    bool frozen;
};

struct usr curr_usr = {
    .authenticated = false,
    .frozen = false,
    .UUID = NULL,
    .disp_name = NULL,
    .campus = NULL,
    .role = NULL,
    .sessionID = NULL,
    .perms = {0, 0, 0, 0, 0, 0, 0, 0}
};

void fill_user() {
    struct kpair *field;
    if (r.cookiemap[COOKIE_SESSIONID] || r.fieldmap[COOKIE_SESSIONID]) {
        if (r.cookiemap[COOKIE_SESSIONID])
            field = r.cookiemap[COOKIE_SESSIONID];
        else
            field = r.fieldmap[COOKIE_SESSIONID];
        size_t stmtid;
        size_t parmsz = 17;
        const struct sqlbox_parmset *res;
        struct sqlbox_parm parms[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_ID"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_NAME"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_BOOK"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_CAMPUS"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_ROLE"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_FREEZE"},
            {.type = SQLBOX_PARM_STRING, .sparm = ""},
            {.type = SQLBOX_PARM_STRING, .sparm = "SESSION"},
            {.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s},
            {.type = SQLBOX_PARM_INT, .iparm = 1},
            {.type = SQLBOX_PARM_INT, .iparm = 0},
            {.type = SQLBOX_PARM_INT, .iparm = 0},
        };
        if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, STMTS_ACCOUNT, parmsz, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
        if ((res = sqlbox_step(boxctx_data, stmtid)) == NULL)
            errx(EXIT_FAILURE, "sqlbox_step");
        if (res->psz != 0) {
            curr_usr.authenticated = true;
            kasprintf(&curr_usr.UUID, "%s", res->ps[0].sparm);

            kasprintf(&curr_usr.disp_name, "%s", res->ps[1].sparm);

            kasprintf(&curr_usr.campus, "%s", res->ps[3].sparm);

            kasprintf(&curr_usr.role, "%s", res->ps[4].sparm);

            kasprintf(&curr_usr.sessionID, "%s", field->parsed.s);

            curr_usr.perms = int_to_accperms((int) res->ps[5].iparm);

            curr_usr.frozen = res->ps[6].iparm;
        }
        sqlbox_finalise(boxctx_data, stmtid);
    }
}

/*
 * Fills the parm parameter array with the values that will replace the interrogation marks in the SQL statements
 *
 */

void fill_params(const enum statement STATEMENT) {
    struct kpair *field;
    switch (STATEMENT) {
        case STMTS_ACTION:
            parmsz = 4;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            break;
        case STMTS_PUBLISHER:
        case STMTS_AUTHOR:
        case STMTS_LANG:
        case STMTS_TYPE:
            parmsz = 8;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DEFAULT_SORT"
            };
            break;
        case STMTS_CAMPUS:
            parmsz = 9;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                             ? "IGNORE_ACCOUNT"
                             : "ACCOUNT"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };
            break;
        case STMTS_ROLE:
            parmsz = 9;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_PERM])) || field->valsz <= 0
                             ? "IGNORE_PERMS"
                             : "PERMS"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_BY_PERM])) ? field->parsed.i : 0
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                             ? "IGNORE_ACCOUNT"
                             : "ACCOUNT"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };

            break;
        case STMTS_CATEGORY:
            parmsz = 13;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));

            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_TREE] && (!((field = r.fieldmap[KEY_FILTER_BY_PARENT])) || field->valsz
                                                          <= 0))
                             ? "ROOT"
                             : "NOT_ROOT"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CLASS])) || field->valsz <= 0
                             ? "IGNORE_CLASS"
                             : "CLASS"
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CLASS])) ? field->parsed.s : ""
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_PARENT])) || field->valsz <= 0
                             ? "IGNORE_PARENT_CLASS"
                             : "PARENT_CLASS"
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DEFAULT_SORT"
            };
            break;
        case STMTS_CATEGORY_CASCADE:
            parmsz = 13;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));

            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_PARENT])) || field->valsz
                         <= 0
                             ? "ROOT"
                             : "NOT_ROOT"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CLASS])) || field->valsz <= 0
                             ? "IGNORE_CLASS"
                             : "CLASS"
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CLASS])) ? field->parsed.s : ""
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_PARENT])) || field->valsz <= 0
                             ? "IGNORE_PARENT_CLASS"
                             : "PARENT_CLASS"
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };

            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_GET_PARENTS]) ? "GET_PARENTS" : "DONT_GET_PARENTS"
            };
            break;
        case STMTS_ACCOUNT:
            parmsz = 17;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            if (r.fieldmap[KEY_FILTER_ME] && curr_usr.authenticated) {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = "SELF"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = curr_usr.UUID
                };
            } else {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ID])) || field->valsz <= 0
                                 ? "IGNORE_ID"
                                 : "ID"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ID])) ? field->parsed.s : ""
                };
            }

            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) || field->valsz <= 0
                             ? "IGNORE_CAMPUS"
                             : "CAMPUS"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ROLE])) || field->valsz <= 0
                             ? "IGNORE_ROLE"
                             : "ROLE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ROLE])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_FROZEN])) || field->valsz <= 0
                             ? "IGNORE_FREEZE"
                             : "FREEZE"
            };
            parms[11] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_FROZEN])) ? field->parsed.i : 0
            };

            parms[12] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_SESSION])) || field->valsz <= 0
                             ? "IGNORE_SESSION"
                             : "SESSION"
            };
            parms[13] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_BY_SESSION])) ? field->parsed.i : 0
            };
            break;
        case STMTS_BOOK:
            parmsz = 26;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CATEGORY])) || field->valsz <= 0 ? "ROOT" : "NOT_ROOT"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CATEGORY])) ? field->parsed.s : ""
            };

            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ID])) || field->valsz <= 0 ? "IGNORE_ID" : "ID"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ID])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_NAME])) || field->valsz <= 0
                             ? "IGNORE_NAME"
                             : "NAME"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_LANG])) || field->valsz <= 0
                             ? "IGNORE_LANG"
                             : "LANG"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_LANG])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_AUTHOR])) || field->valsz <= 0
                             ? "IGNORE_AUTHOR"
                             : "AUTHOR"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_AUTHOR])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_TYPE])) || field->valsz <= 0
                             ? "IGNORE_TYPE"
                             : "TYPE"
            };
            parms[11] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_TYPE])) ? field->parsed.s : ""
            };
            parms[12] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_PUBLISHER])) || field->valsz <= 0
                             ? "IGNORE_PUBLISHER"
                             : "PUBLISHER"
            };
            parms[13] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PUBLISHER])) ? field->parsed.s : ""
            };
            parms[14] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) || field->valsz <= 0
                             ? "IGNORE_CAMPUS"
                             : "CAMPUS"
            };
            parms[15] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };
            parms[16] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                             ? "IGNORE_ACCOUNT"
                             : "ACCOUNT"
            };
            parms[17] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };

            parms[18] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_FROM_YEAR])) || field->valsz <= 0
                             ? "IGNORE_FROM_DATE"
                             : "FROM_DATE"
            };
            parms[19] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_FROM_YEAR])) ? field->parsed.i : 0
            };
            parms[20] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_TO_YEAR])) || field->valsz <= 0
                             ? "IGNORE_TO_DATE"
                             : "TO_DATE"
            };
            parms[21] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_TO_YEAR])) ? field->parsed.i : 0
            };
            parms[22] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DEFAULT_SORT"
            };
            break;
        case STMTS_STOCK:
            parmsz = 9;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) || field->valsz <= 0
                             ? "IGNORE_CAMPUS"
                             : "CAMPUS"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_IGNORE_EMPTY]) ? "AVAILABLE" : "IGNORE_EMPTY"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DEFAULT_SORT"
            };
            break;
        case STMTS_INVENTORY:
            parmsz = 7;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));

            if (r.fieldmap[KEY_FILTER_ME] && curr_usr.authenticated) {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = "SELF"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = curr_usr.UUID
                };
            } else {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                                 ? "IGNORE_ACCOUNT"
                                 : "ACCOUNT"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
                };
            }

            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            break;
        case STMTS_HISTORY:
            parmsz = 15;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            if (r.fieldmap[KEY_FILTER_ME] && curr_usr.authenticated) {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = "SELF"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = curr_usr.UUID
                };
            } else {
                parms[0] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                                 ? "IGNORE_ACCOUNT"
                                 : "ACCOUNT"
                };
                parms[1] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
                };
            }
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ISSUER])) || field->valsz <= 0
                             ? "IGNORE_ISSUER"
                             : "ISSUER"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ISSUER])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_BOOK])) || field->valsz <= 0
                             ? "IGNORE_BOOK"
                             : "BOOK"

            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };

            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACTION])) || field->valsz <= 0
                             ? "IGNORE_ACTION"
                             : "ACTION"

            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACTION])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_FROM_DATE])) || field->valsz <= 0
                             ? "IGNORE_FROM_DATE"
                             : "FROM_DATE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_FROM_DATE])) ? field->parsed.i : 0
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_TO_DATE])) || field->valsz <= 0
                             ? "IGNORE_TO_DATE"
                             : "TO_DATE"
            };
            parms[11] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_TO_DATE])) ? field->parsed.i : 0
            };
            break;
        case STMTS_SESSIONS:

            parmsz = 7;
            parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ID])) || field->valsz <= 0
                             ? "IGNORE_ID"
                             : "ID"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ID])) ? field->parsed.s : ""
            };

            if (r.fieldmap[KEY_FILTER_ME] && curr_usr.authenticated) {
                parms[2] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = "SELF"
                };
                parms[3] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = curr_usr.UUID
                };
            } else {
                parms[2] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = !((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) || field->valsz <= 0
                                 ? "IGNORE_ACCOUNT"
                                 : "ACCOUNT"
                };
                parms[3] = (struct sqlbox_parm){
                    .type = SQLBOX_PARM_STRING,
                    .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
                };
            }
            break;
        default:
            errx(EXIT_FAILURE, "params");
    }


    parms[parmsz - 3] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT,
        .iparm = ((field = r.fieldmap[KEY_LIMIT])) ? ((field->parsed.i < 100) ? field->parsed.i : 100) : 20
    };
    parms[parmsz - 2] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT, .iparm = ((field = r.fieldmap[KEY_PAGE])) ? field->parsed.i : 0
    };

    parms[parmsz - 1] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT,
        .iparm = ((field = r.fieldmap[KEY_LIMIT])) ? ((field->parsed.i < 100) ? field->parsed.i : 100) : 20
    };
}

void get_cat_children(const char *class) {
    size_t stmtid;
    size_t parmsz2 = 13;
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms2[] = {
        {.type = SQLBOX_PARM_STRING, .sparm = "NOT_ROOT"},
        {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_NAME"},
        {.type = SQLBOX_PARM_STRING, .sparm = ""},
        {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_CLASS"},
        {.type = SQLBOX_PARM_STRING, .sparm = ""},
        {.type = SQLBOX_PARM_STRING, .sparm = "DONT_IGNORE"},
        {.type = SQLBOX_PARM_STRING, .sparm = class},
        {.type = SQLBOX_PARM_STRING, .sparm = "IGNORE_BOOK"},
        {.type = SQLBOX_PARM_STRING, .sparm = ""},
        {.type = SQLBOX_PARM_STRING, .sparm = "DONT_IGNORE"},
        {.type = SQLBOX_PARM_INT, .iparm = -1},
        {.type = SQLBOX_PARM_INT, .iparm = 0},
        {.type = SQLBOX_PARM_INT, .iparm = 0}
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, STMTS_CATEGORY, parmsz2, parms2, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    while ((res = sqlbox_step(boxctx_data, stmtid)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(&req);
        for (int i = 0; i < (int) res->psz; ++i) {
            switch (res->ps[i].type) {
                case SQLBOX_PARM_INT:
                    kjson_putintp(&req, rows[STMTS_CATEGORY][i], res->ps[i].iparm);
                    break;
                case SQLBOX_PARM_STRING:
                    kjson_putstringp(&req, rows[STMTS_CATEGORY][i], res->ps[i].sparm);
                    break;
                case SQLBOX_PARM_FLOAT:
                    kjson_putdoublep(&req, rows[STMTS_CATEGORY][i], res->ps[i].fparm);
                    break;
                case SQLBOX_PARM_BLOB:
                    kjson_putstringp(&req, rows[STMTS_CATEGORY][i], res->ps[i].bparm);
                    break;
                case SQLBOX_PARM_NULL:
                    kjson_putnullp(&req, rows[STMTS_CATEGORY][i]);
                    break;
                default:
                    break;
            }
        }
        if (r.fieldmap[KEY_FILTER_TREE]) {
            kjson_arrayp_open(&req, "children");
            get_cat_children(res->ps[0].sparm);
            kjson_array_close(&req);
        }
        kjson_obj_close(&req);
    }
    if (!sqlbox_finalise(boxctx_data, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
}

void process(const enum statement STATEMENT) {
    size_t stmtid_data;
    size_t stmtid_count;
    const struct sqlbox_parmset *res;
    if (!(stmtid_data = sqlbox_prepare_bind(boxctx_data, dbid_data, STATEMENT, parmsz, parms, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");

    if (!(stmtid_count = sqlbox_prepare_bind(boxctx_count, dbid_count, STATEMENT, parmsz, parms,
                                             SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_objp_open(&req, "user");
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    if (curr_usr.authenticated) {
        kjson_putstringp(&req, "UUID", curr_usr.UUID);
        kjson_putstringp(&req, "disp_name", curr_usr.disp_name);
        kjson_putstringp(&req, "campus", curr_usr.campus);
        kjson_putstringp(&req, "role", curr_usr.role);
        kjson_putboolp(&req, "frozen", curr_usr.frozen);

        kjson_objp_open(&req, "perms");
        kjson_putintp(&req, "numeric", curr_usr.perms.numeric);
        kjson_putboolp(&req, "admin", curr_usr.perms.admin);
        kjson_putboolp(&req, "staff", curr_usr.perms.staff);
        kjson_putboolp(&req, "manage_stock", curr_usr.perms.manage_stock);
        kjson_putboolp(&req, "manage_inventories", curr_usr.perms.manage_inventories);
        kjson_putboolp(&req, "see_accounts", curr_usr.perms.see_accounts);
        kjson_putboolp(&req, "monitor_history", curr_usr.perms.monitor_history);
        kjson_putboolp(&req, "has_inventory", curr_usr.perms.has_inventory);
        kjson_obj_close(&req);
    }
    kjson_obj_close(&req);

    kjson_arrayp_open(&req, "res");
    while ((res = sqlbox_step(boxctx_data, stmtid_data)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(&req);
        for (int i = 0; i < (int) res->psz; ++i) {
            switch (res->ps[i].type) {
                case SQLBOX_PARM_INT:
                    if (STATEMENT == STMTS_ROLE && r.fieldmap[KEY_PERMS_DETAILS]) {
                        struct accperms perms = int_to_accperms((int) res->ps[i].iparm);
                        kjson_objp_open(&req, rows[STATEMENT][i]);
                        kjson_putintp(&req, "numerical", res->ps[i].iparm);
                        kjson_putboolp(&req, "admin", perms.admin);
                        kjson_putboolp(&req, "staff", perms.staff);
                        kjson_putboolp(&req, "manage_stock", perms.manage_stock);
                        kjson_putboolp(&req, "manage_inventories", perms.manage_inventories);
                        kjson_putboolp(&req, "see_accounts", perms.see_accounts);
                        kjson_putboolp(&req, "monitor_history", perms.monitor_history);
                        kjson_putboolp(&req, "has_inventory", perms.has_inventory);
                        kjson_obj_close(&req);
                    } else
                        kjson_putintp(&req, rows[STATEMENT][i], res->ps[i].iparm);
                    break;
                case SQLBOX_PARM_STRING:
                    kjson_putstringp(&req, rows[STATEMENT][i], res->ps[i].sparm);
                    break;
                case SQLBOX_PARM_FLOAT:
                    kjson_putdoublep(&req, rows[STATEMENT][i], res->ps[i].fparm);
                    break;
                case SQLBOX_PARM_BLOB:
                    kjson_putstringp(&req, rows[STATEMENT][i], res->ps[i].bparm);
                    break;
                case SQLBOX_PARM_NULL:
                    kjson_putnullp(&req, rows[STATEMENT][i]);
                    break;
                default:
                    break;
            }
        }
        if (r.fieldmap[KEY_FILTER_TREE] && STATEMENT == STMTS_CATEGORY) {
            kjson_arrayp_open(&req, "children");
            get_cat_children(res->ps[0].sparm);
            kjson_array_close(&req);
        }
        if (STATEMENT == STMTS_BOOK) {
            size_t stmtid_book_data;
            size_t parmsz_book = 1;
            const struct sqlbox_parmset *res_book;
            struct sqlbox_parm parms_book[] = {
                {.type = SQLBOX_PARM_STRING, .sparm = res->ps[0].sparm},
            };
            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, __STMT_BOOK_AUTHORED__, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "authors");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code == SQLBOX_CODE_OK
                   && res_book->psz != 0)
                kjson_putstring(&req, res_book->ps[0].sparm);
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");

            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, __STMT_BOOK_LANGUAGES__, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "langs");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code == SQLBOX_CODE_OK
                   && res_book->psz != 0)
                kjson_putstring(&req, res_book->ps[0].sparm);
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");

            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, __STMT_BOOK_STOCK__, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "stock");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code == SQLBOX_CODE_OK
                   && res_book->psz != 0) {
                kjson_obj_open(&req);
                kjson_putstringp(&req,"campus", res_book->ps[0].sparm);
                kjson_putintp(&req,"stock", res_book->ps[1].iparm);
                kjson_obj_close(&req);
            }
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");
        }
        kjson_obj_close(&req);
    }
    if (!sqlbox_finalise(boxctx_data, stmtid_data))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    kjson_array_close(&req);
    if ((res = sqlbox_step(boxctx_count, stmtid_count)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    kjson_putintp(&req, "nbrres", res->ps[0].iparm);
    if (!sqlbox_finalise(boxctx_count, stmtid_count))
        errx(EXIT_FAILURE, "sqlbox_finalise");

    kjson_obj_close(&req);
    kjson_close(&req);
}

void save(const enum statement STATEMENT, const bool failed) {
    char *requestDesc = NULL;
    if (!failed) {
        kasprintf(&requestDesc, "Stmt:%s, Parms:(", statement_string[STATEMENT]);
        for (int i = 0; i < (int) parmsz; ++i) {
            switch (parms[i].type) {
                case SQLBOX_PARM_INT:
                    kasprintf(&requestDesc, "%s\"%lld\",", requestDesc, parms[i].iparm);
                    break;
                case SQLBOX_PARM_STRING:
                    if (strlen(parms[i].sparm) > 0)
                        kasprintf(&requestDesc, "%s\"%s\",", requestDesc, parms[i].sparm);
                    break;
                case SQLBOX_PARM_FLOAT:
                    kasprintf(&requestDesc, "%s\"%f\",", requestDesc, parms[i].fparm);
                    break;
                default:
                    break;
            }
        }
        kasprintf(&requestDesc, "%s)", requestDesc);
    } else {
        kasprintf(&requestDesc, "Stmt:%s, ACCESS DENIED", statement_string[STATEMENT]);
    }
    size_t parmsz_save = 3;
    struct sqlbox_parm parms_save[] = {
        {
            .type = (curr_usr.UUID != NULL) ? SQLBOX_PARM_STRING : SQLBOX_PARM_NULL,
            .sparm = curr_usr.UUID
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.remote
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = requestDesc
        },
    };
    if (sqlbox_exec(boxctx_data, dbid_data, __STMT_STORE__, parmsz_save, parms_save,SQLBOX_STMT_CONSTRAINT) !=
        SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
}

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
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG_BOOK) != KCGI_OK)
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
    alloc_ctx_cfg();
    fill_user();
    if ((STMT == STMTS_HISTORY || STMT == STMTS_ACCOUNT || STMT == STMTS_SESSIONS || STMT == STMTS_INVENTORY)) {
        if (!curr_usr.authenticated)
            goto access_denied;
        if (!curr_usr.perms.admin && !curr_usr.perms.staff) {
            if (!r.fieldmap[KEY_FILTER_ME]) {
                if (!((curr_usr.perms.monitor_history && STMT == STMTS_HISTORY) || (
                          curr_usr.perms.manage_inventories && STMT == STMTS_INVENTORY) || (
                          curr_usr.perms.see_accounts && STMT == STMTS_ACCOUNT))) {
                    goto access_denied;
                }
            } else if (STMT == STMTS_INVENTORY && !curr_usr.perms.has_inventory) {
                goto access_denied;
            }
        }
    }
    fill_params(STMT);
    process(STMT);
    save(STMT,false);
    goto cleanup;
access_denied:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_403]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    kjson_putstringp(&req, "error", "You don't have the permissions to access this ressource");
    kjson_obj_close(&req);
    kjson_close(&req);
    save(STMT,true);
cleanup:
    khttp_free(&r);
    sqlbox_free(boxctx_data);
    sqlbox_free(boxctx_count);
    return EXIT_SUCCESS;
}
