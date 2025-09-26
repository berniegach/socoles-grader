-- Init script: creates front-user role and manageclass database on FIRST container init only.
-- Runs only when PGDATA is empty (fresh volume). Safe to keep in repo.
-- Assumes POSTGRES_USER=back-user already exists (the owning super/primary user) when this runs.

-- Create the role for frontend (LOGIN allows connections)
CREATE ROLE "front-user" LOGIN PASSWORD 'ajsdfgfhjgdfhdg$87676g5';  -- REPLACE_FRONT_PASSWORD

-- Create the database owned by front-user (front-user is the owner now)
CREATE DATABASE manageclass OWNER "front-user";

-- Allow future connections by owner automatically; still, explicit CONNECT doesn't hurt.
GRANT CONNECT ON DATABASE manageclass TO "front-user";

-- Switch to the new database to set up schema/table privileges
\connect manageclass

-- Grant schema usage (public is default schema)
GRANT USAGE ON SCHEMA public TO "front-user";
-- Allow CRUD on existing tables (none yet, but future tables need default privileges updated)
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO "front-user";
GRANT USAGE, SELECT, UPDATE ON ALL SEQUENCES IN SCHEMA public TO "front-user";

-- Ensure future tables/sequences also grant privileges automatically:
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO "front-user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT USAGE, SELECT, UPDATE ON SEQUENCES TO "front-user";


