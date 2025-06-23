CREATE TABLE Dummy(dummy_int INT);

CREATE TABLE Journal(
  issn TEXT,
  journal_name TEXT,
  start_year NUMERIC(4, 0) NOT NULL,
  end_year NUMERIC(4, 0), 
  PRIMARY KEY(issn),
  CONSTRAINT history CHECK (end_year IS NULL OR end_year >= start_year)
);

CREATE TABLE Paper(
  doi TEXT,
  title TEXT,
  year NUMERIC(4, 0),
  issn TEXT,
  PRIMARY KEY(doi),
  FOREIGN KEY(issn) REFERENCES Journal(issn) ON UPDATE CASCADE
);

CREATE TABLE Person(
  email TEXT PRIMARY KEY,
  person_name TEXT
);

CREATE TABLE Written_by(
  doi TEXT,
  email TEXT,
  sequence_nr INT,
  FOREIGN KEY (doi) REFERENCES Paper(doi),
  FOREIGN KEY (email) REFERENCES Person(email)
);

CREATE TABLE Edited_by(
  issn TEXT,
  email TEXT,
  FOREIGN KEY (issn) REFERENCES Journal(issn),
  FOREIGN KEY (email) REFERENCES Person(email)
);

CREATE TABLE Cites(
  doi TEXT,
  cites_doi TEXT,
  FOREIGN KEY (doi) REFERENCES Paper(doi),
  FOREIGN KEY (cites_doi) REFERENCES Paper(doi)
);

INSERT INTO Person(email, person_name) VALUES 
  ('ser@ucl.ac.uk', 'John Doe'),
  ('ksj@cam.ac.uk', 'Karen Sparck-Jones'),
  ('hiemstra@cs.ru.nl', 'Djoerd Hiemstra'),
  ('n.ghasemi@cs.ru.nl', 'Negin Ghasemi'),
  ('lavrenko@umass.edu', 'Victor Lavrenko'),
  ('croft@umass.edu', 'Bruce Croft'),
  ('zhang@tsinghua.edu.cn', 'Yi Zhang'),
  ('editor@utwente.nl', 'Ed Itor'),
  ('devil@hell.com', 'Devil''s Advocate');

INSERT INTO Journal(issn, journal_name, start_year) VALUES
  ('2330-1635', 'Journal of the American Society of Information Science', 1960),
  ('1381-3617', 'University of Twente PhD thesis', 1969),
  ('1236-9821', 'Proceedings of the Conference of the European Chapter of the ACL', 1981),
  ('3214-2312', 'Proceedings of the ACM SIGIR Conference', 1974);

INSERT INTO Paper(doi, title, year, issn) VALUES
  ('10.1002/asi.4630270302', 'Relevance weighting of search terms', 1976, '2330-1635'),
  ('10.3990/1.9075296053', 'Using Language Models for Information Retrieval', 2001, '1381-3617'),
  ('10.18653/v1/2021.eacl-srw.9', 'BERT meets Cranfield: Uncovering the Properties of Full Ranking on Fully Labeled Data', 2021, '1236-9821'),
  ('10.1145/383952.383972', 'Relevance-based Language Models', 2001, '3214-2312'),
  ('10.1145/1008992.1009052','Using bayesian priors to combine classifiers for adaptive filtering', 2004, '3214-2312'), 
  ('x/y1', 'Fake paper 1', 2021, '3214-2312'),
  ('x/y2', 'Fake paper 2', 2021, '3214-2312'),
  ('x/y3', 'Fake paper 3', 2021, '3214-2312'),
  ('x/y4', 'Fake paper 4', 2021, '3214-2312'),
  ('x/y5', 'Fake paper 5', 2021, '1236-9821'),
  ('10.666/666.6666', 'A scam by the devil''s advocate', 2022, '2330-1635');

INSERT INTO Written_by(doi, email, sequence_nr) VALUES
  ('10.1002/asi.4630270302', 'ser@ucl.ac.uk', 1),
  ('10.1002/asi.4630270302', 'ksj@cam.ac.uk', 2),
  ('10.3990/1.9075296053', 'hiemstra@cs.ru.nl', 1),
  ('10.18653/v1/2021.eacl-srw.9', 'n.ghasemi@cs.ru.nl', 1),
  ('10.18653/v1/2021.eacl-srw.9', 'hiemstra@cs.ru.nl', 2),
  ('10.1145/383952.383972', 'lavrenko@umass.edu', 1),
  ('10.1145/383952.383972', 'croft@umass.edu', 2),
  ('10.1145/1008992.1009052', 'zhang@tsinghua.edu.cn', 1),
  ('10.666/666.6666', 'devil@hell.com', 1);

INSERT INTO Cites(doi, cites_doi) VALUES 
  ('10.1002/asi.4630270302', '10.666/666.6666'),
  ('10.3990/1.9075296053', '10.1002/asi.4630270302'),
  ('10.1145/383952.383972', '10.3990/1.9075296053'),
  ('10.18653/v1/2021.eacl-srw.9', '10.1145/383952.383972'),
  ('10.1145/1008992.1009052', '10.1002/asi.4630270302');  

INSERT INTO Edited_by(issn, email) VALUES
  ('1381-3617', 'editor@utwente.nl'),
  ('3214-2312', 'ser@ucl.ac.uk'),
  ('2330-1635', 'devil@hell.com');

