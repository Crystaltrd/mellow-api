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

struct kreq r;
struct kjsonreq req;
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
    KEY_PAGE,
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
    {kvalid_uint, "page"},
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
    STMTS__MAX
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
        .mode = SQLBOX_SRC_RO
    }
};
struct sqlbox *boxctx;
struct sqlbox_cfg cfg;
size_t dbid; // Database associated with a config and a context
static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {
        (char *)
        "SELECT publisherName FROM PUBLISHER LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName WHERE ((?) = 'IGNORE_NAME' OR instr(publisherName, (?)) > 0) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) GROUP BY publisherName ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT authorName FROM AUTHOR LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author LEFT JOIN BOOK B ON B.serialnum = A.serialnum WHERE ((?) = 'IGNORE_NAME' OR instr(authorName, (?)) > 0) AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) GROUP BY authorName ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT langCode FROM LANG LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang LEFT JOIN BOOK B ON B.serialnum = A.serialnum WHERE ((?) = 'IGNORE_NAME' OR instr(langCode, (?)) > 0) AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?)) GROUP BY langCode ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {(char *) "SELECT actionName FROM ACTION WHERE instr(actionName,(?)) > 0 LIMIT 10 OFFSET (? * 10)"},
    {
        (char *)
        "SELECT typeName FROM DOCTYPE  LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type WHERE ((?) = 'IGNORE_NAME' OR instr(typeName, (?)) > 0)  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) GROUP BY typeName ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT campusName FROM CAMPUS LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus WHERE ((?) = 'IGNORE_NAME' OR instr(campusName, (?)) > 0) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT roleName, perms FROM ROLE LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName WHERE ((?) = 'IGNORE_NAME' OR instr(roleName, (?)) > 0) AND ((?) = 'IGNORE_PERMS' OR perms = (?)) AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) ORDER BY perms DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT categoryClass, categoryName, parentCategoryID FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category WHERE ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) GROUP BY categoryClass ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        " WITH RECURSIVE CategoryCascade AS (SELECT categoryName,categoryClass, parentCategoryID FROM CATEGORY LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category WHERE ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0) AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?)) AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?)) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) GROUP BY categoryClass UNION ALL SELECT c.categoryName,c.categoryClass, c.parentCategoryID FROM CATEGORY c INNER JOIN CategoryCascade ct ON IIF((?) = 'GET_PARENTS', c.parentCategoryID = ct.categoryClass, c.categoryClass = ct.parentCategoryID)) SELECT CATEGORY.categoryClass, CATEGORY.categoryName,CATEGORY.parentCategoryID FROM CATEGORY, CategoryCascade WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, frozen FROM ACCOUNT LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID WHERE ((?) = 'IGNORE_ID' OR ACCOUNT.UUID = (?)) AND ((?) = 'IGNORE_NAME' OR instr(displayname, (?)) > 0) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) AND ((?) = 'IGNORE_ROLE' OR role = (?)) AND ((?) = 'IGNORE_FREEZE' OR frozen = (?)) ORDER BY ACCOUNT.UUID LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID FROM CATEGORY WHERE ((?) = 'ROOT' AND parentCategoryID IS NULL) OR categoryClass = (?) UNION ALL SELECT c.categoryClass, c.parentCategoryID FROM CATEGORY c INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass) SELECT BOOK.serialnum, type, category, publisher, booktitle, bookreleaseyear, bookcover, hits FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum), LANGUAGES, AUTHORED, STOCK, CategoryCascade WHERE category = CategoryCascade.categoryClass AND AUTHORED.serialnum = BOOK.serialnum AND LANGUAGES.serialnum = BOOK.serialnum AND STOCK.serialnum = BOOK.serialnum AND ((?) = 'IGNORE_ID' OR BOOK.serialnum = (?)) AND ((?) = 'IGNORE_NAME' OR instr(booktitle, (?))) AND ((?) = 'IGNORE_LANG' OR lang = (?)) AND ((?) = 'IGNORE_AUTHOR' OR instr(author, (?)) > 0) AND ((?) = 'IGNORE_TYPE' OR type = (?)) AND ((?) = 'IGNORE_PUBLISHER' OR instr(publisher, (?)) > 0) AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) AND ((?) = 'INCLUDE_EMPTY' OR STOCK.instock > 0) AND ((?) = 'IGNORE_FROM_DATE' OR bookreleaseyear >= (?)) AND ((?) = 'IGNORE_TO_DATE' OR bookreleaseyear <= (?)) GROUP BY BOOK.serialnum, hits, booktitle ORDER BY IIF((?) = 'POPULAR', hits, booktitle) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT STOCK.serialnum, campus, instock FROM STOCK, BOOK WHERE STOCK.serialnum = BOOK.serialnum AND ((?) = 'IGNORE_BOOK' OR STOCK.serialnum = (?)) AND ((?) = 'IGNORE_CAMPUS' OR campus = (?)) AND IIF((?) = 'AVAILABLE', instock > 0, TRUE) GROUP BY BOOK.serialnum, campus, instock, hits ORDER BY IIF((?) = 'POPULAR', hits, instock) DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT UUID, serialnum, rentduration, rentdate, extended FROM INVENTORY WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) ORDER BY rentdate DESC LIMIT 10 OFFSET (? * 10)"
    },
    {
        (char *)
        "SELECT UUID, UUID_ISSUER, serialnum, action, actiondate FROM HISTORY WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?)) AND ((?) = 'IGNORE_ISSUER' OR UUID_ISSUER = (?)) AND ((?) = 'IGNORE_BOOK' OR serialnum = (?)) AND ((?) = 'IGNORE_ACTION' OR action = (?)) AND ((?) = 'IGNORE_FROM_DATE' OR actiondate >= (?)) AND ((?) = 'IGNORE_TO_DATE' OR actiondate <= (?)) ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)"
    }
};
struct sqlbox_parm *parms; //Array of statement parameters
size_t parmsz;

void alloc_ctx_cfg() {
    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = 4;
    cfg.stmts.stmts = pstmts;
    if ((boxctx = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(boxctx, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}

void fill_params(const enum statement STATEMENT) {
    struct kpair *field;
    switch (STATEMENT) {
        case STMTS_ACTION:
            parmsz = 2;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            break;
        case STMTS_PUBLISHER:
        case STMTS_AUTHOR:
        case STMTS_LANG:
        case STMTS_TYPE:
            parmsz = 6;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DONT_IGNORE"
            };
            break;
        case STMTS_CAMPUS:

            parmsz = 7;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_ACCOUNT]) ? "IGNORE_ACCOUNT" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };
            break;
        case STMTS_ROLE:
            parmsz = 7;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_PERM]) ? "IGNORE_PERMS" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_BY_PERM])) ? field->parsed.i : 0
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_ACCOUNT]) ? "IGNORE_ACCOUNT" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };

            break;
        case STMTS_CATEGORY:
            parmsz = 12;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_CLASS]) ? "IGNORE_CLASS" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CLASS])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_PARENT]) ? "IGNORE_PARENT_CLASS" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_PARENT]) ? "IGNORE_PARENT" : "DONT_IGNORE"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DONT_IGNORE"
            };
            break;
        case STMTS_CATEGORY_CASCADE:
            parmsz = 12;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_CLASS]) ? "IGNORE_CLASS" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CLASS])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_PARENT]) ? "IGNORE_PARENT_CLASS" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_PARENT]) ? "IGNORE_PARENT" : "DONT_IGNORE"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PARENT])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_GET_PARENTS]) ? "GET_PARENTS" : "DONT_IGNORE"
            };
            break;
        case STMTS_ACCOUNT:
            parmsz = 13;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_ID]) ? "IGNORE_ID" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ID])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_CAMPUS]) ? "IGNORE_CAMPUS" : "DONT_IGNORE"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_ROLE]) ? "IGNORE_ROLE" : "DONT_IGNORE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ROLE])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_FROZEN]) ? "IGNORE_FREEZE" : "DONT_IGNORE"
            };
            parms[11] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_FROZEN])) ? field->parsed.i : 0
            };

            break;
        case STMTS_BOOK:
            parmsz = 25;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_CATEGORY]) ? "ROOT" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CATEGORY])) ? field->parsed.s : ""
            };

            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_ID]) ? "IGNORE_ID" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ID])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_NAME]) ? "IGNORE_NAME" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_NAME])) ? field->parsed.s : ""
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_LANG]) ? "IGNORE_LANG" : "DONT_IGNORE"
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_LANG])) ? field->parsed.s : ""
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_AUTHOR]) ? "IGNORE_AUTHOR" : "DONT_IGNORE"
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_AUTHOR])) ? field->parsed.s : ""
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_TYPE]) ? "IGNORE_TYPE" : "DONT_IGNORE"
            };
            parms[11] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_TYPE])) ? field->parsed.s : ""
            };
            parms[12] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_PUBLISHER]) ? "IGNORE_PUBLISHER" : "DONT_IGNORE"
            };
            parms[13] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_PUBLISHER])) ? field->parsed.s : ""
            };
            parms[14] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_CAMPUS]) ? "IGNORE_CAMPUS" : "DONT_IGNORE"
            };
            parms[15] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };

            parms[16] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_ACCOUNT]) ? "IGNORE_ACCOUNT" : "DONT_IGNORE"
            };
            parms[17] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : ""
            };
            parms[18] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_IGNORE_EMPTY]) ? "INCLUDE_EMPTY" : "DONT_IGNORE"
            };
            parms[19] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_FROM_YEAR]) ? "IGNORE_FROM_DATE" : "DONT_IGNORE"
            };
            parms[20] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_FROM_YEAR])) ? field->parsed.i : 0
            };
            parms[21] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_TO_YEAR]) ? "IGNORE_TO_DATE" : "DONT_IGNORE"
            };
            parms[22] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_INT,
                .iparm = ((field = r.fieldmap[KEY_FILTER_TO_YEAR])) ? field->parsed.i : 0
            };
            parms[23] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DONT_IGNORE"
            };
            break;
        case STMTS_STOCK:
            parmsz = 7;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_CAMPUS]) ? "IGNORE_CAMPUS" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_CAMPUS])) ? field->parsed.s : ""
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (r.fieldmap[KEY_FILTER_IGNORE_EMPTY]) ? "AVAILABLE" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (r.fieldmap[KEY_FILTER_BY_POPULARITY]) ? "POPULAR" : "DONT_IGNORE"
            };
            break;
        case STMTS_INVENTORY:
            parmsz = 5;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_ACCOUNT] || r.fieldmap[KEY_FILTER_SELF])
                             ? "IGNORE_ACCOUNT"
                             : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : "" //TODO : Handle Self
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            break;
        case STMTS_HISTORY:
            parmsz = 12;
            parms = calloc(parmsz, sizeof(struct sqlbox_parm));
            parms[0] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_ACCOUNT] || r.fieldmap[KEY_FILTER_SELF])
                             ? "IGNORE_ACCOUNT"
                             : "DONT_IGNORE"
            };
            parms[1] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACCOUNT])) ? field->parsed.s : "" //TODO : Handle Self
            };
            parms[2] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (r.fieldmap[KEY_FILTER_BY_ISSUER]) ? "IGNORE_ISSUER" : "DONT_IGNORE"
            };
            parms[3] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ISSUER])) ? field->parsed.s : "" //TODO : Handle Self
            };
            parms[4] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING, .sparm = (!r.fieldmap[KEY_FILTER_BY_BOOK]) ? "IGNORE_BOOK" : "DONT_IGNORE"
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_BOOK])) ? field->parsed.s : ""
            };
            parms[5] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_BY_ACTION]) ? "IGNORE_ACTION" : "DONT_IGNORE"
            };
            parms[6] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_BY_ACTION])) ? field->parsed.s : ""
            };
            parms[7] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_FROM_DATE]) ? "IGNORE_FROM_DATE" : "DONT_IGNORE"
            };
            parms[8] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_FROM_DATE])) ? field->parsed.s : ""
            };
            parms[9] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = (!r.fieldmap[KEY_FILTER_TO_DATE]) ? "IGNORE_TO_DATE" : "DONT_IGNORE"
            };
            parms[10] = (struct sqlbox_parm){
                .type = SQLBOX_PARM_STRING,
                .sparm = ((field = r.fieldmap[KEY_FILTER_TO_DATE])) ? field->parsed.s : ""
            };
            break;
        default:
            errx(EXIT_FAILURE, "params");
            break;
    }

    parms[parmsz - 1] = (struct sqlbox_parm){
        .type = SQLBOX_PARM_INT, .iparm = ((field = r.fieldmap[KEY_PAGE])) ? field->parsed.i : 0
    };
}



int main(void) {
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the INVENTORY if invalid page
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG_INVENTORY) != KCGI_OK)
        return EXIT_FAILURE;
    errx(EXIT_FAILURE,"%s",pages[get_stmts()]);
    return EXIT_SUCCESS;
}
