-- This script runs only on first Postgres init (when the data dir is empty).
SELECT 'CREATE DATABASE keycloak OWNER "socoles-web"'
WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'keycloak')\gexec

GRANT ALL PRIVILEGES ON DATABASE keycloak TO "socoles-web";
