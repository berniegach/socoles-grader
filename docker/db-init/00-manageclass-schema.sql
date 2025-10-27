-- Base schema for the primary application database (manageclass)
-- Runs only on first Postgres initialization (empty data dir)

-- Required for gen_random_uuid()
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Instructors (owners / tenants)
CREATE TABLE IF NOT EXISTS instructors (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS instructor_secrets (
  instructor_id UUID PRIMARY KEY REFERENCES instructors(id) ON DELETE CASCADE,
  password TEXT NOT NULL
);
