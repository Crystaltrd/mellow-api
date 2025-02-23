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

enum pg {
    PG_BOOKS,
    PG_CAMPUSES,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "book",
    "campus",
};

void handle_err(struct kreq *r, struct kjsonreq *req, enum khttp status, int err) {

    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[status]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_putintp(req, "status", err);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

void handle_campuses(struct kreq *r, struct kjsonreq *req) {
    size_t dbid, stmtid;
    struct sqlbox *p2;
    struct sqlbox_cfg cfg;
    struct sqlbox_src srcs[] = {
        {
            .fname = (char *) "db/database.db",
            .mode = SQLBOX_SRC_RWC
        }
    };
    struct sqlbox_pstmt pstmts[] = {
        {.stmt = (char *) "SELECT ROWID,* FROM CAMPUSES"},
    };
    const struct sqlbox_parmset *res;

    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = 1;
    cfg.stmts.stmts = pstmts;
    if ((p2 = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(p2, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
    if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 0, 0, 0, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_arrayp_open(req, "campus");
    while ((res = sqlbox_step(p2, stmtid)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(req);
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "campus", res->ps[1].sparm);
        kjson_obj_close(req);
    }
    if (!sqlbox_finalise(p2, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    sqlbox_free(p2);
    kjson_obj_close(req);
    kjson_putintp(req, "status", 200);
    kjson_close(req);
    khttp_free(r);
}

int main(void) {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, NULL, 0, pages, PG__MAX, PG_BOOKS) != KCGI_OK)
        return EXIT_FAILURE;
    if (r.method != KMETHOD_GET && r.method != KMETHOD_HEAD) {
        handle_err(&r, &req, KHTTP_405, 405);
    } else {
        switch (r.page) {
            case PG_CAMPUSES:
                handle_campuses(&r, &req);
                break;
            default:
                handle_err(&r, &req, KHTTP_403, 403);
                break;
        }
    }
    return EXIT_SUCCESS;
}
