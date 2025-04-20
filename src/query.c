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
enum statement_pieces {
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
    STMTS__MAX
};

/*
 * Helper function to get the Statement for a specific page
 */
enum statement_pieces get_stmts() {
    return (enum statement_pieces) r.page;
}

static struct sqlbox_pstmt pstms_data_top[STMTS__MAX] = {
    {
        (char *)
        "SELECT publisherName "
        "FROM PUBLISHER "
        "LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName "
    },
    {
        (char *)
        "SELECT authorName "
        "FROM AUTHOR "
        "LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
    },
    {
        (char *)
        "SELECT langCode "
        "FROM LANG "
        "LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
    },
    {
        (char *)
        "SELECT actionName "
        "FROM ACTION "
    },
    {
        (char *)
        "SELECT typeName "
        "FROM DOCTYPE "
        "LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type "
    },
    {
        (char *)
        "SELECT campusName "
        "FROM CAMPUS "
        "LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus "
        "LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus "
    },
    {
        (char *)
        "SELECT roleName, perms "
        "FROM ROLE LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName "
    },
    {
        (char *)
        "SELECT categoryClass, categoryName, parentCategoryID "
        "FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
    },

    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen "
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
    },
    {
        (char *)
        "SELECT BOOK.serialnum, type, category,categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "STOCK,"
        "CategoryCascade "
        "WHERE category = CategoryCascade.categoryClass "
        "AND CATEGORY.categoryClass = BOOK.category "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND STOCK.serialnum = BOOK.serialnum "
    },
    {
        (char *)
        "SELECT STOCK.serialnum,campus,instock "
        "FROM STOCK,BOOK "
        "WHERE STOCK.serialnum = BOOK.serialnum "
    },
    {
        (char *)
        "SELECT UUID, serialnum, rentduration, rentdate, extended "
        "FROM INVENTORY "
    },
    {

        (char *)
        "SELECT UUID,UUID_ISSUER,serialnum,IP,action,actiondate,details "
        "FROM HISTORY "
    },
    {
        (char *)
        "SELECT account,sessionID,expiresAt "
        "FROM SESSIONS "
    },
};

enum key {
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
    KEY_LIMIT,
    KEY_OFFSET,
    KEY_CASCADE,
    KEY_TREE,
    KEY_MANDATORY_GROUP_BY,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "name"},
    {kvalid_stringne, "serialnum"},
    {kvalid_stringne, "uuid"},
    {kvalid_stringne, "perms"},
    {NULL, "root"},
    {kvalid_stringne, "parent"},
    {kvalid_stringne, "class"},
    {kvalid_stringne, "campus"},
    {kvalid_stringne, "role"},
    {kvalid_bit, "frozen"},
    {kvalid_stringne, "sessionid"},
    {kvalid_stringne, "lang"},
    {kvalid_stringne, "author"},
    {kvalid_stringne, "publisher"},
    {kvalid_stringne, "type"},
    {kvalid_int, "upperyear"},
    {kvalid_int, "loweryear"},
    {kvalid_bit, "notempty"},
    {kvalid_stringne, "issuer"},
    {kvalid_stringne, "action"},
    {kvalid_date, "upperdate"},
    {kvalid_date, "lowerdate"},
    {kvalid_int, "order_hits"},
    {kvalid_int, "order_name"},
    {kvalid_int, "order_perms"},
    {kvalid_int, "order_class"},
    {kvalid_int, "order_uuid"},
    {kvalid_int, "order_serialnum"},
    {kvalid_int, "order_date"},
    {kvalid_int, "order_stock"},
    {kvalid_int, "limit"},
    {kvalid_int, "page"},
    {NULL, "cascade"},
    {NULL, "tree"},
};

enum statement {
    STMT_DATA,
    STMT_COUNT,
    STMT_LOGIN,
    STMT_SAVE,
    STMT_CATEGORY_CHILD,
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

static enum key switch_keys[STMTS__MAX][11] = {

    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {KEY_SWITCH_NAME, KEY__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY_SWITCH_UUID, KEY__MAX},
    {KEY_SWITCH_NAME, KEY_SWITCH_PERMS, KEY_SWITCH_UUID, KEY__MAX},
    {KEY_SWITCH_ROOT, KEY_SWITCH_PARENT, KEY_SWITCH_NAME, KEY_SWITCH_CLASS, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {
        KEY_SWITCH_UUID, KEY_SWITCH_NAME, KEY_SWITCH_SERIALNUM, KEY_SWITCH_CAMPUS, KEY_SWITCH_ROLE, KEY_SWITCH_FROZEN,
        KEY_SWITCH_SESSIONID, KEY__MAX
    },
    {
        KEY_SWITCH_SERIALNUM, KEY_SWITCH_NAME, KEY_SWITCH_LANG, KEY_SWITCH_AUTHOR, KEY_SWITCH_ROLE,
        KEY_SWITCH_PUBLISHER, KEY_SWITCH_CAMPUS, KEY_SWITCH_UUID, KEY_SWITCH_UPPERYEAR, KEY_SWITCH_LOWERYEAR,
        KEY__MAX
    },
    {KEY_SWITCH_SERIALNUM, KEY_SWITCH_CAMPUS, KEY_SWITCH_NOTEMPTY, KEY__MAX},
    {KEY_SWITCH_UUID, KEY_SWITCH_SERIALNUM, KEY__MAX},
    {
        KEY_SWITCH_UUID, KEY_SWITCH_ISSUER, KEY_SWITCH_SERIALNUM, KEY_SWITCH_ACTION, KEY_SWITCH_UPPERYEAR,
        KEY_SWITCH_LOWERDATE, KEY__MAX
    },
    {KEY_SWITCH_SESSIONID, KEY_SWITCH_UUID, KEY__MAX}
};


static enum key bottom_keys[STMTS__MAX][8] = {
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_NAME,
        KEY__MAX
    },

    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_PERMS,
        KEY_ORDER_NAME,
        KEY__MAX
    },

    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_HITS,
        KEY_ORDER_CLASS,
        KEY_ORDER_NAME,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_NAME,
        KEY_ORDER_PERMS,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_NAME,
        KEY_ORDER_DATE,
        KEY_ORDER_HITS,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_STOCK,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_DATE,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_SERIALNUM,
        KEY_ORDER_DATE,
        KEY__MAX
    },
    {
        KEY_MANDATORY_GROUP_BY,
        KEY_ORDER_UUID,
        KEY_ORDER_DATE,
        KEY__MAX
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
        {(char *) "actionName"},
        {(char *) "actionName"},
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
    if (r.fieldmap[KEY_SWITCH_ROOT] && r.fieldmap[KEY_SWITCH_PARENT])
        return KHTTP_400;
    if (r.fieldmap[KEY_TREE] && r.fieldmap[KEY_CASCADE])
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

        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    const enum statement_pieces STMT = get_stmts();
    char *stmt = "";
    char *stmt_cat = "";
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_body(&r);
    if (STMT == STMTS_BOOK) {
        if (r.fieldmap[KEY_SWITCH_CLASS]) {
            kasprintf(&stmt, "%s""WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID "
                      "FROM CATEGORY "
                      "WHERE "
                      "categoryClass = (?)) "
                      "UNION ALL "
                      "SELECT c.categoryClass, c.parentCategoryID "
                      "FROM CATEGORY c "
                      "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) ", stmt);
        } else {
            kasprintf(&stmt, "%s""WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID,stmt "
                      "FROM CATEGORY "
                      "WHERE "
                      "parentCategoryID IS NULL "
                      "UNION ALL "
                      "SELECT c.categoryClass, c.parentCategoryID "
                      "FROM CATEGORY c "
                      "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) ", stmt);
        }
    }
    kasprintf(&stmt, "%s%s", stmt, pstms_data_top[STMT].stmt);
    bool flag = false;
    for (int i = 0; switch_keys[STMT][i] != KEY__MAX; i++) {
        if (r.fieldmap[switch_keys[STMT][i]]) {
            if (!flag) {
                if (!strstr(pstms_data_top[STMT].stmt, "WHERE")) {
                    kasprintf(&stmt, "%s"" WHERE ", stmt);
                } else {
                    kasprintf(&stmt, "%s"" AND ", stmt);
                }
                flag = true;
            } else {
                kasprintf(&stmt, "%s"" AND ", stmt);
            }
            kasprintf(&stmt, "%s%s", stmt, pstmts_switches[STMT][i].stmt);
        }
        if (switch_keys[STMT][i] == KEY_SWITCH_PARENT && (r.fieldmap[KEY_TREE] || r.fieldmap[KEY_CASCADE]))
            kasprintf(&stmt_cat, "%sWHERE parentCategoryID = (?)", stmt);
    }
    flag = false;
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; i++) {
        if (bottom_keys[STMT][i] == KEY_MANDATORY_GROUP_BY) {
            kasprintf(&stmt, "%s"" GROUP BY ", stmt);
            kasprintf(&stmt, "%s%s", stmt, pstmts_bottom[STMT][i].stmt);
        } else {
            if (r.fieldmap[bottom_keys[STMT][i]]) {
                if (!flag) {
                    kasprintf(&stmt, "%s"" ORDER BY ", stmt);
                    flag = true;
                } else {
                    kasprintf(&stmt, "%s"",", stmt);
                }
                kasprintf(&stmt, "%s%s", stmt, pstmts_bottom[STMT][i].stmt);
                kasprintf(&stmt, "%s%s", stmt, (r.fieldmap[bottom_keys[STMT][i]]->parsed.i == 0) ? " DESC" : " ASC");
            }
        }
    }
    kasprintf(&stmt, "%s"" LIMIT(?),(? * ?)", stmt);
    khttp_puts(&r, stmt_cat);
    khttp_free(&r);
    return EXIT_SUCCESS;
}
