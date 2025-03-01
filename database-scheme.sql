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
    categoryName     TEXT UNIQUE NOT NULL,
    categoryDesc     TEXT UNIQUE,
    parentCategoryID INTEGER,
    FOREIGN KEY (parentCategoryID) REFERENCES CATEGORY (ROWID)
);
CREATE TABLE CAMPUS
(
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
    FOREIGN KEY (campusID) REFERENCES CAMPUS (ROWID),
    FOREIGN KEY (roleID) REFERENCES ROLE (ROWID)
);
CREATE TABLE BOOK
(
    serialnum       INTEGER PRIMARY KEY NOT NULL,
    langID          INTEGER             NOT NULL,
    typeID          INTEGER             NOT NULL,
    categoryID      INTEGER             NOT NULL,
    authorID        INTEGER             NOT NULL,
    publisherID     INTEGER             NOT NULL,
    booktitle       TEXT                NOT NULL,
    bookreleaseyear INTEGER             NOT NULL,
    bookcover       TEXT                NOT NULL,
    FOREIGN KEY (langID) REFERENCES LANG (ROWID),
    FOREIGN KEY (typeID) REFERENCES DOCTYPE (ROWID),
    FOREIGN KEY (categoryID) REFERENCES CATEGORY (ROWID),
    FOREIGN KEY (authorID) REFERENCES AUTHOR (ROWID),
    FOREIGN KEY (publisherID) REFERENCES PUBLISHER (ROWID)
);

CREATE TABLE STOCK
(
    serialnum INTEGER NOT NULL,
    campusID  INTEGER NOT NULL,
    instock   INTEGER,
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum),
    FOREIGN KEY (campusID) REFERENCES CAMPUS (ROWID),
    PRIMARY KEY (serialnum, campusID)
);

CREATE TABLE INVENTORY
(
    UUID         INTEGER PRIMARY KEY NOT NULL,
    serialnum    INTEGER             NOT NULL,
    rentduration INTEGER             NOT NULL,
    rentdate     DATETIME            NOT NULL,
    extended     BOOLEAN             NOT NULL,
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID),
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum)
);

CREATE TABLE HISTORY
(
    UUID       INTEGER  NOT NULL,
    serialnum  INTEGER  NOT NULL,
    actionID   INTEGER  NOT NULL,
    actiondate DATETIME NOT NULL,
    FOREIGN KEY (UUID) REFERENCES ACCOUNT (UUID),
    FOREIGN KEY (serialnum) REFERENCES BOOK (serialnum),
    FOREIGN KEY (actionID) REFERENCES ACTION (ROWID)
);
INSERT INTO CAMPUS VALUES('El kseur'),('Targa'),('Aboudaw');
