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
#include <pwd.h>
#include <unistd.h>
struct kreq r;
struct kjsonreq req;

enum statement {
    STMTS_LOGOUT,
    STMTS_LOGOUTALL,
    STMTS_CUSTOM,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {(char *) "DELETE FROM SESSIONS WHERE sessionID = (?)"},
    {(char *) "DELETE FROM SESSIONS WHERE account = (SELECT account FROM SESSIONS WHERE sessionID = (?))"},
    {(char *) "DELETE FROM SESSIONS WHERE IIF((?) = 'ACCOUNT', account,sessionID) = (?)" }
};
