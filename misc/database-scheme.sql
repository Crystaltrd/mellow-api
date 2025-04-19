pragma foreign_keys= true;
CREATE TABLE PUBLISHER
(
    publisherName TEXT PRIMARY KEY
);

INSERT INTO PUBLISHER
VALUES ('pub1'),
       ('pub2'),
       ('pub3');

CREATE TABLE AUTHOR
(
    authorName TEXT PRIMARY KEY
);

INSERT INTO AUTHOR
VALUES ('AUTHOR1'),
       ('AUTHOR2'),
       ('AUTHOR3');

CREATE TABLE ACTION
(
    actionName TEXT PRIMARY KEY
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
    langCode TEXT PRIMARY KEY
);

INSERT INTO LANG
VALUES ('lang1'),
       ('lang2'),
       ('lang3');

CREATE TABLE DOCTYPE
(
    typeName TEXT PRIMARY KEY
);

INSERT INTO DOCTYPE
VALUES ('type1'),
       ('type2'),
       ('type3');

CREATE TABLE CAMPUS
(
    campusName TEXT PRIMARY KEY
);

INSERT INTO CAMPUS
VALUES ('El Kseur'),
       ('Targa Ouzemmour'),
       ('Aboudaou');

CREATE TABLE ROLE
(
    roleName TEXT PRIMARY KEY,
    perms    INTEGER NOT NULL
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
    categoryClass    TEXT PRIMARY KEY,
    categoryName     TEXT UNIQUE NOT NULL,
    parentCategoryID TEXT REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE
);
INSERT INTO CATEGORY
VALUES ('0', 'Primary1', null),
       ('1', 'Primary2', null);
INSERT INTO CATEGORY
VALUES ('00', 'Sec11', '0'),
       ('01', 'Sec12', '0'),
       ('10', 'Sec21', '1'),
       ('11', 'Sec22', '1');
INSERT INTO CATEGORY
VALUES ('000', 'Sec111', '00'),
       ('001', 'Sec112', '00');

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
VALUES ('admin', 'Administrator C',
        '$argon2id$v=19$m=47104,t=1,p=1$c29tZWhhc2g$qq6HFnwYwAuscx21QUlAZAf9PDs6ve5tTAFDisdc0dk', 'El Kseur', 'ADMIN',
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
    hits            INTEGER DEFAULT 0 CHECK (hits >= 0)
);
INSERT INTO BOOK
VALUES ('1', 'type1', '0', 'pub1', 'Book 1', 1950, null, 0),
       ('2', 'type1', '1', 'pub2', 'Book 2', 2007, null, 9),
       ('3', 'type2', '01', 'pub2', 'Book 3', 9999, null, 7),
       ('4', 'type1', '001', 'pub2', 'Book 4', 2003, null, 15),
       ('5', 'type1', '000', 'pub1', 'Book 5', 2001, null, 94),
       ('6', 'type1', '00', 'pub2', 'Book 6', 2001, null, 95),
       ('7', 'type1', '10', 'pub1', 'Book 7', 2055, null, 93),
       ('8', 'type2', '11', 'pub1', 'Book 8', 3055, null, 19);

CREATE TABLE LANGUAGES
(
    serialnum TEXT NOT NULL REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    lang      TEXT NOT NULL REFERENCES LANG (langCode) ON UPDATE CASCADE ON DELETE CASCADE,
    UNIQUE (serialnum, lang)
);
INSERT INTO LANGUAGES
VALUES ('1', 'lang1'),
       ('2', 'lang2'),
       ('3', 'lang3'),
       ('4', 'lang1'),
       ('4', 'lang2'),
       ('5', 'lang2'),
       ('6', 'lang1'),
       ('6', 'lang3'),
       ('7', 'lang3'),
       ('8', 'lang2');
CREATE TABLE AUTHORED
(
    serialnum TEXT NOT NULL REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    author    TEXT NOT NULL REFERENCES AUTHOR (authorName) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, author)
);

INSERT INTO AUTHORED
VALUES ('1', 'AUTHOR1'),
       ('2', 'AUTHOR2'),
       ('3', 'AUTHOR3'),
       ('4', 'AUTHOR1'),
       ('4', 'AUTHOR2'),
       ('5', 'AUTHOR2'),
       ('6', 'AUTHOR1'),
       ('6', 'AUTHOR3'),
       ('7', 'AUTHOR3'),
       ('8', 'AUTHOR2');
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
VALUES ('1', 'Aboudaou', 3),
       ('2', 'El Kseur', 2),
       ('3', 'Targa Ouzemmour', 5),
       ('4', 'El Kseur', 6),
       ('5', 'El Kseur', 6),
       ('6', 'Aboudaou', 7),
       ('6', 'El Kseur', 7),
       ('7', 'Targa Ouzemmour', 74),
       ('8', 'El Kseur', 1);

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

