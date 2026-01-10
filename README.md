# SOCOLES

SOCOLES is an automated grading system for evaluating SQL statements. It can evaluate Data Query Language (SELECT), Data Definition Language (DDL), and Data Manipulation Language (DML) statements. The system supports 3 types of users:
  - Students
  - Instructors
  - Teaching Assistants 

## ✨ Features
- **DQL, DDL and DML statements**
  Capable of grading DQL(`SELECT`) Queries, DDL statements such as `CREATE`, `ALTER`, `DROP` and DML statements such as `INSERT`, `UPDATE`, `DELETE`.
- **Detailed and constructive feedback**
  Generates feedback at the clause or component level (e.g. pointing out a missing `CASCADE` in a `DROP TABLE`), helping students pinpoint exactly what went wrong. This is possible through the evaluation of each individual clause (e.g. `SELECT`, `FROM`, `WHERE`, `GROUP BY`) in isolation, to generate fine-grained feedback.

- **Query Repair**  
  Auto-corrects minor syntax and semantic mistakes (typos, missing keywords) to help in grading malformed statements.

- **Multiple Submissions**  
  Supports multiple attempts per question, therefore a student a work on an answer until they get it right.

- **Feedback-Linked Manual Review Requests**  
   Allows students to select a specific feedback item or grade and request a manual review when they disagree with the evaluation. Both the teaching assistants and instructors can see these requests.

- **Configurable Grading**  
  Instructors can tune:
  - Which properties to grade (syntax, semantics, results)  
  - Number of outcomes per property  
  - Property priorities  
  - Text/tree-edit distance thresholds



## 🐳 Docker Quick Start

This is the easiest way to get SOCOLES up and running, with PostgreSQL, backend, and frontend all in Docker containers. Make sure you have Docker installed.

### 1. Clone the repo
```bash
git clone https://github.com/berniegach/socoles-grader.git
cd socoles-grader
```

### 2. Configure environment (.env)
Copy `.env.example` to `.env` and edit as needed.

Minimum required for local development:
```bash
POSTGRES_PASSWORD=change-me-strong
SOCOLES_VERSION=1.1.5
# (optional) defaults already work for localhost
CORS_ORIGINS=http://localhost:3000,http://127.0.0.1:3000
NEXT_PUBLIC_SOCOLES_API_URL=http://localhost:5000
```

For a custom HTTPS domain:
```bash
POSTGRES_PASSWORD=your-strong-password
SOCOLES_VERSION=1.1.5
CORS_ORIGINS=https://socoles.example.edu #comma-separated, no spaces
NEXT_PUBLIC_SOCOLES_API_URL=https://socoles.example.edu #what the browser calls
# Internal service URL usually stays as-is for Docker:
SOCOLES_INTERNAL_API_URL=http://backend:5000
```

Notes:
- `NEXT_PUBLIC_SOCOLES_GRADE_PATH` is fixed to `/grade-queries` in `docker-compose.yml` (no need to set it).
- Frontend DB env (PGHOST/PGPORT/PGUSER/PGDATABASE) are defined in `docker-compose.yml` and do not need to be set in `.env`.

### 3. Start everything
```bash
docker compose up -d
```

Optional: build images locally instead of pulling prebuilt ones (uncomment the `build:` sections in `docker-compose.yml` first):
```bash
docker compose build
docker compose up -d
```

You can now access the webapp at http://localhost:3000 or the custom https://socoles.example.edu



## 📚 Citation

If you use SOCOLES in your research, please cite it as follows:

```bibtex
@software{socoles-autograder,
  author       = {Benard Wanjiru},
  title        = {SOCOLES: An Automatic SQL Query Grading System},
  version      = {v1.1.5},
  url          = {https://github.com/berniegach/socoles-grader},
  year         = {2026},
  month        = {Jan},
  note         = {Accessed: YYYY-MM-DD}
}
```

## Publications

Feel free to check this publications that describe the grading methodology and evaluation:

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


