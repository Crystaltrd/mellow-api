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

static const char *rows[STMTS__MAX][10] = {
    {"publisherName",NULL},
    {"authorName",NULL},
    {"langcode",NULL},
    {"actionName",NULL},
    {"typeName",NULL},
    {"campusName",NULL},
    {"roleName", "perms",NULL},
    {"categoryClass", "categoryName", "parentCategoryID",NULL},
    {"UUID", "displayname", "pwhash", "campus", "role", "perm", "frozen",NULL},
    {
        "BOOK.serialnum", "type", "category", "categoryName", "publisher", "booktitle", "bookreleaseyear", "bookcover",
        "hits",NULL
    },
    {"STOCK.serialnum", "campus", "instock",NULL},
    {"UUID", "serialnum", "rentduration", "rentdate", "extended",NULL},
    {"UUID", "UUID_ISSUER", "serialnum", "IP", "action", "actiondate", "details",NULL},
    {"account", "sessionID", "expiresAt",NULL}
};
static char *pstms_data_top[STMTS__MAX] = {
    {
        (char *)
        "FROM PUBLISHER "
        "LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName "
    },
    {
        (char *)
        "FROM AUTHOR "
        "LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
    },
    {
        (char *)
        "FROM LANG "
        "LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang "
        "LEFT JOIN BOOK B ON B.serialnum = A.serialnum "
    },
    {
        (char *)
        "FROM ACTION "
    },
    {
        (char *)
        "FROM DOCTYPE "
        "LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type "
    },
    {
        (char *)
        "FROM CAMPUS "
        "LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus "
        "LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus "
    },
    {
        (char *)
        "FROM ROLE LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName "
    },
    {
        (char *)
        "FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category "
    },

    {
        (char *)
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
    },
    {
        (char *)
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
        "FROM HISTORY "
    },
    {
        (char *)
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
    STMT__MAX
};

static struct sqlbox_pstmt pstmts[STMT__MAX] = {
    {(char *) ""},
    {(char *) ""},
    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen "
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
        "AND sessionID = (?) "
        "GROUP BY ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen "
    },
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'EDIT',datetime('now','localtime'),(?))"
    },
    {
        (char *)
        "SELECT categoryClass, categoryName, parentCategoryID "
        "FROM CATEGORY "
        "WHERE parentCategoryID = (?) "
        "ORDER BY categoryClass DESC"
    }
};
static char *pstmts_switches[STMTS__MAX][10] = {
    {
        "instr(publisherName,(?)) > 0",
        "serialnum = (?)"
    },
    {
        "instr(authorName,(?)) > 0",
        "A.serialnum = (?)"
    },
    {
        "instr(langCode,(?)) > 0",
        "A.serialnum = (?)"
    },
    {
        "instr(actionName,(?)) > 0"
    },
    {
        "instr(typeName,(?)) > 0",
        "serialnum = (?)"
    },
    {
        "instr(campusName,(?)) > 0",
        "serialnum = (?)",
        "UUID = (?)"
    },
    {
        "instr(roleName,(?)) > 0",
        "perms = (?)",
        "UUID = (?)"
    },
    {
        "parentCategoryID IS NULL",
        "parentCategoryID = (?)",
        "instr(categoryName,(?)) > 0",
        "categoryClass = (?)",
        "serialnum = (?)",
    },
    {
        "ACCOUNT.UUID = (?)",
        "instr(displayname, (?))> 0",
        "serialnum = (?)",
        "campus = (?)",
        "role = (?)",
        "frozen = (?)",
        "sessionID = (?)",
    },
    {
        "BOOK.serialnum = (?)",
        "instr(booktitle, (?))",
        "lang = (?)",
        "instr(author, (?)) > 0",
        "type = (?)",
        "instr(publisher, (?)) > 0",
        "campus = (?)",
        "UUID = (?)",
        "bookreleaseyear >= (?)",
        "bookreleaseyear <= (?)",
    },
    {
        "STOCK.serialnum = (?)",
        "campus = (?)",
        "instock > 0",
    },
    {
        "UUID = (?)",
        "serialnum = (?)"
    },
    {
        "UUID = (?)",
        "UUID_ISSUER = (?)",
        "serialnum = (?)",
        "action = (?)",
        "actiondate >= datetime((?),'unixepoch')",
        "actiondate <= datetime((?),'unixepoch')",
    },
    {
        "sessionID = (?)",
        "account = (?)",
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
static char *pstmts_bottom[STMTS__MAX][5] = {
    {
        "publisherName",
        "SUM(hits)",
        "publisherName",
    },
    {
        "authorName",
        "SUM(hits)",
        "authorName",
    },
    {
        "langCode",
        "SUM(hits)",
        "langCode",
    },
    {
        "actionName",
        "actionName",
    },
    {
        "typeName",
        "SUM(hits)",
        "typeName",
    },
    {
        "campusName ",
        "campusName ",
    },
    {
        "roleName, perms",
        "perms",
        "roleName",
    },
    {
        "categoryClass, categoryName, parentCategoryID",
        "SUM(hits)",
        "categoryClass",
        "categoryName",
    },
    {

        "ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen",
        "UUID",
        "displayname",
        "perms",
    },
    {

        "BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits",
        "serialnum",
        "booktitle",
        "bookreleaseyear",
        "hits",
    },
    {
        "STOCK.serialnum, campus, instock,hits",
        "BOOK.serialnum",
        "instock",
    },
    {
        "UUID, serialnum, rentduration, rentdate, extended",
        "UUID",
        "serialnum",
        "rentdate",
    },
    {
        "UUID, UUID_ISSUER, serialnum, action, actiondate",
        "UUID",
        "serialnum",
        "actiondate",
    },
    {
        "account,sessionID,expiresAt ",
        "account",
        "expiresAt",
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
        return KHTTP_400;
    return KHTTP_200;
}

int build_stmt(enum statement_pieces STMT) {
    int n = 0;
    if (STMT == STMTS_BOOK) {
        if (r.fieldmap[KEY_SWITCH_CLASS]) {
            kasprintf(&pstmts[STMT_DATA].stmt,
                      "%s""WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID "
                      "FROM CATEGORY "
                      "WHERE "
                      "categoryClass = (?)) "
                      "UNION ALL "
                      "SELECT c.categoryClass, c.parentCategoryID "
                      "FROM CATEGORY c "
                      "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) ",
                      pstmts[STMT_DATA].stmt);
            n++;
        } else {
            kasprintf(&pstmts[STMT_DATA].stmt,
                      "%s""WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID "
                      "FROM CATEGORY "
                      "WHERE "
                      "parentCategoryID IS NULL "
                      "UNION ALL "
                      "SELECT c.categoryClass, c.parentCategoryID "
                      "FROM CATEGORY c "
                      "INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) ",
                      pstmts[STMT_DATA].stmt);
        }
    }
    kasprintf(&pstmts[STMT_DATA].stmt, "%s SELECT", pstmts[STMT_DATA].stmt);
    kasprintf(&pstmts[STMT_COUNT].stmt, "%s SELECT COUNT(DISTINCT", pstmts[STMT_DATA].stmt);
    for (int i = 0; rows[STMT][i] != NULL; ++i) {
        if (i == 0) {
            kasprintf(&pstmts[STMT_DATA].stmt, "%s %s", pstmts[STMT_DATA].stmt, rows[STMT][i]);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s %s", pstmts[STMT_COUNT].stmt, rows[STMT][i]);
        }
        kasprintf(&pstmts[STMT_DATA].stmt, "%s,%s", pstmts[STMT_DATA].stmt, rows[STMT][i]);
        kasprintf(&pstmts[STMT_COUNT].stmt, "%s,%s", pstmts[STMT_COUNT].stmt, rows[STMT][i]);
    }
    kasprintf(&pstmts[STMT_DATA].stmt, "%s %s", pstmts[STMT_DATA].stmt, pstms_data_top[STMT]);
    kasprintf(&pstmts[STMT_COUNT].stmt, "%s) %s", pstmts[STMT_COUNT].stmt, pstms_data_top[STMT]);
    bool flag = false;
    for (int i = 0; switch_keys[STMT][i] != KEY__MAX; i++) {
        if (r.fieldmap[switch_keys[STMT][i]]) {
            n++;
            if (!flag) {
                if (!strstr(pstms_data_top[STMT], "WHERE")) {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" WHERE ", pstmts[STMT_DATA].stmt);
                    kasprintf(&pstmts[STMT_COUNT].stmt, "%s"" WHERE ", pstmts[STMT_COUNT].stmt);
                } else {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" AND ", pstmts[STMT_DATA].stmt);
                    kasprintf(&pstmts[STMT_COUNT].stmt, "%s"" AND ", pstmts[STMT_COUNT].stmt);
                }
                flag = true;
            } else {
                kasprintf(&pstmts[STMT_DATA].stmt, "%s"" AND ", pstmts[STMT_DATA].stmt);
                kasprintf(&pstmts[STMT_COUNT].stmt, "%s"" AND ", pstmts[STMT_COUNT].stmt);
            }
            kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt, pstmts_switches[STMT][i]);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s%s", pstmts[STMT_COUNT].stmt, pstmts_switches[STMT][i]);
        }
    }
    flag = false;
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; i++) {
        if (bottom_keys[STMT][i] == KEY_MANDATORY_GROUP_BY) {
            kasprintf(&pstmts[STMT_DATA].stmt, "%s"" GROUP BY ", pstmts[STMT_DATA].stmt);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s"" GROUP BY ", pstmts[STMT_COUNT].stmt);
            kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt, pstmts_bottom[STMT][i]);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s%s", pstmts[STMT_COUNT].stmt, pstmts_bottom[STMT][i]);
        } else {
            if (r.fieldmap[bottom_keys[STMT][i]]) {
                n++;
                if (!flag) {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" ORDER BY ", pstmts[STMT_DATA].stmt);
                    kasprintf(&pstmts[STMT_COUNT].stmt, "%s"" ORDER BY ", pstmts[STMT_COUNT].stmt);
                    flag = true;
                } else {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"",", pstmts[STMT_DATA].stmt);
                    kasprintf(&pstmts[STMT_COUNT].stmt, "%s"",", pstmts[STMT_COUNT].stmt);
                }
                kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt, pstmts_bottom[STMT][i]);
                kasprintf(&pstmts[STMT_COUNT].stmt, "%s%s", pstmts[STMT_COUNT].stmt, pstmts_bottom[STMT][i]);
                kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt,
                          (r.fieldmap[bottom_keys[STMT][i]]->parsed.i == 0) ? " DESC" : " ASC");
                kasprintf(&pstmts[STMT_COUNT].stmt, "%s%s", pstmts[STMT_COUNT].stmt,
                          (r.fieldmap[bottom_keys[STMT][i]]->parsed.i == 0) ? " DESC" : " ASC");
            }
        }
    }
    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" LIMIT(?),(? * ?)", pstmts[STMT_DATA].stmt);
    return n += 3;
}

struct sqlbox_parm *parms;
size_t parmsz;

void fill_parms(enum statement_pieces STMT) {
    parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
    int n = 0;
    struct kpair *field;
    if (STMT == STMTS_BOOK && r.fieldmap[KEY_SWITCH_CLASS]) {
        parms[0].sparm = r.fieldmap[KEY_SWITCH_CLASS]->parsed.s;
        parms[0].type = SQLBOX_PARM_STRING;
        n++;
    }
    for (int i = 0; switch_keys[STMT][i] != KEY__MAX; i++) {
        if ((field = r.fieldmap[switch_keys[STMT][i]])) {
            switch (field->type) {
                case KPAIR_INTEGER:
                    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = field->parsed.i};
                    break;
                case KPAIR_STRING:
                    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s};
                    break;
                default:
                    break;
            }
        }
    }
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; i++) {
        if ((field = r.fieldmap[bottom_keys[STMT][i]])) {
            switch (field->type) {
                case KPAIR_INTEGER:
                    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = field->parsed.i};
                    break;
                case KPAIR_STRING:
                    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s};
                    break;
                default:
                    break;
            }
        }
    }
    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = 0};
    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = 0};
    parms[n++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = 0};
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
    parmsz = build_stmt(STMT);
    fill_parms(STMT);
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_body(&r);
    for (int i = 0; i < parmsz; ++i) {
        if (parms[i].type == SQLBOX_PARM_STRING) {
            khttp_puts(&r,parms[i].sparm);
            khttp_puts(&r,"\r\n");
        }
    }
    khttp_puts(&r, pstmts[STMT_DATA].stmt);
    khttp_free(&r);
    return EXIT_SUCCESS;
}
