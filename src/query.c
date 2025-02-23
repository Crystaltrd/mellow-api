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

void handle_err(struct kreq *r, struct kjsonreq *req, enum khttp status) {
    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[status]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_putstringp(req, "error", khttps[status]);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

void handle_campuses(struct kreq *r, struct kjsonreq *req) {
     khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_putstringp(req,"answer","campus");
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}
int main(void) {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, NULL, 0, pages, PG__MAX, PG_BOOKS) != KCGI_OK)
        return EXIT_FAILURE;
    if (r.method != KMETHOD_GET && r.method != KMETHOD_HEAD) {
        handle_err(&r, &req, KHTTP_405);
    } else {
        switch (r.page) {
            case PG_CAMPUSES:
                handle_campuses(&r,&req);
                break;
            default:
                handle_err(&r, &req, KHTTP_403);
                break;
        }
    }
    return EXIT_SUCCESS;
}
