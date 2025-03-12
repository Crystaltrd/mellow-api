pragma foreign_keys= true;
CREATE TABLE PUBLISHER
(
    publisherName TEXT PRIMARY KEY
);
CREATE TABLE AUTHOR
(
    authorName TEXT PRIMARY KEY
);
CREATE TABLE ACTION
(
    actionName TEXT PRIMARY KEY
);
CREATE TABLE LANG
(
    langCode TEXT PRIMARY KEY
);
CREATE TABLE DOCTYPE
(
    typeName TEXT PRIMARY KEY
);
CREATE TABLE CAMPUS
(
    campusName TEXT PRIMARY KEY
);
CREATE TABLE ROLE
(
    roleName TEXT PRIMARY KEY,
    perms    INTEGER NOT NULL
);

CREATE TABLE CATEGORY
(
    categoryClass    TEXT PRIMARY KEY,
    categoryName     TEXT UNIQUE NOT NULL,
    parentCategoryID TEXT,
    FOREIGN KEY (parentCategoryID) REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE
);


CREATE TABLE ACCOUNT
(
    UUID        INTEGER PRIMARY KEY NOT NULL,
    displayname TEXT                NOT NULL,
    pwhash      TEXT                NOT NULL,
    campus TEXT NOT NULL,
    role   TEXT NOT NULL,
    FOREIGN KEY (campus) REFERENCES CAMPUS (campusName) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (role) REFERENCES ROLE (roleName) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE TABLE BOOK
(
    serialnum       INTEGER PRIMARY KEY NOT NULL,
    type      TEXT NOT NULL,
    category  TEXT NOT NULL,
    publisher TEXT NOT NULL,
    booktitle       TEXT                NOT NULL,
    bookreleaseyear INTEGER             NOT NULL,
    bookcover TEXT,
    hits      INTEGER DEFAULT 0,
    FOREIGN KEY (type) REFERENCES DOCTYPE (typeName) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (category) REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (publisher) REFERENCES PUBLISHER (publisherName) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE TABLE LANGUAGES
(
    serialnum INTEGER NOT NULL,
    lang INTEGER NOT NULL,
    FOREIGN KEY (lang) REFERENCES LANG (langCode) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    UNIQUE (serialnum, lang)
);
CREATE TABLE AUTHORED
(
    author INTEGER NOT NULL,
    serialnum INTEGER NOT NULL,
    FOREIGN KEY (author) REFERENCES AUTHOR (authorName) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, author)
);
CREATE TABLE STOCK
(
    serialnum INTEGER NOT NULL,
    campus INTEGER NOT NULL,
    instock   INTEGER,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (campus) REFERENCES CAMPUS (campusName) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, campus)
);

CREATE TABLE INVENTORY
(
    UUID         INTEGER  NOT NULL,
    serialnum    INTEGER  NOT NULL,
    rentduration INTEGER  NOT NULL,
    rentdate     DATETIME NOT NULL,
    extended     BOOLEAN  NOT NULL,
    UNIQUE (UUID, serialnum),
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE HISTORY
(
    UUID       INTEGER  NOT NULL,
    serialnum  INTEGER  NOT NULL,
    action TEXT NOT NULL,
    actiondate DATETIME NOT NULL,
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (action) REFERENCES ACTION (actionName) ON UPDATE CASCADE ON DELETE CASCADE
);
INSERT INTO CAMPUS
VALUES ('El Kseur'),
       ('Targa Ouzemmour'),
       ('Aboudaou');

INSERT INTO ROLE
VALUES ('ADMIN', 64),
       ('STAFF', 62),
       ('SHELF MANAGER', 56),
       ('LIBRARIAN', 6),
       ('PROFESSOR', 1),
       ('STUDENT', 1);
INSERT INTO CATEGORY
VALUES ('00', 'Category', null);
INSERT INTO DOCTYPE
VALUES ('type');
INSERT INTO PUBLISHER
VALUES ('Pub1'),
       ('Pub2'),
       ('Pub3');

INSERT INTO BOOK
VALUES (123, 'type', '00', 'Pub1', 'Hello', 2005, 'hello', 99),
       (12333, 'type', '00', 'Pub2', 'Hello', 2005, 'hello', 1),
       (14423, 'type', '00', 'Pub2', 'Hello', 2005, 'hello', 1);

INSERT INTO AUTHOR
VALUES ('AUTHOR1'),
       ('AUTHOR2'),
       ('AUTHOR3');

INSERT INTO AUTHORED
VALUES ('AUTHOR1', 123),
       ('AUTHOR1', 12333),
       ('AUTHOR2', 14423);

/*
WITH RECURSIVE CategoryCascade AS (SELECT ROWID, parentCategoryID
                            FROM CATEGORY
                            WHERE ROWID = 3
                            UNION ALL
                            SELECT c.ROWID, c.parentCategoryID
                            FROM CATEGORY c
                                     INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.ROWID)
SELECT CATEGORY.ROWID, CATEGORY.categoryName
FROM CATEGORY,
 CategoryCascade
WHERE CategoryCascade.ROWID = CATEGORY.ROWID;*/