## Permissions:

- Admin: Can manage Staff accounts, can import and export backups.
- Staff: Can manage non-staff accounts
- Managing Stock: Can manage books and the stock, can create and delete languages, categories...etc
- Managing Inventories: Can give and take books from other Inventories
- See Account details: See other accounts details
- Monitor the History: Can see the entire history
- Has an Inventory: Can rent books

## API endpoints :

- Optional : `?page` , default `0`
- ?by_name
- ?by_book
- ?by_popularity
- ?by_account
- ?by_perm
- ?by_class
- ?by_parent
- ?cascade
- ?tree
- ?get_parents
- ?by_ID
- ?by_campus
- ?by_role
- ?frozen
- ?by_category
- ?by_lang
- ?by_author
- ?by_type
- ?by_publisher
- ?ignore_empty
- ?from_year
- ?to_year
- ?self
- ?by_issuer
- ?by_action
- ?from_date
- ?to_date

### Publisher:

- ?by_name
- ?by_book
- ?by_popularity

```sql
SELECT publisherName
FROM PUBLISHER
         LEFT JOIN BOOK B ON B.publisher = PUBLISHER.publisherName
WHERE ((?) = 'IGNORE_NAME' OR instr(publisherName, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
GROUP BY publisherName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)

```

```json
[
  {
    "publisherName": "pub1"
  },
  {
    "publisherName": "pub2"
  },
  {
    "publisherName": "pub3"
  }
]
```

### Author:

- ?by_name
- ?by_book
- ?by_popularity

```sql
SELECT authorName
FROM AUTHOR
         LEFT JOIN AUTHORED A ON AUTHOR.authorName = A.author
         LEFT JOIN BOOK B ON B.serialnum = A.serialnum
WHERE ((?) = 'IGNORE_NAME' OR instr(authorName, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?))
GROUP BY authorName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

```json
[
  {
    "authorName": "AUTHOR2"
  },
  {
    "authorName": "AUTHOR1"
  },
  {
    "authorName": "AUTHOR3"
  }
]
```

### Language:

- ?by_name
- ?by_book
- ?by_popularity

```sql
SELECT langCode
FROM LANG
         LEFT JOIN LANGUAGES A ON LANG.langCode = A.lang
         LEFT JOIN BOOK B ON B.serialnum = A.serialnum
WHERE ((?) = 'IGNORE_NAME' OR instr(langCode, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR A.serialnum = (?))
GROUP BY langCode
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

```json
[
  {
    "langcode": "lang2"
  },
  {
    "langcode": "lang1"
  },
  {
    "langcode": "lang3"
  }
]
```

### Action:

- ?by_name

`SELECT actionName FROM ACTION WHERE instr(actionName,(?)) > 0 LIMIT 10 OFFSET (? * 10)`

```json

[
  {
    "actionName": "act1"
  },
  {
    "actionName": "act2"
  },
  {
    "actionName": "ECHOES act3"
  }
]
```

### Doctype:

- ?by_name
- ?by_book
- ?by_popularity

```sql
SELECT typeName
FROM DOCTYPE
         LEFT JOIN BOOK B ON DOCTYPE.typeName = B.type
WHERE ((?) = 'IGNORE_NAME' OR instr(typeName, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
GROUP BY typeName
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

```json
[
  {
    "typeName": "type1"
  },
  {
    "typeName": "type2"
  },
  {
    "typeName": "type3"
  }
]
```

### Campus:

- ?by_name
- ?by_book
- ?by_account

```sql
SELECT campusName
FROM CAMPUS
         LEFT JOIN STOCK S ON CAMPUS.campusName = S.campus
         LEFT JOIN ACCOUNT A on CAMPUS.campusName = A.campus
WHERE ((?) = 'IGNORE_NAME' OR instr(campusName, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
  AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?))
GROUP BY campusName
ORDER BY campusName
LIMIT 10 OFFSET (? * 10)
```

```json

[
  {
    "campusName": "Aboudaou"
  },
  {
    "campusName": "El Kseur"
  },
  {
    "campusName": "Targa Ouzemmour"
  }
]
```

### Role:

- ?by_name
- ?by_perm
- ?by_account

```sql
SELECT roleName, perms
FROM ROLE
         LEFT JOIN ACCOUNT A ON A.role = ROLE.roleName
WHERE ((?) = 'IGNORE_NAME' OR instr(roleName, (?)) > 0)
  AND ((?) = 'IGNORE_PERMS' OR perms = (?))
  AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?))
GROUP BY roleName, perms
ORDER BY perms DESC
LIMIT 10 OFFSET (? * 10)

```

```json
[
  {
    "roleName": "ADMIN",
    "perms": 63
  },
  {
    "roleName": "STAFF",
    "perms": 62
  },
  {
    "roleName": "SHELF MANAGER",
    "perms": 56
  },
  {
    "roleName": "LIBRARIAN",
    "perms": 6
  },
  {
    "roleName": "PROFESSOR",
    "perms": 1
  },
  {
    "roleName": "STUDENT",
    "perms": 1
  }
]
```

### Category:

Initial condition:

- ?by_name
- ?by_class
- ?by_parent
- ?by_book

```sql
SELECT categoryClass, categoryName, parentCategoryID
FROM CATEGORY
         LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category
WHERE IIF((?) = 'ROOT', parentCategoryID IS NULL, TRUE)
  AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0)
  AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?))
  AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?))
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
GROUP BY categoryClass, categoryName, parentCategoryID
ORDER BY IIF((?) = 'POPULAR', SUM(hits), COUNT()) DESC
LIMIT 10 OFFSET (? * 10)
```

ONLY RETURNS THE LEVEL 0 CATEGORIES IF ?by_parent is NOT specified

```json
[
  {
    "categoryClass": "0",
    "categoryName": "Primary1",
    "parentCategoryID": null
  },
  {
    "categoryClass": "1",
    "categoryName": "Primary2",
    "parentCategoryID": null
  }
]
```

- ?cascade
- ?get_parents

```sql
WITH RECURSIVE CategoryCascade AS (SELECT categoryName, categoryClass, parentCategoryID
                                   FROM CATEGORY
                                            LEFT JOIN BOOK B ON CATEGORY.categoryClass = B.category
                                   WHERE IIF((?) = 'ROOT', parentCategoryID IS NULL, TRUE)
                                     AND ((?) = 'IGNORE_NAME' OR instr(categoryName, (?)) > 0)
                                     AND ((?) = 'IGNORE_CLASS' OR categoryClass = (?))
                                     AND ((?) = 'IGNORE_PARENT_CLASS' OR parentCategoryID = (?))
                                     AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
                                   GROUP BY categoryName, categoryClass, parentCategoryID
                                   UNION ALL
                                   SELECT c.categoryName, c.categoryClass, c.parentCategoryID
                                   FROM CATEGORY c
                                            INNER JOIN CategoryCascade ct
                                                       ON IIF((?) = 'GET_PARENTS',
                                                              c.categoryClass = ct.parentCategoryID,
                                                              c.parentCategoryID = ct.categoryClass))
SELECT CATEGORY.categoryClass, CATEGORY.categoryName, CATEGORY.parentCategoryID
FROM CATEGORY,
     CategoryCascade
WHERE CategoryCascade.categoryClass = CATEGORY.categoryClass
GROUP BY CATEGORY.categoryClass, CATEGORY.categoryName, CATEGORY.parentCategoryID
LIMIT 10 OFFSET (? * 10);
```

```json
[
  {
    "categoryClass": "0",
    "categoryName": "Primary1",
    "parentCategoryID": null
  },
  {
    "categoryClass": "1",
    "categoryName": "Primary2",
    "parentCategoryID": null
  },
  {
    "categoryClass": "00",
    "categoryName": "Sec11",
    "parentCategoryID": "0"
  },
  {
    "categoryClass": "01",
    "categoryName": "Sec12",
    "parentCategoryID": "0"
  },
  {
    "categoryClass": "10",
    "categoryName": "Sec21",
    "parentCategoryID": "1"
  },
  {
    "categoryClass": "11",
    "categoryName": "Sec22",
    "parentCategoryID": "1"
  },
  {
    "categoryClass": "000",
    "categoryName": "Sec111",
    "parentCategoryID": "00"
  },
  {
    "categoryClass": "001",
    "categoryName": "Sec112",
    "parentCategoryID": "00"
  }
]

```

Cascade with ?get_parents basically returns all the parents of the category, without any search criteria, it is an
undefined behavior

- ?tree

```sql
SELECT categoryClass, categoryName, parentCategoryID
FROM CATEGORY
WHERE 'INITIAL CONDITION'
SELECT categoryClass, categoryName, parentCategoryID
FROM CATEGORY
WHERE parentCategoryID = (?)
```

recursivity baby :3

```json

[
  {
    "categoryClass": "0",
    "categoryName": "Primary1",
    "parentCategoryID": null,
    "children": [
      {
        "categoryClass": "00",
        "categoryName": "Sec11",
        "parentCategoryID": "0",
        "children": [
          {
            "categoryClass": "000",
            "categoryName": "Sec111",
            "parentCategoryID": "00",
            "children": []
          },
          {
            "categoryClass": "001",
            "categoryName": "Sec112",
            "parentCategoryID": "00",
            "children": []
          }
        ]
      },
      {
        "categoryClass": "01",
        "categoryName": "Sec12",
        "parentCategoryID": "0",
        "children": []
      }
    ]
  },
  {
    "categoryClass": "1",
    "categoryName": "Primary2",
    "parentCategoryID": null,
    "children": [
      {
        "categoryClass": "10",
        "categoryName": "Sec21",
        "parentCategoryID": "1",
        "children": []
      },
      {
        "categoryClass": "11",
        "categoryName": "Sec22",
        "parentCategoryID": "1",
        "children": []
      }
    ]
  }
]
```

### Account:

- ?by_ID
- ?by_name
- ?by_book
- ?by_campus
- ?by_role
- ?by_session
- ?frozen

```sql
SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen
FROM ROLE,
     ACCOUNT
         LEFT JOIN INVENTORY I on ACCOUNT.UUID = I.UUID
         LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account
WHERE ACCOUNT.role = ROLE.roleName
  AND ((?) = 'IGNORE_ID' OR ACCOUNT.UUID = (?))
  AND ((?) = 'IGNORE_NAME' OR instr(displayname, (?)) > 0)
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
  AND ((?) = 'IGNORE_CAMPUS' OR campus = (?))
  AND ((?) = 'IGNORE_ROLE' OR role = (?))
  AND ((?) = 'IGNORE_FREEZE' OR frozen = (?))
  AND ((?) = 'IGNORE_SESSION' OR sessionID = (?))
GROUP BY ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen
ORDER BY displayname
LIMIT 10 OFFSET (? * 10)

```

```json
[
  {
    "UUID": "1",
    "displayname": "Alice",
    "pwhash": "$2b$10$2HqdewExklOfFFCBhOkTP.ufmT.cq1lQP4QSyRRYHETWRGxs3YTH6",
    "campus": "El Kseur",
    "role": "ADMIN",
    "perm": 63,
    "frozen": 0
  },
  {
    "UUID": "2",
    "displayname": "Bob",
    "pwhash": "pwhash2",
    "campus": "El Kseur",
    "role": "LIBRARIAN",
    "perm": 6,
    "frozen": 0
  },
  {
    "UUID": "3",
    "displayname": "Charlie",
    "pwhash": "pwhash3",
    "campus": "Aboudaou",
    "role": "STUDENT",
    "perm": 1,
    "frozen": 0
  }
]
```

**This request fails if: (Unless the user is staff, admin, or has access_history permissions)**

- Accountless user
- A user with an account without the ?me option

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
                                   WHERE IIF((?) = 'ROOT', parentCategoryID IS NULL, categoryClass = (?))
                                   UNION ALL
                                   SELECT c.categoryClass, c.parentCategoryID
                                   FROM CATEGORY c
                                            INNER JOIN CategoryCascade ct ON c.parentCategoryID = ct.categoryClass)
SELECT BOOK.serialnum,
       type,
       category,
       categoryName,
       publisher,
       booktitle,
       bookreleaseyear,
       bookcover,
       hits
FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),
     CATEGORY,
     LANGUAGES,
     AUTHORED,
     STOCK,
     CategoryCascade
WHERE category = CategoryCascade.categoryClass
  AND AUTHORED.serialnum = BOOK.serialnum
  AND LANGUAGES.serialnum = BOOK.serialnum
  AND STOCK.serialnum = BOOK.serialnum
  AND CATEGORY.categoryClass = BOOK.category
  AND ((?) = 'IGNORE_ID' OR BOOK.serialnum = (?))
  AND ((?) = 'IGNORE_NAME' OR instr(booktitle, (?)))
  AND ((?) = 'IGNORE_LANG' OR lang = (?))
  AND ((?) = 'IGNORE_AUTHOR' OR instr(author, (?)) > 0)
  AND ((?) = 'IGNORE_TYPE' OR type = (?))
  AND ((?) = 'IGNORE_PUBLISHER' OR instr(publisher, (?)) > 0)
  AND ((?) = 'IGNORE_CAMPUS' OR campus = (?))
  AND ((?) = 'IGNORE_ACCOUNT' OR UUID = (?))
  AND ((?) = 'INCLUDE_EMPTY' OR STOCK.instock > 0)
  AND ((?) = 'IGNORE_FROM_DATE' OR bookreleaseyear >= (?))
  AND ((?) = 'IGNORE_TO_DATE' OR bookreleaseyear <= (?))
GROUP BY BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits
ORDER BY IIF((?) = 'POPULAR', hits, booktitle) DESC
LIMIT 10 OFFSET (? * 10);

```

```json

[
  {
    "serialnum": "8",
    "type": "type2",
    "category": "11",
    "categoryName": "Sec22",
    "publisher": "pub1",
    "booktitle": "Book 8",
    "bookreleaseyear": 3055,
    "bookcover": null,
    "hits": 19
  },
  {
    "serialnum": "7",
    "type": "type1",
    "category": "10",
    "categoryName": "Sec21",
    "publisher": "pub1",
    "booktitle": "Book 7",
    "bookreleaseyear": 2055,
    "bookcover": null,
    "hits": 93
  },
  {
    "serialnum": "6",
    "type": "type1",
    "category": "00",
    "categoryName": "Sec11",
    "publisher": "pub2",
    "booktitle": "Book 6",
    "bookreleaseyear": 2001,
    "bookcover": null,
    "hits": 95
  },
  {
    "serialnum": "5",
    "type": "type1",
    "category": "000",
    "categoryName": "Sec111",
    "publisher": "pub1",
    "booktitle": "Book 5",
    "bookreleaseyear": 2001,
    "bookcover": null,
    "hits": 94
  },
  {
    "serialnum": "4",
    "type": "type1",
    "category": "001",
    "categoryName": "Sec112",
    "publisher": "pub2",
    "booktitle": "Book 4",
    "bookreleaseyear": 2003,
    "bookcover": null,
    "hits": 15
  },
  {
    "serialnum": "3",
    "type": "type2",
    "category": "01",
    "categoryName": "Sec12",
    "publisher": "pub2",
    "booktitle": "Book 3",
    "bookreleaseyear": 9999,
    "bookcover": null,
    "hits": 7
  },
  {
    "serialnum": "2",
    "type": "type1",
    "category": "1",
    "categoryName": "Primary2",
    "publisher": "pub2",
    "booktitle": "Book 2",
    "bookreleaseyear": 2007,
    "bookcover": null,
    "hits": 9
  },
  {
    "serialnum": "1",
    "type": "type1",
    "category": "0",
    "categoryName": "Primary1",
    "publisher": "pub1",
    "booktitle": "Book 1",
    "bookreleaseyear": 1950,
    "bookcover": null,
    "hits": 0
  }
]
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
WHERE STOCK.serialnum = BOOK.serialnum
  AND ((?) = 'IGNORE_BOOK' OR STOCK.serialnum = (?))
  AND ((?) = 'IGNORE_CAMPUS' OR campus = (?))
  AND IIF((?) = 'AVAILABLE', instock > 0, TRUE)
GROUP BY STOCK.serialnum, campus, instock, hits
ORDER BY IIF((?) = 'POPULAR', hits, instock) DESC
LIMIT 10 OFFSET (? * 10)
```

```json
[
  {
    "serialnum": "7",
    "campus": "Targa Ouzemmour",
    "instock": 74
  },
  {
    "serialnum": "6",
    "campus": "Aboudaou",
    "instock": 7
  },
  {
    "serialnum": "6",
    "campus": "El Kseur",
    "instock": 7
  },
  {
    "serialnum": "4",
    "campus": "El Kseur",
    "instock": 6
  },
  {
    "serialnum": "5",
    "campus": "El Kseur",
    "instock": 6
  },
  {
    "serialnum": "3",
    "campus": "Targa Ouzemmour",
    "instock": 5
  },
  {
    "serialnum": "1",
    "campus": "Aboudaou",
    "instock": 3
  },
  {
    "serialnum": "2",
    "campus": "El Kseur",
    "instock": 2
  },
  {
    "serialnum": "8",
    "campus": "El Kseur",
    "instock": 1
  }
]
```

### Inventory:

- ?by_account | ?me (alias to ?by_account with cookie)
- ?by_book

* [ ] Done

```sql
SELECT UUID, serialnum, rentduration, rentdate, extended
FROM INVENTORY
WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?))
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
GROUP BY UUID, serialnum, rentduration, rentdate, extended
ORDER BY rentdate DESC
LIMIT 10 OFFSET (? * 10)
```

**This request fails if: (Unless the user is staff, admin, or has manage_inventories permissions)**

- Accountless user
- Inventory-less user
- A user with an inventory without the ?me option

```json
[
  {
    UUID,
    UUID_ISSUER,
    serialnum,
    action,
    actiondate
    "UUID": "1",
    "serialnum": "1",
    "rentduration": 44,
    "rentdate": "2025-03-25 01:19:52",
    "extended": 0
  }
]
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
WHERE ((?) = 'IGNORE_ACCOUNT' OR UUID = (?))
  AND ((?) = 'IGNORE_ISSUER' OR UUID_ISSUER = (?))
  AND ((?) = 'IGNORE_BOOK' OR serialnum = (?))
  AND ((?) = 'IGNORE_ACTION' OR action = (?))
  AND ((?) = 'IGNORE_FROM_DATE' OR actiondate >= datetime(?, 'unixepoch'))
  AND ((?) = 'IGNORE_TO_DATE' OR actiondate <= datetime(?, 'unixepoch'))
GROUP BY UUID, UUID_ISSUER, serialnum, action, actiondate
ORDER BY actiondate DESC
LIMIT 10 OFFSET (? * 10)
```

**This request fails if: (Unless the user is staff, admin, or has monitor_history permissions)**

- Accountless user
- A user without the ?me option

```json
[
  {
    "UUID": "1",
    "UUID_ISSUER": "2",
    "serialnum": "6",
    "action": "act2",
    "actiondate": "2028-01-01 15:03:33"
  },
  {
    "UUID": "1",
    "UUID_ISSUER": null,
    "serialnum": "5",
    "action": "act1",
    "actiondate": "2024-12-09 15:03:33"
  }
]

```

NULL in the ISSUER means that the action was done internally(rare but happens)

### Session:

- ?by_account | ?me
- ?by_id
```sql
SELECT account, sessionID, expiresAt
FROM SESSIONS
WHERE ((?) = 'IGNORE_ID' OR sessionID = (?))
  AND ((?) = 'IGNORE_ACCOUNT' OR account = (?))
GROUP BY account, sessionID, expiresAt
ORDER BY expiresAt DESC
LIMIT 10 OFFSET (? * 10)
```