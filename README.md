## API endpoints :

- Optional : `?page` , default `0`

### publisher:

* [ ]  `SELECT publisherName FROM PUBLISHER LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT publisherName FROM PUBLISHER WHERE instr(publisherName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] `SELECT publisher FROM BOOK WHERE serialnum = (?)`

#### ?by_popularity:

* [ ] `SELECT publisher FROM BOOK GROUP BY publisher ORDER BY SUM(hits) DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_count:

* [ ] `SELECT publisher FROM BOOK GROUP BY publisher ORDER BY COUNT() DESC LIMIT 10 OFFSET (? * 10)`

### Author:

* [ ] `SELECT authorName FROM AUTHOR LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT authorName FROM AUTHOR WHERE instr(authorName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] `SELECT author FROM AUTHORED WHERE serialnum = (?)`

#### ?by_popularity:

* [ ] 
  `SELECT author,SUM(hits) AS total_hits FROM AUTHORED,BOOK WHERE BOOK.serialnum = AUTHORED.serialnum GROUP BY author ORDER BY total_hits DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_count:

* [ ] 
  `SELECT author,COUNT() AS total_count FROM AUTHORED,BOOK WHERE BOOK.serialnum = AUTHORED.serialnum GROUP BY author ORDER BY total_count DESC LIMIT 10 OFFSET (? * 10)`

### Doctype:

* [ ] `SELECT typeName FROM DOCTYPE LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT typeName FROM DOCTYPE WHERE instr(typeName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] `SELECT type FROM DOCTYPE WHERE serialnum = (?)`

#### ?by_popularity:

* [ ] `SELECT type FROM BOOK GROUP BY type ORDER BY SUM(hits) DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_count:

* [ ] `SELECT type FROM BOOK GROUP BY type ORDER BY COUNT() DESC LIMIT 10 OFFSET (? * 10)`

### Campus:

* [ ] `SELECT campusName FROM CAMPUS LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT campusName FROM CAMPUS WHERE instr(campusName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] `SELECT campus FROM STOCK WHERE serialnum = (?)`

#### ?by_account:

* [ ] `SELECT campus FROM ACCOUNT WHERE UUID = (?)`

### Role:

* [ ] `SELECT roleName,perms FROM ROLE LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT roleName,perms FROM ROLE WHERE instr(roleName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_perm:

* [ ] `SELECT roleName,perms FROM ROLE WHERE perms = (?) LIMIT 10 OFFSET (? * 10)`

#### ?by_account:

* [ ] `SELECT role,perms FROM ACCOUNT,ROLE WHERE UUID = (?) AND role = roleName`

### Category

* [ ] `SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY LIMIT 10 OFFSET (? * 10)`

#### ?cascade:

* [ ] Done

```sqlite
WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID
                            FROM CATEGORY
                            WHERE 'INITIAL CONDITION'
                            UNION ALL
                            SELECT c.categoryClass, c.parentCategoryID
                            FROM CATEGORY c
                                     INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass 'RELATION CONDITION')
SELECT CATEGORY.categoryClass, CATEGORY.categoryName
FROM CATEGORY,
 CategoryCascade
WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass LIMIT 10 OFFSET (? * 10);
```

#### ?tree:

* [ ] Done

```sqlite
SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY WHERE 'INITIAL CONDITION'
SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY WHERE 'RELATION CONDITION'
```

#### ?by_name:

* [ ] 
  `SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY WHERE instr(categoryName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_class:

* [ ] 
  `SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY WHERE instr(categoryClass,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_parent:

* [ ] 
  `SELECT categoryClass,categoryName,parentCategoryID FROM CATEGORY WHERE instr(parentCategoryID,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] 
  `SELECT category,categoryName,parentCategoryID FROM CATEGORY,BOOK WHERE serialnum = (?) AND categoryClass = category`

### Account:

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT LIMIT 10 OFFSET (? * 10)`

#### ?by_ID

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE UUID = (?)`

#### ?by_name

* [ ] 
  `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(displayname,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book

* [ ] 
  `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT,INVENTORY WHERE INVENTORY.UUID = ACCOUNT.UUID AND serialnum = (?) LIMIT 10 OFFSET (? * 10)`

#### ?by_campus

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(campus,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_role

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(role,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?frozen

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE frozen = (?) LIMIT 10 OFFSET (? * 10)`

### Book:
