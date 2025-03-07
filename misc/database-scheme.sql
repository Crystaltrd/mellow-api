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
VALUES ('0 Science and knowledge. Organization. Computer science. Information. Documentation. Librarianship. Institution. Publications',
        null),
       ('1 Philosophy. Psychology', null),
       ('2 Religion. Theology', null),
       ('3 Social sciences', null),
       ('4 vacant', null),
       ('5 Mathematics. Natural Sciences', null),
       ('6 Applied Sciences. Medicine, Technology', null),
       ('7 The Arts. Entertainment. Sport', null),
       ('8 Linguistics. Literature', null),
       ('9 Geography. History', null),
       ('00 Prolegomena. Fundamentals of knowledge and culture. Propaedeutics', 1),
       ('001 Science and knowledge in general. Organization of intellectual work', 11),
       ('002 Documentation. Books. Writings. Authorship', 11),
       ('003 Writing systems and scripts', 11),
       ('004 Computer science and technology. Computing. Data processing', 11),
       ('004.2 Computer architecture', 15),
       ('004.3 Computer hardware', 15),
       ('004.4 Software', 15),
       ('004.5 Human-computer interaction. Man-machine interface. User interface. User environment', 15),
       ('004.6 Data', 15),
       ('004.7 Computer communication', 15),
       ('004.8 Artificial intelligence', 15),
       ('004.9 Application-oriented computer-based techniques', 15),
       ('005 Management', 11),
       ('005.1 Management Theory', 24),
       ('005.2 Management agents. Mechanisms. Measures', 24),
       ('005.3 Management activities', 24),
       ('005.4 Processes in management', 24),
       ('005.5 Management operations. Direction', 24),
       ('005.6 Quality management. Total quality management (TQM)', 24),
       ('005.7 Organizational management (OM)', 24),
       ('005.9 Fields of management', 24),
       ('005.91 Administrative management. Secretariat', 32),
       ('005.92 Records management', 32),
       ('005.93 Plant management. Physical resources management', 32),
       ('005.94 Knowledge management', 32),
       ('005.95/96 Personnel management. Human Resources management', 32),
       ('006 Standardization of products, operations, weights, measures and time', 11),
       ('007 Activity and organizing. Information. Communication and control theory generally (cybernetics)', 11),
       ('008 Civilization. Culture. Progress', 11),
       ('01 Bibliography and bibliographies. Catalogues', 1),
       ('02 Librarianship', 1),
       ('030 General reference works (as subject)', 1),
       ('050 Serial publications, periodicals (as subject)', 1),
       ('06 Organizations of a general nature', 1),
       ('070 Newspapers (as subject). The Press. Outline of journalism', 1),
       ('08 Polygraphies. Collective works (as subject)', 1),
       ('09 Manuscripts. Rare and remarkable works (as subject)', 1);