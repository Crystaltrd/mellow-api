API calls:


## / :
Return Endpoints

## auth/:
- Args: UUID, passhash, type
- Returns: Session Cookie
## deauth/:
Removes session cookie
## query/:
Return usage
### query/books
- Args(optional): ISBN13, title, author, publisher, release_date, campus,available, genre(s)
- Returns: Array of books matching arguments.
### query/campuses
- Args: None
- Returns: Array of campus-rowid pairs
### query/rentees (auth req)
- Args: Matricule
- Returns: Rentee Attributes
### query/librarians (auth req)
- Args: UUID
- Returns: Librarian Attributes
### query/inventory (auth req)
- Args(optional): rentee,book
- Returns: Books that rentee has. Or Returns the rentee of a book
### query/admins (auth req)
- Args: UUID
- Returns: Admin Attributes
