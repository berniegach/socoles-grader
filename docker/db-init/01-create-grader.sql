-- Create secondary database (grader) after primary manageclass has been created by POSTGRES_DB
-- Uses the single application role ${POSTGRES_USER} (injected at container start)
-- Note: environment variables are not expanded inside SQL; role must already exist (created implicitly as superuser/owner by official image)

CREATE DATABASE grader OWNER "socoles-web";

-- Basic privileges (owner already all-powerful, but future-proof explicit GRANT example)
GRANT ALL PRIVILEGES ON DATABASE grader TO "socoles-web";
