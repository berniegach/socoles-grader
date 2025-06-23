CREATE TABLE Employee(
   emp_nr TEXT,
   superv_nr TEXT,
   name TEXT,
   job TEXT,
   hire_date DATE,
   salary NUMERIC(6, 2),
   unit_id INT
);

CREATE TABLE Unit(
  unit_id INT,
  unit_name TEXT
);

CREATE TABLE Dummy(
  dummy_int INT
);

INSERT INTO Unit VALUES (1, 'iCIS');
INSERT INTO Unit VALUES (2, 'CLST');
INSERT INTO Unit VALUES (3, 'Donders');

INSERT INTO Employee VALUES ('U111111', NULL, 'Lutgarde Buydens', 'Dean', '2000-07-01', 5000.00, 1);
INSERT INTO Employee VALUES ('U222222', 'U111111', 'Arjen de Vries', 'Director', '2017-07-01', 4000.00, 1);
INSERT INTO Employee VALUES ('U333333', 'U222222', 'Djoerd Hiemstra', 'Prof', '2019-07-01', 3000.00, 1);
INSERT INTO Employee VALUES ('U444444', 'U333333', 'Hendrik Werner', 'Student', '2016-09-01', 500.00, 1);
INSERT INTO Employee VALUES ('U555555', NULL, 'Henk van den Heuvel', 'Prof', '1999-12-31', 4000.00, 2);
