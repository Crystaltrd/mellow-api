pragma foreign_keys= true;
CREATE TABLE PUBLISHER
(
    publisherName TEXT PRIMARY KEY NOT NULL
);

INSERT INTO PUBLISHER
VALUES ('Longman Publishing');
CREATE TABLE AUTHOR
(
    authorName TEXT PRIMARY KEY NOT NULL
);

INSERT INTO AUTHOR
VALUES ('Brian Wilson Kernighan'),
       ('Dennis M Ritchie');
CREATE TABLE ACTION
(
    actionName TEXT PRIMARY KEY NOT NULL
);

INSERT INTO ACTION
VALUES ('ADD'),
       ('REMOVE'),
       ('EDIT'),
       ('QUERY'),
       ('LOGIN'),
       ('LOGOUT'),
       ('SIGNUP'),
       ('REMOVE_ACCOUNT'),
       ('RENT');

CREATE TABLE LANG
(
    langCode TEXT PRIMARY KEY NOT NULL
);

INSERT INTO LANG
VALUES ('kab'),
       ('en'),
       ('fr'),
       ('ar');

CREATE TABLE DOCTYPE
(
    typeName TEXT PRIMARY KEY NOT NULL
);

INSERT INTO DOCTYPE
VALUES ('Book'),
       ('Thesis'),
       ('Report'),
       ('Article');

CREATE TABLE CAMPUS
(
    campusName TEXT PRIMARY KEY NOT NULL
);

INSERT INTO CAMPUS
VALUES ('El Kseur'),
       ('Targa Ouzemmour'),
       ('Aboudaou');

CREATE TABLE ROLE
(
    roleName TEXT PRIMARY KEY NOT NULL,
    perms    INTEGER          NOT NULL
);

INSERT INTO ROLE
VALUES ('ADMIN', 127),
       ('STAFF', 62),
       ('SHELF MANAGER', 56),
       ('LIBRARIAN', 6),
       ('PROFESSOR', 1),
       ('STUDENT', 1);

CREATE TABLE CATEGORY
(
    categoryClass    TEXT PRIMARY KEY NOT NULL,
    categoryName     TEXT UNIQUE      NOT NULL,
    parentCategoryID TEXT REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE DEFAULT NULL
);
INSERT INTO CATEGORY
VALUES ('0', 'Non-Fiction', null),
       ('1', 'Fiction', null);
INSERT INTO CATEGORY
VALUES ('00', 'Programming', '0'),
       ('01', 'Physics', '0'),
       ('10', 'Fantasy', '1'),
       ('11', 'Sci-Fi', '1');
INSERT INTO CATEGORY
VALUES ('110', 'Utopian', '11'),
       ('111', 'Dystopian', '11');

CREATE TABLE ACCOUNT
(
    UUID        TEXT PRIMARY KEY NOT NULL,
    displayname TEXT             NOT NULL,
    pwhash      TEXT             NOT NULL,
    campus      TEXT             NOT NULL REFERENCES CAMPUS (campusName) ON UPDATE CASCADE ON DELETE CASCADE,
    role        TEXT             NOT NULL REFERENCES ROLE (roleName) ON UPDATE CASCADE ON DELETE CASCADE,
    frozen      BOOLEAN DEFAULT FALSE
);
CREATE TABLE SESSIONS
(
    account   TEXT     NOT NULL REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    sessionID TEXT     NOT NULL UNIQUE,
    expiresAt DATETIME NOT NULL
);
INSERT INTO ACCOUNT
VALUES ('teto', 'Administrator Kasane Teto',
        '$argon2id$v=19$m=47104,t=1,p=2$ekVIR1hycXl0Rk9hb0l6Qg$a4F4t8KwYTGXFprw2mI1xdIYtNsir9TWMpCO89v26e8', 'El Kseur',
        'ADMIN',
        FALSE);

CREATE TABLE BOOK
(
    serialnum       TEXT PRIMARY KEY NOT NULL,
    type            TEXT             NOT NULL REFERENCES DOCTYPE (typeName) ON UPDATE CASCADE ON DELETE CASCADE,
    category        TEXT             NOT NULL REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE,
    publisher       TEXT             NOT NULL REFERENCES PUBLISHER (publisherName) ON UPDATE CASCADE ON DELETE CASCADE,
    booktitle       TEXT             NOT NULL,
    bookreleaseyear INTEGER          NOT NULL,
    bookcover       TEXT,
    description     TEXT,
    hits            INTEGER DEFAULT 0 CHECK (hits >= 0)
);
Insert INTO BOOK
VALUES ('9780131101630', 'Book', '00', 'Longman Publishing',
        'The C Programming Language', 1978, '9780131101630.jpg',
        'Known as the bible of C, this classic bestseller introduces the C programming language and illustrates ' ||
        'algorithms, data structures, and programming techniques.', 0),
       ('9', 'Book', '00', 'Longman Publishing',
        'The C Programming Language', 1978, '9780131101630.jpg',
        'Known as the bible of C, this classic bestseller introduces the C programming language and illustrates ' ||
        'algorithms, data structures, and programming techniques.', 0);

CREATE TABLE LANGUAGES
(
    serialnum TEXT NOT NULL REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    lang      TEXT NOT NULL REFERENCES LANG (langCode) ON UPDATE CASCADE ON DELETE CASCADE,
    UNIQUE (serialnum, lang)
);
INSERT INTO LANGUAGES
VALUES ('9780131101630', 'en'),
       ('9', 'en');
CREATE TABLE AUTHORED
(
    serialnum TEXT NOT NULL REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    author    TEXT NOT NULL REFERENCES AUTHOR (authorName) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, author)
);

INSERT INTO AUTHORED
VALUES ('9780131101630', 'Brian Wilson Kernighan'),
       ('9', 'Dennis M Ritchie');
CREATE TABLE STOCK
(
    serialnum TEXT NOT NULL,
    campus    TEXT NOT NULL,
    instock   INTEGER DEFAULT 0 CHECK (instock >= 0),
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (campus) REFERENCES CAMPUS (campusName) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, campus)
);

INSERT INTO STOCK
VALUES ('9780131101630', 'Aboudaou', 1),
       ('9', 'El Kseur', 1),
       ('9780131101630', 'Targa Ouzemmour', 1)
;

CREATE TABLE INVENTORY
(
    UUID         TEXT     NOT NULL REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    serialnum    TEXT     NOT NULL REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    rentduration INTEGER  NOT NULL CHECK (rentduration > 0),
    rentdate     DATETIME NOT NULL,
    extended     BOOLEAN  NOT NULL,
    UNIQUE (UUID, serialnum)
);


CREATE TABLE HISTORY
(
    UUID        TEXT DEFAULT NULL,
    UUID_ISSUER TEXT DEFAULT NULL,
    serialnum   TEXT DEFAULT NULL,
    IP          TEXT DEFAULT NULL,
    action      TEXT     NOT NULL REFERENCES ACTION (actionName) ON UPDATE CASCADE ON DELETE CASCADE,
    actiondate  DATETIME NOT NULL,
    details     TEXT DEFAULT NULL
);
