CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL,author VARCHAR,PRIMARY KEY(isbn));
CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, 
  PRIMARY KEY(isbn, serial_number), 
  FOREIGN KEY(isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE);
INSERT INTO Book(isbn, title, author) 
  VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');
INSERT INTO Copy(isbn, serial_number, weight, bookcase) VALUES ('9781292025827', 1, 200, 81);
INSERT INTO Copy(isbn, serial_number, weight, bookcase) VALUES ('9781292025827', 2, 200, 81);