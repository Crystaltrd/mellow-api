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
