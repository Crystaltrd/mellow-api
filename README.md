## API endpoints :

- Optional : `?page` , default `0`

### Publisher:

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

### Action:

* [ ] `SELECT actionName FROM ACTION LIMIT 10 OFFSET (? * 10)`

#### ?by_name:

* [ ] `SELECT actionName FROM ACTION WHERE instr(actionName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

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

### Category:

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

#### ?by_ID:

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE UUID = (?)`

#### ?by_name:

* [ ] 
  `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(displayname,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] 
  `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT,INVENTORY WHERE INVENTORY.UUID = ACCOUNT.UUID AND serialnum = (?) LIMIT 10 OFFSET (? * 10)`

#### ?by_campus:

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(campus,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?by_role:

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE instr(role,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

#### ?frozen:

* [ ] `SELECT UUID,displayname,pwhash,campus,role FROM ACCOUNT WHERE frozen = (?) LIMIT 10 OFFSET (? * 10)`

### Book:

* [ ] 
  `SELECT serialnum,type,category,publisher,booktitle,bookrelease,bookcover,hits FROM BOOKS LIMIT 10 OFFSET (? * 10)`

#### ?by_ID:

* [ ] `SELECT serialnum,type,category,publisher,booktitle,bookrelease,bookcover,hits FROM BOOK WHERE serialnum = (?)`

#### ?filter:

- ?by_category
- ?by_name
- ?by_lang
- ?by_author
- ?by_type
- ?by_publisher
- ?by_campus
- ?from_year
- ?to_year

* [ ] Done

```sqlite
WITH RECURSIVE CategoryCascade AS (SELECT categoryClass, parentCategoryID
                                   FROM CATEGORY
                                   WHERE ((?) = 'ROOT' AND parentCategoryID IS NULL)
                                      OR categoryClass = (?)
                                   UNION ALL
                                   SELECT c.categoryClass, c.parentCategoryID
                                   FROM CATEGORY c
                                          INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass)
SELECT BOOK.serialnum,
       type,
       category,
       publisher,
       booktitle,
       bookreleaseyear,
       bookcover,
       hits
FROM BOOK,
     LANGUAGES,
     AUTHORED,
     STOCK,
     CategoryCascade
WHERE category = CategoryCascade.categoryClass
  AND instr(booktitle, (?)) > 0
  AND BOOK.serialnum = LANGUAGES.serialnum
  AND instr(lang, (?)) > 0
  AND AUTHORED.serialnum = BOOK.serialnum
  AND instr(author, (?)) > 0
  AND instr(type, (?)) > 0
  AND instr(publisher, (?)) > 0
  AND STOCK.serialnum = BOOK.serialnum
  AND instr(STOCK.campus,(?)) > 0
  AND STOCK.instock > 0
  AND IIF((?) = 'FILTER_LOW',bookreleaseyear >= (?),TRUE)
  AND IIF((?) = 'FILTER_HIGH',bookreleaseyear <= (?),TRUE)
GROUP BY BOOK.serialnum,BOOK.hits ORDER BY BOOK.hits DESC LIMIT 10 OFFSET (? * 10) ;

```

#### ?by_account:

* [ ] 
  `SELECT serialnum,type,category,publisher,booktitle,bookrelease,bookcover,hits FROM BOOK,INVENTORY WHERE UUID = (?) AND BOOK.serialnum = INVENTORY.serialnum`

#### ?by_popularity:

* [ ] 
  `SELECT serialnum,type,category,publisher,booktitle,bookrelease,bookcover,hits FROM BOOK ORDER BY hits DESC LIMIT 10 OFFSET (? * 10)`

### Stock:

* [ ] `SELECT serialnum,campus,instock FROM STOCK LIMIT 10 OFFSET (? * 10)`

#### ?by_book:

* [ ] `SELECT serialnum,campus,instock FROM STOCK WHERE serialnum = (?) LIMIT 10 OFFSET (? * 10)`

#### ?by_campus:

* [ ] `SELECT serialnum,campus,instock FROM STOCK WHERE instr(campus,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

### Inventory:

* [ ] `SELECT UUID,serialnum,rentduration,rentdate,extended FROM INVENTORY LIMIT 10 OFFSET (? * 10)`

#### ?by_ID or ?self

* [ ] 
  `SELECT UUID,serialnum,rentduration,rentdate,extended FROM INVENTORY WHERE UUID = (?) ORDER BY rentdate DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_book

* [ ] 
  `SELECT UUID,serialnum,rentduration,rentdate,extended FROM INVENTORY WHERE serialnum = (?) ORDER BY rentdate DESC LIMIT 10 OFFSET (? * 10)`

### History:

* [ ] `SELECT UUID,serialnum,action,actiondate FROM HISTORY ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_id

* [ ] 
  `SELECT UUID,UUID_ISSUER,serialnum,action,actiondate FROM HISTORY WHERE UUID = (?) ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_issuer

* [ ] 
  `SELECT UUID,UUID_ISSUER,serialnum,action,actiondate FROM HISTORY WHERE UUID_ISSUER = (?) ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_book

* [ ] 
  `SELECT UUID,UUID_ISSUER,serialnum,action,actiondate FROM HISTORY WHERE serialnum = (?) ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`

#### ?from_date ?to_date

* [ ] 
  `SELECT UUID,UUID_ISSUER,serialnum,action,actiondate FROM HISTORY WHERE actiondate BETWEEN (?) AND (?) ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`

#### ?by_type

* [ ] 
  `SELECT UUID,UUID_ISSUER,serialnum,action,actiondate FROM HISTORY WHERE instr(action,(?)) > 0 ORDER BY actiondate DESC LIMIT 10 OFFSET (? * 10)`
