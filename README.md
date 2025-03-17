## API endpoints :

- Optional : `?page` , default `0`

### Publisher:

- ?by_name
- ?by_book
- ?by_popularity

* [ ] Done

```sql
SELECT publisherName
FROM PUBLISHER,
     BOOK
WHERE instr(publisherName, (?)) > 0
  AND publisherName = publisher
  AND instr(serialnum, (?)) > 0
GROUP BY publisherName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)

```

### Author:

- ?by_name
- ?by_book
- ?by_popularity

* [ ] Done

```sql
SELECT authorName
FROM AUTHOR,
     AUTHORED,
     BOOK
WHERE instr(authorName, (?)) > 0
  AND author = authorName
  AND instr(AUTHORED.serialnum, (?)) > 0
  AND AUTHORED.serialnum = BOOK.serialnum
GROUP BY authorName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

### Action:

- ?by_name

* [ ] `SELECT actionName FROM ACTION WHERE instr(actionName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

### Doctype:

- ?by_name
- ?by_book
- ?by_popularity

* [ ] Done

```sql
SELECT typeName
FROM DOCTYPE,
     BOOK
WHERE instr(typeName, (?)) > 0
  AND typeName = type
  AND instr(serialnum, (?)) > 0
GROUP BY typeName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

### Campus:

- ?by_name
- ?by_book
- ?by_account

* [ ] Done

```sql
SELECT campusName
FROM CAMPUS,
     STOCK,
     ACCOUNT
WHERE instr(campusName, (?)) > 0
  AND STOCK.campus = campusName
  AND ACCOUNT.campus = campusName
  AND instr(serialnum, (?)) > 0
  AND instr(UUID, (?)) > 0
LIMIT 10 OFFSET (? * 10)
```

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

```sql
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
WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass
LIMIT 10 OFFSET (? * 10);
```

#### ?tree:

* [ ] Done

```sql
SELECT categoryClass, categoryName, parentCategoryID
FROM CATEGORY
WHERE 'INITIAL CONDITION'
SELECT categoryClass, categoryName, parentCategoryID
FROM CATEGORY
WHERE 'RELATION CONDITION'
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
- ?ignore_empty
- ?by_campus
- ?from_year
- ?to_year

* [ ] Done

```sql
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
  AND AUTHORED.serialnum = BOOK.serialnum
  AND BOOK.serialnum = LANGUAGES.serialnum
  AND STOCK.serialnum = BOOK.serialnum
  AND instr(booktitle, (?)) > 0
  AND instr(lang, (?)) > 0
  AND instr(author, (?)) > 0
  AND instr(type, (?)) > 0
  AND instr(publisher, (?)) > 0
  AND instr(STOCK.campus, (?)) > 0
  AND IIF((?) = 'AVAILABLE', STOCK.instock > 0, TRUE)
  AND IIF((?) = 'FILTER_LOW', bookreleaseyear >= (?), TRUE)
  AND IIF((?) = 'FILTER_HIGH', bookreleaseyear <= (?), TRUE)
GROUP BY BOOK.serialnum, BOOK.hits
ORDER BY BOOK.hits DESC
LIMIT 10 OFFSET (? * 10);

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
