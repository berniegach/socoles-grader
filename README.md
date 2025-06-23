# SOCOLES-Grader

SOCOLES is a C++ powered SQL auto-grader with a JavaScript frontend

## Components

- **backend/** - C++ server engine that grades queries  
- **frontend/** - JS app for submitting queries & viewing results  

> For detailed build & run instructions, see the [backend README](backend/README.md) and [frontend README](frontend/README.md).

## ✨ Features

- **SELECT statements**
  Capable of grading SELECT (DQL) Queries.

- **DDL/DML statements**  
  Extends grading beyond SELECT (DQL) to include Data Definition (CREATE, ALTER, DROP) and Data Manipulation (INSERT, UPDATE, DELETE) statements.

- **Clause-level Analysis**  
  Evaluates each individual clause (e.g. `SELECT`, `FROM`, `WHERE`, `GROUP BY`) in isolation, so you get fine-grained insight into query structure.

- **Clause-level Feedback**  
  Generates feedback at the clause or component level (e.g. pointing out a missing `CASCADE` in a `DROP TABLE`), helping students pinpoint exactly what went wrong.

- **Static & Dynamic Analysis**  
  *Static* checks the syntax tree against expected patterns; *dynamic* runs the query and inspects actual results for correctness.

- **Query Repair**  
  Auto-corrects minor syntax and semantic mistakes (typos, missing keywords) and suggests fixes.

- **Configurable Grading**  
  Instructors can tune:
  - Which properties to grade (syntax, semantics, results)  
  - Number of outcomes per property  
  - Property priorities  
  - Text/tree-edit distance thresholds

- **Correctness Levels**  
  Supports multiple levels of correctness.

- **Constraint Support**  
  Checks and grades table constraints like `PRIMARY KEY`, `UNIQUE`, `CHECK`, and foreign keys.


## 🚀 Quick Start


```bash
# 1. Clone the repo
git clone https://github.com/berniegach/socoles-grader.git
cd socoles-grader

# 2. Build backend
cd backend
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 3. Create & configure PostgreSQL database
#    This will create an empty database named `grader` and
#    give your current shell user ownership so the backend can connect.
createdb grader
psql -c "GRANT ALL PRIVILEGES ON DATABASE grader TO $(whoami);"

# 4. Start backend server
#    Note the `--db-name` now points to `grader`
./socoles_server 

# 5. Build & run frontend
cd ../../frontend
npm install
npm start
```

## 📚 Citation

If you use SOCOLES in your research, please cite it as follows:

```bibtex
@software{socoles-autograder,
  author       = {Your Name and Coauthors},
  title        = {SOCOLES-Grader: An Automatic SQL Query Grading Tool},
  version      = {v1.0.0},
  url          = {https://github.com/berniegach/socoles-grader},
  year         = {2025},
  month        = {Jun},
  note         = {Accessed: YYYY-MM-DD}
}
```

## Publications

Please also cite these publications that describe the grading methodology and evaluation:

```bibtex
@InProceedings{benard:2024,
author={Wanjiru, Benard and Bommel, Patrick van and Hiemstra, Djoerd},
editor="Daimi, Kevin
and Al Sadoon, Abeer",
title="Sensitivity of Automated SQL Grading in Computer Science Courses",
booktitle="Proceedings of the Third International Conference on Innovations in Computing Research (ICR'24)",
year="2024",
publisher="Springer Nature Switzerland",
address="Cham",
pages="283--299",
isbn="978-3-031-65522-7",
doi = {doi.org/10.1007/978-3-031-65522-7_26}
}


@article{benard:jenrs,
author = {Wanjiru, Benard and Bommel, Patrick van and Hiemstra, Djoerd},
doi = {10.55708/js0308001} ,
journal = {Journal of Engineering Research and Sciences},
number = {},
pages = {},
title = {{Dynamic and Partial Grading of SQL Queries}},
volume = {},
year = {2024}
}

@inproceedings{DBLP:conf/icse-seeng/WanjiruBH23,
  author       = {Benard Wanjiru and Patrick van Bommel and  Djoerd Hiemstra},
  title        = {Towards a Generic Model for Classifying Software into Correctness
                  Levels and its Application to {SQL}},
  booktitle    = {5th {IEEE/ACM} International Workshop on Software Engineering Education
                  for the Next Generation, SEENG@ICSE 2023, Melbourne, Australia, May
                  16, 2023},
  pages        = {37--40},
  publisher    = {{IEEE}},
  year         = {2023},
  url          = {https://doi.org/10.1109/SEENG59157.2023.00012},
  doi          = {10.1109/SEENG59157.2023.00012},
  timestamp    = {Tue, 12 Sep 2023 01:00:00 +0200},
  biburl       = {https://dblp.org/rec/conf/icse-seeng/WanjiruBH23.bib},
  bibsource    = {dblp computer science bibliography, https://dblp.org}
}
```


