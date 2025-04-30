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
    {"ACCOUNT.UUID", "displayname", "pwhash", "campus", "role", "perms", "frozen",NULL},
    {
        "BOOK.serialnum", "type", "category", "categoryName", "publisher", "booktitle", "bookreleaseyear", "bookcover",
        "hits",NULL
    },
    {"STOCK.serialnum", "campus", "instock",NULL},
    {"UUID", "serialnum", "rentduration", "rentdate", "extended",NULL},
    {"UUID", "UUID_ISSUER", "serialnum", "IP", "action", "actiondate", "details",NULL},
    {"account", "sessionID", "expiresAt",NULL}
};
static struct sqlbox_pstmt pstms_data_top[STMTS__MAX] = {
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
    COOKIE_SESSIONID,
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
    {kvalid_stringne, "sessionID"},

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

enum statement {
    STMT_DATA,
    STMT_COUNT,
    STMT_LOGIN,
    STMT_SAVE,
    STMT_CATEGORY_CHILD,
    STMT_AUTHORED,
    STMT_LANGUAGED,
    STMT_STOCKED,
    STMT__FINAL__MAX
};

static struct sqlbox_pstmt pstmts[STMT__FINAL__MAX] = {
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

struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx_data;
struct sqlbox_cfg cfg_data;
size_t dbid_data; // Database associated with a config and a context for query results
struct sqlbox *boxctx_login;
struct sqlbox_cfg cfg_login;
size_t dbid_login;
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
    cfg_data.stmts.stmtsz = STMT__FINAL__MAX;
    cfg_data.stmts.stmts = pstmts;
    if ((boxctx_data = sqlbox_alloc(&cfg_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_data = sqlbox_open(boxctx_data, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}


void alloc_ctx_cfg_login() {
    memset(&cfg_login, 0, sizeof(struct sqlbox_cfg));
    cfg_login.msg.func_short = warnx;
    cfg_login.srcs.srcsz = 1;
    cfg_login.srcs.srcs = srcs;
    cfg_login.stmts.stmtsz = 1;
    cfg_login.stmts.stmts = &pstmts[STMT_LOGIN];
    if ((boxctx_login = sqlbox_alloc(&cfg_login)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_login = sqlbox_open(boxctx_login, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}

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

    if ((field = r.cookiemap[COOKIE_SESSIONID])) {
        size_t stmtid;
        size_t parmsz = 1;
        const struct sqlbox_parmset *res;
        struct sqlbox_parm parms[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s},
        };
        if (!(stmtid = sqlbox_prepare_bind(boxctx_login, dbid_login, 0, parmsz, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
        if ((res = sqlbox_step(boxctx_login, stmtid)) == NULL)
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
        sqlbox_finalise(boxctx_login, stmtid);
    }
}

void build_stmt(enum statement_pieces STMT) {
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
            parmsz++;
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
    kasprintf(&pstmts[STMT_COUNT].stmt, "%s SELECT COUNT(DISTINCT COALESCE(", pstmts[STMT_DATA].stmt);
    kasprintf(&pstmts[STMT_DATA].stmt, "%s SELECT", pstmts[STMT_DATA].stmt);
    for (int i = 0; rows[STMT][i] != NULL; ++i) {
        if (i == 0) {
            kasprintf(&pstmts[STMT_DATA].stmt, "%s %s", pstmts[STMT_DATA].stmt, rows[STMT][i]);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s %s", pstmts[STMT_COUNT].stmt, rows[STMT][i]);
        } else {
            kasprintf(&pstmts[STMT_DATA].stmt, "%s,%s", pstmts[STMT_DATA].stmt, rows[STMT][i]);
            kasprintf(&pstmts[STMT_COUNT].stmt, "%s,%s", pstmts[STMT_COUNT].stmt, rows[STMT][i]);
        }
    }
    kasprintf(&pstmts[STMT_DATA].stmt, "%s %s", pstmts[STMT_DATA].stmt, pstms_data_top[STMT].stmt);
    kasprintf(&pstmts[STMT_COUNT].stmt, "%s,NULL)) %s", pstmts[STMT_COUNT].stmt, pstms_data_top[STMT].stmt);
    bool flag = false;
    for (int i = 0; switch_keys[STMT][i] != KEY__MAX; i++) {
        if (r.fieldmap[switch_keys[STMT][i]]) {
            if (switch_keys[STMT][i] != KEY_SWITCH_ROOT)
                parmsz++;
            if (!flag) {
                if (!strstr(pstms_data_top[STMT].stmt, "WHERE")) {
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

        if ((STMT == STMTS_HISTORY || STMT == STMTS_ACCOUNT || STMT == STMTS_SESSIONS || STMT == STMTS_INVENTORY)) {
            if (switch_keys[STMT][i] == KEY_SWITCH_UUID && !r.fieldmap[KEY_SWITCH_UUID]) {
                if (!curr_usr.perms.admin && !curr_usr.perms.staff) {
                    if (!((curr_usr.perms.monitor_history && STMT == STMTS_HISTORY) || (
                              curr_usr.perms.manage_inventories && STMT == STMTS_INVENTORY) || (
                              curr_usr.perms.see_accounts && STMT == STMTS_ACCOUNT))) {
                        parmsz++;
                        if (!flag) {
                            if (!strstr(pstms_data_top[STMT].stmt, "WHERE")) {
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
            }
        }
    }
    flag = false;
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; i++) {
        if (bottom_keys[STMT][i] == KEY_MANDATORY_GROUP_BY) {
            kasprintf(&pstmts[STMT_DATA].stmt, "%s"" GROUP BY ", pstmts[STMT_DATA].stmt);
            kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt, pstmts_bottom[STMT][i]);
        } else {
            if (r.fieldmap[bottom_keys[STMT][i]]) {
                parmsz++;
                if (!flag) {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" ORDER BY ", pstmts[STMT_DATA].stmt);
                    flag = true;
                } else {
                    kasprintf(&pstmts[STMT_DATA].stmt, "%s"",", pstmts[STMT_DATA].stmt);
                }
                kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt, pstmts_bottom[STMT][i]);
                kasprintf(&pstmts[STMT_DATA].stmt, "%s%s", pstmts[STMT_DATA].stmt,
                          (r.fieldmap[bottom_keys[STMT][i]]->parsed.i == 0) ? " DESC" : " ASC");
            }
        }
    }
    kasprintf(&pstmts[STMT_DATA].stmt, "%s"" LIMIT(? * ?),(?)", pstmts[STMT_DATA].stmt);
    parmsz += 3;
}


int fill_parms(enum statement_pieces STMT) {
    if ((STMT == STMTS_HISTORY || STMT == STMTS_ACCOUNT || STMT == STMTS_SESSIONS || STMT == STMTS_INVENTORY)) {
        if (!curr_usr.authenticated)
            return 0;
    }
    if (STMT == STMTS_INVENTORY && !curr_usr.perms.has_inventory)
        return 0;
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
            if (switch_keys[STMT][i] != KEY_SWITCH_ROOT) {
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
                if (switch_keys[STMT][i] == KEY_SWITCH_UUID) {
                    if (!curr_usr.perms.admin && !curr_usr.perms.staff) {
                        if (!((curr_usr.perms.monitor_history && STMT == STMTS_HISTORY) || (
                                  curr_usr.perms.manage_inventories && STMT == STMTS_INVENTORY) || (
                                  curr_usr.perms.see_accounts && STMT == STMTS_ACCOUNT))) {
                            parms[n - 1] = (struct sqlbox_parm){.type = SQLBOX_PARM_STRING, .sparm = curr_usr.UUID};
                        }
                    }
                }
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

    parms[n++] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT, .iparm = r.fieldmap[KEY_OFFSET] ? r.fieldmap[KEY_OFFSET]->parsed.i : 0
    };
    parms[n++] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT, .iparm = r.fieldmap[KEY_LIMIT] ? r.fieldmap[KEY_LIMIT]->parsed.i : 25
    };
    parms[n++] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT, .iparm = r.fieldmap[KEY_LIMIT] ? r.fieldmap[KEY_LIMIT]->parsed.i : 25
    };
    return 1;
}

void get_cat_children(const char *class) {
    size_t stmtid;
    size_t parmsz2 = 1;
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms2[] = {
        {.type = SQLBOX_PARM_STRING, .sparm = class},
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_CATEGORY_CHILD, parmsz2, parms2,
                                       SQLBOX_STMT_MULTI)))
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
        if (r.fieldmap[KEY_TREE]) {
            kjson_arrayp_open(&req, "children");
            get_cat_children(res->ps[0].sparm);
            kjson_array_close(&req);
        }
        kjson_obj_close(&req);
        if (r.fieldmap[KEY_CASCADE]) {
            get_cat_children(res->ps[0].sparm);
        }
    }
    if (!sqlbox_finalise(boxctx_data, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
}

void process(const enum statement_pieces STATEMENT) {
    size_t stmtid_data;
    const struct sqlbox_parmset *res;
    if (!(stmtid_data = sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_DATA, parmsz, parms, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
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
                    if (STATEMENT == STMTS_ROLE) {
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
                    if ((STATEMENT == STMTS_BOOK || STATEMENT == STMTS_STOCK) && i == 0)
                        kjson_putstringp(&req, "serialnum", res->ps[i].sparm);
                    kjson_putstringp(&req, rows[STATEMENT][i], res->ps[i].bparm);
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
        if (STATEMENT == STMTS_CATEGORY) {
            if (r.fieldmap[KEY_TREE]) {
                kjson_arrayp_open(&req, "children");
                get_cat_children(res->ps[0].sparm);
                kjson_array_close(&req);
            } else if (r.fieldmap[KEY_CASCADE]) {
                kjson_obj_close(&req);
                get_cat_children(res->ps[0].sparm);
            }
        }
        if (STATEMENT == STMTS_BOOK) {
            size_t stmtid_book_data;
            size_t parmsz_book = 1;
            const struct sqlbox_parmset *res_book;
            struct sqlbox_parm parms_book[] = {
                {.type = SQLBOX_PARM_STRING, .sparm = res->ps[0].sparm},
            };
            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_AUTHORED, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "authors");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code ==
                   SQLBOX_CODE_OK
                   && res_book->psz != 0)
                kjson_putstring(&req, res_book->ps[0].sparm);
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");

            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_LANGUAGED, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "langs");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code ==
                   SQLBOX_CODE_OK
                   && res_book->psz != 0)
                kjson_putstring(&req, res_book->ps[0].sparm);
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");

            if (!(stmtid_book_data =
                  sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_STOCKED, parmsz_book, parms_book,
                                      SQLBOX_STMT_MULTI)))
                errx(EXIT_FAILURE, "sqlbox_prepare_bind");

            kjson_arrayp_open(&req, "stock");
            while ((res_book = sqlbox_step(boxctx_data, stmtid_book_data)) != NULL && res_book->code ==
                   SQLBOX_CODE_OK
                   && res_book->psz != 0) {
                kjson_obj_open(&req);
                kjson_putstringp(&req, "campus", res_book->ps[0].sparm);
                kjson_putintp(&req, "stock", res_book->ps[1].iparm);
                kjson_obj_close(&req);
            }
            kjson_array_close(&req);

            if (!sqlbox_finalise(boxctx_data, stmtid_book_data))
                errx(EXIT_FAILURE, "sqlbox_finalise");
        }
        if (!(STATEMENT == STMTS_CATEGORY && r.fieldmap[KEY_CASCADE]))
            kjson_obj_close(&req);
    }
    if (!sqlbox_finalise(boxctx_data, stmtid_data))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    kjson_array_close(&req);
    if (!(stmtid_data = sqlbox_prepare_bind(boxctx_data, dbid_data, STMT_COUNT, parmsz - 3, parms, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx_data, stmtid_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    kjson_putintp(&req, "nbrres", res->ps[0].iparm);
    if (!sqlbox_finalise(boxctx_data, stmtid_data))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    kjson_obj_close(&req);
    kjson_close(&req);
}

void save(const bool failed) {
    char *requestDesc = NULL;
    if (!failed) {
        kasprintf(&requestDesc, "Stmt:%s,parmsz: %ld, Parms:(", pages[r.page], parmsz);
        for (int i = 0; i < (int) parmsz; ++i) {
            switch (parms[i].type) {
                case SQLBOX_PARM_INT:
                    kasprintf(&requestDesc, "%s\"%"PRId64"\",", requestDesc, parms[i].iparm);
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
        kasprintf(&requestDesc, "Stmt:%s, ACCESS DENIED", pages[r.page]);
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
    if (sqlbox_exec(boxctx_data, dbid_data, STMT_SAVE, parmsz_save, parms_save,SQLBOX_STMT_CONSTRAINT) !=
        SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
}

int main(void) {
    enum khttp er;


    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG_BOOK) != KCGI_OK)
        return EXIT_FAILURE;
    khttp_head(&r,"Access-Control-Allow-Origin","https://seele.serveo.net");
    puts("Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r");
    puts("Access-Control-Allow-Credentials: true");
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    const enum statement_pieces STMT = get_stmts();
    alloc_ctx_cfg_login();
    fill_user();
    sqlbox_free(boxctx_login);
    build_stmt(STMT);
    alloc_ctx_cfg();
    if (!fill_parms(STMT)) goto access_denied;
    save(false);
    process(STMT);
    goto cleanup;
access_denied:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_403]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    kjson_putstringp(&req, "error", "You don't have the permissions to access this ressource");
    kjson_obj_close(&req);
    kjson_close(&req);
    save(true);
cleanup:
    khttp_free(&r);
    sqlbox_free(boxctx_data);
    return EXIT_SUCCESS;
}
