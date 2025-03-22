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

struct kreq r;
struct kjsonreq req;

enum key {
    KEY_UUID,
    KEY_PASSWD,
    KEY_REMEMBER,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "UUID"},
    {kvalid_stringne, "passwd"},
    {NULL, "remember"},
};

enum statement {
    STMTS_CHECK,
    STMTS_ADD,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX]{
    {(char *) "SELECT EXISTS(SELECT 1 FROM ACCOUNT WHERE UUID=(?) AND pwhash=(?) LIMIT 1"},
    {(char *) "INSERT INTO SESSIONS VALUES((?),(?),datetime('now',(?),'localtime'))"}
};

enum khttp sanitize() {
    if (r.method != KMETHOD_POST)
        return KHTTP_405;
    if (!r.fieldmap[KEY_UUID] || !r.fieldmap[KEY_PASSWD])
        return KHTTP_402;
    return KHTTP_200;
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, NULL, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    return EXIT_SUCCESS;
}
