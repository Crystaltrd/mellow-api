## API endpoints :

- Optional : `?page` , default `0`

### Publisher:

- ?by_name
- ?by_book
- ?by_popularity

* [ ] Done

```sql
SELECT publisherName
FROM PUBLISHER
         LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName
WHERE instr(publisherName, (?)) > 0
  AND ((?) = 'IGNORE_BOOK' OR instr(serialnum, (?)) > 0)
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
FROM AUTHOR
         LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author
         LEFT JOIN BOOK B ON B.serialnum = A.serialnum
WHERE instr(authorName, (?)) > 0
  AND ((?) = 'IGNORE_BOOK' OR instr(A.serialnum, (?)) > 0)
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
FROM DOCTYPE
         LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type
WHERE instr(typeName, (?)) > 0
  AND ((?) = 'IGNORE_BOOK' OR instr(serialnum, (?)) > 0)
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
FROM ROLE
         LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName
WHERE instr(roleName, (?)) > 0
  AND ((?) = 'IGNORE_PERMS' OR perms = (?))
  AND ((?) = 'IGNORE_ID' OR instr(UUID, (?)) > 0)
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

- ?by_ID
- ?by_name
- ?by_book
- ?by_campus
- ?by_role
- ?frozen

* [ ] Done

```sql
SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, frozen
FROM ACCOUNT
         LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID
WHERE instr(ACCOUNT.UUID, (?)) > 0
  AND ((?) = 'IGNORE_FILTER' OR instr(serialnum, (?)) > 0)
  AND instr(campus, (?)) > 0
  AND instr(role, (?)) > 0
  AND frozen = (?)
ORDER BY ACCOUNT.UUID
LIMIT 10 OFFSET (? * 10)

```

### Book:

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
FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),
     LANGUAGES,
     AUTHORED,
     STOCK,
     CategoryCascade
WHERE category = CategoryCascade.categoryClass
  AND ((?) = 'IGNORE_INV' OR instr(UUID, (?)) > 0)
  AND AUTHORED.serialnum = BOOK.serialnum
  AND LANGUAGES.serialnum = BOOK.serialnum
  AND STOCK.serialnum = BOOK.serialnum
  AND instr(booktitle, (?)) > 0
  AND instr(lang, (?)) > 0
  AND instr(author, (?)) > 0
  AND instr(type, (?)) > 0
  AND instr(publisher, (?)) > 0
  AND instr(campus, (?)) > 0
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