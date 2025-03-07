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
    UDCClass TEXT UNIQUE,
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
INSERT INTO CAMPUS
VALUES ('Campus El Kseur'),
       ('Campus Targa Ouzemmour'),
       ('Campus Aboudaou');

INSERT INTO ROLE
VALUES ('ADMIN', 64),
       ('STAFF', 62),
       ('SHELF MANAGER', 56),
       ('LIBRARIAN', 6),
       ('PROFESSOR', 1),
       ('STUDENT', 1);

INSERT INTO CATEGORY
VALUES ('Science and knowledge. Organization. Computer science. Information. Documentation. Librarianship. Institution. Publications',
        'UDC 0', null),
       ('Philosophy. Psychology', 'UDC 1', null),
       ('Religion. Theology', 'UDC 2', null),
       ('Social sciences', 'UDC 3', null),
       ('vacant', 'UDC 4', null),
       ('Mathematics. Natural Sciences', 'UDC 5', null),
       ('Applied Sciences. Medicine, Technology', 'UDC 6', null),
       ('The Arts. Entertainment. Sport', 'UDC 7', null),
       ('Linguistics. Literature', 'UDC 8', null),
       ('Geography. History', 'UDC 9', null),
       ('Prolegomena. Fundamentals of knowledge and culture. Propaedeutics', null, 1),
       ('Science and knowledge in general. Organization of intellectual work', null, 11),
       ('Documentation. Books. Writings. Authorship', null, 11),
       ('Writing systems and scripts', null, 11),
       ('Computer science and technology. Computing', null, 11),
       ('Computer architecture', null, 15),
       ('Computer hardware', null, 15),
       ('Software', null, 15),
       ('Human-computer interaction', null, 15),
       ('Data', null, 15),
       ('Computer communication', null, 15),
       ('Artificial intelligence', null, 15),
       ('Application-oriented computer-based techniques', null, 15),
       ('Management', null, 11),
       ('Management Theory', null, 24),
       ('Management agents. Mechanisms. Measures', null, 24),
       ('Management activities', null, 24),
       ('Management operations. Direction', null, 24),
       ('Quality management. Total quality management (TQM)', null, 24),
       ('Organizational management (OM)', null, 24),
       ('Fields of management', null, 24),
       ('Records management', null, 31),
       ('Plant management. Physical resources management', null, 31),
       ('Knowledge management', null, 31),
       ('Personnel management. Human Resources management', null, 31),
       ('Standardization of products, operations, weights, measures and time', null, 11),
       ('Activity and organizing. Information. Communication and control theory generally (cybernetics)', null, 11),
       ('Civilization. Culture. Progress', null, 11),
       ('Bibliography and bibliographies. Catalogues', null, 1),
       ('Librarianship', null, 1),
       ('General reference works (as subject)', null, 1),
       ('Serial publications, periodicals (as subject)', null, 1),
       ('Organizations of a general nature', null, 1),
       ('Museums', null, 43),
       ('Newspapers (as subject). The Press. Outline of journalism', null, 1),
       ('Polygraphies. Collective works (as subject)', null, 1),
       ('Manuscripts. Rare and remarkable works (as subject)', null, 1);