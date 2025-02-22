#include <err.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdio.h>
#include <string.h>

enum apis {
    PG_QUERY,
    PG_CREATE,
    PG_SET,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "query",
    "create",
    "set"
};

void handle_err(struct kreq *r, struct kjsonreq *req, enum khttp status) {
    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[status]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_putstringp(req, "details", khttps[status]);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

int
main(void) {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, NULL, 0, pages, PG__MAX, PG_QUERY) != KCGI_OK)
        return EXIT_FAILURE;
    if (pledge("stdio", NULL) == -1)
        err(EXIT_FAILURE, "pledge");
    if (r.method != KMETHOD_GET && r.method != KMETHOD_HEAD) {
        handle_err(&r, &req, KHTTP_405);
    } else if (r.page == PG__MAX) {
        handle_err(&r, &req, KHTTP_403);
    } else {
        khttp_head(&r, kresps[KRESP_STATUS],
                   "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putintp(&req, "code", 200);
        kjson_putstringp(&req, "details", khttps[KHTTP_200]);
        switch (r.page) {
            case PG_QUERY:
                if (strcmp(r.path, "books") != 0) {
                    kjson_putstringp(&req, "querying", "books");
                } else if (strcmp(r.path, "librarians") != 0) {
                    kjson_putstringp(&req, "querying", "librarians");
                } else if (strcmp(r.path, "campuses") != 0) {
                    kjson_putstringp(&req, "querying", "campuses");
                } else if (strcmp(r.path, "rentees") != 0) {
                    kjson_putstringp(&req, "querying", "rentees");
                } else {
                    kjson_putstringp(&req, "querying", "Error");
                }
                break;
            case PG_SET:
                break;
            case PG_CREATE:
                break;
            default:
                break;
        }

        kjson_obj_close(&req);
        kjson_close(&req);
        khttp_free(&r);
    }
    return EXIT_SUCCESS;
}
