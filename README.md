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

- ?by_name
- ?by_perm
- ?by_account

* [ ] Done

```sql
SELECT roleName, perms
FROM ROLE,
     ACCOUNT
WHERE roleName = role
  AND instr(roleName, (?)) > 0
  AND perms = (?)
  AND instr(UUID, (?)) > 0
ORDER BY perms DESC
LIMIT 10 OFFSET (? * 10)
```

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

#### ?filter:

- ?by_ID
- ?by_category
- ?by_name
- ?by_lang
- ?by_author
- ?by_type
- ?by_publisher
- ?by_campus
- ?by_account
- ?ignore_empty
- ?from_year
- ?to_year
- ?by_popularity

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
     INVENTORY,
     CategoryCascade
WHERE category = CategoryCascade.categoryClass
  AND AUTHORED.serialnum = BOOK.serialnum
  AND LANGUAGES.serialnum = BOOK.serialnum
  AND STOCK.serialnum = BOOK.serialnum
  AND INVENTORY.serialnum = BOOK.serialnum
  AND instr(BOOK.serialnum, (?)) > 0
  AND instr(booktitle, (?)) > 0
  AND instr(lang, (?)) > 0
  AND instr(author, (?)) > 0
  AND instr(type, (?)) > 0
  AND instr(publisher, (?)) > 0
  AND instr(campus, (?)) > 0
  AND instr(UUID, (?)) > 0
  AND IIF((?) = 'AVAILABLE', STOCK.instock > 0, TRUE)
  AND IIF((?) = 'FILTER_LOW', bookreleaseyear >= (?), TRUE)
  AND IIF((?) = 'FILTER_HIGH', bookreleaseyear <= (?), TRUE)
GROUP BY BOOK.serialnum, hits, booktitle
ORDER BY IIF((?) = 'POPULAR', hits, booktitle) DESC
LIMIT 10 OFFSET (? * 10);

```

### Stock:

- ?by_book
- ?by_campus
- ?ignore_empty
- ?by_popularity

* [ ] Done

```sql
SELECT STOCK.serialnum, campus, instock
FROM STOCK,
     BOOK
WHERE instr(STOCK.serialnum, (?)) > 0
  AND STOCK.serialnum = BOOK.serialnum
  AND instr(campus, (?)) > 0
  AND IIF((?) = 'AVAILABLE', instock > 0, TRUE)
GROUP BY BOOK.serialnum, campus, instock, hits
ORDER BY IIF((?) = 'POPULAR', hits, instock) DESC
LIMIT 10 OFFSET (? * 10)
```

### Inventory:

- ?by_account | ?self (alias to ?by_account with cookie)
- ?by_book

* [ ] Done

```sql
SELECT UUID, serialnum, rentduration, rentdate, extended
FROM INVENTORY
WHERE instr(UUID, (?)) > 0
  AND instr(serialnum, (?)) > 0
ORDER BY rentdate DESC
LIMIT 10 OFFSET (? * 10)
```

### History:

- ?by_account
- ?by_issuer
- ?by_book
- ?by_action
- ?from_date
- ?to_date

* [ ] Done

```sql
SELECT UUID, UUID_ISSUER, serialnum, action, actiondate
FROM HISTORY
WHERE instr(UUID, (?)) > 0
  AND instr(UUID_ISSUER, (?)) > 0
  AND instr(serialnum, (?)) > 0
  AND instr(action, (?)) > 0
  AND IIF((?) = 'FILTER_LOW', actiondate >= (?), TRUE)
  AND IIF((?) = 'FILTER_HIGH', actiondate <= (?), TRUE)
ORDER BY actiondate DESC
LIMIT 10 OFFSET (? * 10)
```