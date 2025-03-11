pragma foreign_keys= true;
CREATE TABLE PUBLISHER
(
    publisherName TEXT UNIQUE NOT NULL
);
CREATE TABLE AUTHOR
(
    authorName TEXT UNIQUE NOT NULL
);
CREATE TABLE ACTION
(
    actionName TEXT UNIQUE NOT NULL
);
CREATE TABLE LANG
(
    langCode TEXT UNIQUE NOT NULL
);
CREATE TABLE DOCTYPE
(
    typeName TEXT UNIQUE NOT NULL
);
CREATE TABLE CATEGORY
(
    categoryClass    TEXT PRIMARY KEY,
    categoryName     TEXT UNIQUE NOT NULL,
    parentCategoryID TEXT,
    FOREIGN KEY (parentCategoryID) REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE TABLE CAMPUS
(
    campusID INTEGER PRIMARY KEY,
    campusName TEXT UNIQUE NOT NULL
);
CREATE TABLE ROLE
(
    roleName TEXT UNIQUE NOT NULL,
    perms    INTEGER     NOT NULL
);


CREATE TABLE ACCOUNT
(
    UUID        INTEGER PRIMARY KEY NOT NULL,
    displayname TEXT                NOT NULL,
    pwhash      TEXT                NOT NULL,
    campusID    INTEGER             NOT NULL,
    roleID      INTEGER             NOT NULL,
    FOREIGN KEY (campusID) REFERENCES CAMPUS (campusID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (roleID) REFERENCES ROLE (ROWID) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE TABLE BOOK
(
    serialnum       INTEGER PRIMARY KEY NOT NULL,
    typeID          INTEGER             NOT NULL,
    categoryClass TEXT NOT NULL,
    publisherID     INTEGER             NOT NULL,
    booktitle       TEXT                NOT NULL,
    bookreleaseyear INTEGER             NOT NULL,
    bookcover       TEXT                NOT NULL,
    hits          INTEGER DEFAULT 0,
    FOREIGN KEY (typeID) REFERENCES DOCTYPE (ROWID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (categoryClass) REFERENCES CATEGORY (categoryClass) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (publisherID) REFERENCES PUBLISHER (ROWID) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE TABLE LANGUAGES
(
    serialnum INTEGER NOT NULL,
    langID    INTEGER NOT NULL,
    FOREIGN KEY (langID) REFERENCES LANG (ROWID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, langID)
);
CREATE TABLE AUTHORED
(
    authorID  INTEGER NOT NULL,
    serialnum INTEGER NOT NULL,
    FOREIGN KEY (authorID) REFERENCES AUTHOR (ROWID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, authorID)
);
CREATE TABLE STOCK
(
    serialnum INTEGER NOT NULL,
    campusID  INTEGER NOT NULL,
    instock   INTEGER,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (campusID) REFERENCES CAMPUS (campusID) ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY (serialnum, campusID)
);

CREATE TABLE INVENTORY
(
    UUID         INTEGER  NOT NULL,
    serialnum    INTEGER  NOT NULL,
    rentduration INTEGER  NOT NULL,
    rentdate     DATETIME NOT NULL,
    extended     BOOLEAN  NOT NULL,
    PRIMARY KEY (UUID, serialnum),
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE HISTORY
(
    UUID       INTEGER  NOT NULL,
    serialnum  INTEGER  NOT NULL,
    actionID   INTEGER  NOT NULL,
    actiondate DATETIME NOT NULL,
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (actionID) REFERENCES ACTION (ROWID) ON UPDATE CASCADE ON DELETE CASCADE
);
INSERT INTO CAMPUS
VALUES (1, 'Campus El Kseur'),
       (2, 'Campus Targa Ouzemmour'),
       (99, 'Campus Aboudaou');

INSERT INTO ROLE
VALUES ('ADMIN', 64),
       ('STAFF', 62),
       ('SHELF MANAGER', 56),
       ('LIBRARIAN', 6),
       ('PROFESSOR', 1),
       ('STUDENT', 1);

INSERT INTO ACCOUNT
VALUES (1020, 'BOO', 'BAR', 1, 99)
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