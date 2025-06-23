CREATE TABLE Album(
  album_id INT,
  title TEXT,
  PRIMARY KEY(album_id)
);

CREATE TABLE Track(
  track_id INT, 
  album_id INT,
  song TEXT,
  duration INT,
  PRIMARY KEY(track_id),
  FOREIGN KEY(album_id) REFERENCES Album(album_id)
);

CREATE TABLE Artist(
  artist_id INT, 
  name TEXT, 
  birth_date DATE,
  death_date DATE,
  PRIMARY KEY(artist_id)
);

CREATE TABLE Performs(
  artist_id INT,
  track_id INT,
  role TEXT,
  PRIMARY KEY(artist_id, track_id, role),
  FOREIGN KEY(artist_id) REFERENCES Artist(artist_id),
  FOREIGN KEY(track_id) REFERENCES Track(track_id)
);

CREATE TABLE Release(
  album_id INT,
  country TEXT,
  release_date DATE,
  PRIMARY KEY (album_id, country),
  FOREIGN KEY(album_id) REFERENCES Album(album_id)
);

CREATE TABLE Dummy(dummy_int INT);

INSERT INTO Artist VALUES (1, 'John Lennon', '1940-10-09', '1980-12-08');
INSERT INTO Artist VALUES (2, 'Paul McCartney', '1942-06-18', NULL);
INSERT INTO Artist VALUES (3, 'George Harrison', '1943-02-25', '2001-11-29');
INSERT INTO Artist VALUES (4, 'Ringo Star', '1940-07-07', NULL);
INSERT INTO Artist VALUES (5, 'Kurt Kobain', '1967-02-20', '1994-04-05');
INSERT INTO Artist VALUES (11, 'Frank Sloos', '1970-10-15', NULL);
INSERT INTO Artist VALUES (12, 'Frank Gerritsen', '1971-02-20', NULL);
INSERT INTO Artist VALUES (13, 'Otis Redding', '1941-09-09', '1967-12-10');
INSERT INTO Artist VALUES (14, 'Paul Heaton', '1962-05-09', NULL);
INSERT INTO Artist VALUES (15, 'Stan Cullimore', '1962-05-06', NULL);
INSERT INTO Artist VALUES (16, 'Ted Key', '1960-07-01', NULL);
INSERT INTO Artist VALUES (17, 'Fatboy Slim', '1963-07-31', NULL);
INSERT INTO Artist VALUES (18, 'Hugh Whitaker', '1961-05-18', NULL);

INSERT INTO Album VALUES (1, 'Please Please Me');
INSERT INTO Album VALUES (2, 'With the Beatles');
INSERT INTO Album VALUES (4, 'Nevermind');
INSERT INTO Album VALUES (5, 'Hasheeda');
INSERT INTO Album VALUES (6, 'Dictionary of Soul');
INSERT INTO Album VALUES (7, 'Parklife');
INSERT INTO Album VALUES (8, 'Now That''s What I Call Quite Good');

INSERT INTO Track VALUES (1, 1, 'I Saw Her Standing There', 163);
INSERT INTO Track VALUES (2, 1, 'Misery', 180);
INSERT INTO Track VALUES (3, 1, 'Anna', 199);
INSERT INTO Track VALUES (4, 1, 'Chains', 169);
INSERT INTO Track VALUES (11, 2, 'It Won''t Be Long', 200);
INSERT INTO Track VALUES (12, 2, 'All I''ve Got To Do', 180);
INSERT INTO Track VALUES (21, 4, 'Smells Like Teen Spirit', 180);
INSERT INTO Track VALUES (22, 4, 'In Bloom', 199);
INSERT INTO Track VALUES (23, 4, 'Come as You Are', 159);
INSERT INTO Track VALUES (24, 4, 'Breed', 162);
INSERT INTO Track VALUES (25, 4, 'Lithium', 174);
INSERT INTO Track VALUES (26, 4, 'Polly', 189);
INSERT INTO Track VALUES (31, 5, 'Red Hot Navigator', 224);
INSERT INTO Track VALUES (32, 5, 'Hasheeda', 208);
INSERT INTO Track VALUES (33, 5, 'Samira', NULL);
INSERT INTO Track VALUES (41, 6, 'Love Have Mercy', 149);
INSERT INTO Track VALUES (51, 7, 'Girls & Boys', 290);
INSERT INTO Track VALUES (52, 7, 'End of a Century', 166);
INSERT INTO Track VALUES (53, 7, 'Parklife', 185);
INSERT INTO Track VALUES (61, 8, 'Caravan Of Love', 185);


INSERT INTO Performs VALUES (1, 1, 'guitarist');
INSERT INTO Performs VALUES (1, 1, 'vocals');
INSERT INTO Performs VALUES (2, 1, 'bassist');
INSERT INTO Performs VALUES (2, 1, 'vocals');
INSERT INTO Performs VALUES (3, 1, 'guitarist');
INSERT INTO Performs VALUES (3, 1, 'vocals');
INSERT INTO Performs VALUES (4, 1, 'drums');
INSERT INTO Performs VALUES (1, 2, 'guitarist');
INSERT INTO Performs VALUES (1, 2, 'vocals');
INSERT INTO Performs VALUES (2, 2, 'bassist');
INSERT INTO Performs VALUES (2, 2, 'vocals');
INSERT INTO Performs VALUES (3, 2, 'guitarist');
INSERT INTO Performs VALUES (3, 2, 'vocals');
INSERT INTO Performs VALUES (4, 2, 'drums');
INSERT INTO Performs VALUES (1, 3, 'guitarist');
INSERT INTO Performs VALUES (1, 3, 'vocals');
INSERT INTO Performs VALUES (2, 3, 'bassist');
INSERT INTO Performs VALUES (2, 3, 'vocals');
INSERT INTO Performs VALUES (3, 3, 'guitarist');
INSERT INTO Performs VALUES (3, 3, 'vocals');
INSERT INTO Performs VALUES (4, 3, 'drums');
INSERT INTO Performs VALUES (1, 4, 'guitarist');
INSERT INTO Performs VALUES (1, 4, 'vocals');
INSERT INTO Performs VALUES (2, 4, 'bassist');
INSERT INTO Performs VALUES (2, 4, 'vocals');
INSERT INTO Performs VALUES (3, 4, 'guitarist');
INSERT INTO Performs VALUES (4, 4, 'drums');
INSERT INTO Performs VALUES (1, 11, 'guitarist');
INSERT INTO Performs VALUES (1, 11, 'vocals');
INSERT INTO Performs VALUES (2, 11, 'bassist');
INSERT INTO Performs VALUES (2, 11, 'vocals');
INSERT INTO Performs VALUES (3, 11, 'guitarist');
INSERT INTO Performs VALUES (4, 11, 'drums');
INSERT INTO Performs VALUES (1, 12, 'guitarist');
INSERT INTO Performs VALUES (1, 12, 'vocals');
INSERT INTO Performs VALUES (2, 12, 'bassist');
INSERT INTO Performs VALUES (2, 12, 'vocals');
INSERT INTO Performs VALUES (3, 12, 'guitarist');
INSERT INTO Performs VALUES (4, 12, 'drums');
INSERT INTO Performs VALUES (5, 21, 'vocals');
INSERT INTO Performs VALUES (5, 21, 'guitarist');
INSERT INTO Performs VALUES (5, 22, 'vocals');
INSERT INTO Performs VALUES (5, 22, 'guitarist');
INSERT INTO Performs VALUES (5, 23, 'vocals');
INSERT INTO Performs VALUES (5, 23, 'guitarist');
INSERT INTO Performs VALUES (5, 24, 'vocals');
INSERT INTO Performs VALUES (5, 24, 'guitarist');
INSERT INTO Performs VALUES (11, 31, 'drums');
INSERT INTO Performs VALUES (12, 31, 'guitarist');
INSERT INTO Performs VALUES (11, 32, 'drums');
INSERT INTO Performs VALUES (12, 32, 'guitarist');
INSERT INTO Performs VALUES (13, 41, 'vocals');
INSERT INTO Performs VALUES (14, 61, 'vocals');
INSERT INTO Performs VALUES (15, 61, 'vocals');
INSERT INTO Performs VALUES (16, 61, 'vocals');
INSERT INTO Performs VALUES (17, 61, 'vocals');
INSERT INTO Performs VALUES (18, 61, 'vocals');

INSERT INTO Release VALUES(1, 'United Kingdom', '1963-03-22');
INSERT INTO Release VALUES(1, 'Netherlands', '1963-03-22');
INSERT INTO Release VALUES(2, 'United Kingdom', '1963-11-22');
INSERT INTO Release VALUES(2, 'Netherlands', '1963-11-22');
INSERT INTO Release VALUES(2, 'Belgium', '1963-11-22');
INSERT INTO Release VALUES(4, 'Netherlands', NULL);
INSERT INTO Release VALUES(5, 'Netherlands', NULL);
INSERT INTO Release VALUES(4, 'Iceland', '1964-05-02');
INSERT INTO Release VALUES(5, 'Iceland', NULL);
INSERT INTO Release VALUES(7, 'Netherlands', '1994-04-25');
INSERT INTO Release VALUES(7, 'France', '1994-04-25');
INSERT INTO Release VALUES(7, 'Kenya', '1994-04-25');
INSERT INTO Release VALUES(8, 'Netherlands', NULL);


