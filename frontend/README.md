
## Database (PostgreSQL)

The app uses PostgreSQL for persisting questions, assignments, submissions and other data.

### Environment variables
Create a `.env.local` file in the project root.

```
PGHOST=localhost
PGPORT=5432
PGUSER=user
PGPASSWORD=password
PGDATABASE=sqlgrader
```

### Creating the PostgreSQL role & database manually

In your local Postgres install (e.g. Debian/Ubuntu), create a dedicated role (user) and database before starting the app. Replace `STRONG_PASSWORD` with a long unique password. These information should match your `.env.local` file. 

1. Switch to the postgres system user and open psql:

```bash
sudo -iu postgres
psql
```

2. Create role and database:

```sql
CREATE ROLE user WITH LOGIN PASSWORD 'STRONG_PASSWORD';
CREATE DATABASE sqlgrader OWNER user;
```

3. Exit psql and the postgres user:

```sql
\q
```
```bash
exit
```


4. Install dependencies and start the development server:

```bash
npm install
npm run dev
```

This will install all required packages and launch the app in development mode.


