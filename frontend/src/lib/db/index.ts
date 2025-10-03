import { Pool } from 'pg';
import { AsyncLocalStorage } from 'node:async_hooks';

// Basic Pool singleton. Expects environment variables:
// PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE or DATABASE_URL
// For local dev you can create a .env.local with:
// DATABASE_URL=postgres://user:password@localhost:5432/sqlgrader

let _pool: Pool | null = null;
// AsyncLocalStorage to keep the current PG client inside a scoped instructor context.
const clientStorage = new AsyncLocalStorage<{ client: any }>();

export function getPool() {
  if (_pool) return _pool;
  const connectionString = process.env.DATABASE_URL;
  if (connectionString) {
    _pool = new Pool({ connectionString });
  } else {
    const { PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE } = process.env;
    _pool = new Pool({
      host: PGHOST || 'localhost',
      port: PGPORT ? Number(PGPORT) : 5432,
      user: PGUSER,
      password: PGPASSWORD,
      database: PGDATABASE
    });
  }
  return _pool;
}

export async function query<T = any>(text: string, params?: any[]): Promise<{ rows: T[] }> {
  const store = clientStorage.getStore();
  if (store?.client) {
    const res = await store.client.query(text, params);
    return { rows: res.rows as T[] };
  }
  const pool = getPool();
  const res = await pool.query(text, params);
  return { rows: res.rows as T[] };
}

// Per-request instructor scoping: we use a dedicated connection & SET LOCAL to store instructor id
export async function withInstructorContext<T>(instructorId: string, fn: () => Promise<T>): Promise<T> {
  const pool = getPool();
  const client = await pool.connect();
  return clientStorage.run({ client }, async () => {
    try {
      await client.query('BEGIN');
      // set_config(name, value, is_local=true) -> LOCAL to this transaction
      await client.query(`SELECT set_config($1, $2, true);`, ['app.current_instructor', instructorId]);
      const result = await fn();
      await client.query('COMMIT');
      return result;
    } catch (e) {
      try { await client.query('ROLLBACK'); } catch { /* ignore */ }
      throw e;
    } finally {
      client.release();
    }
  });
}

export async function scopedQuery<T = any>(instructorId: string, text: string, params?: any[]): Promise<{ rows: T[] }> {
  return withInstructorContext(instructorId, () => query<T>(text, params));
}

export async function initSchema() {
  // Fresh schema creation (no migrations)
  await query(`CREATE EXTENSION IF NOT EXISTS pgcrypto;`);

  // Instructors (owners / tenants)
  await query(`CREATE TABLE IF NOT EXISTS instructors (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

  await query(`CREATE TABLE IF NOT EXISTS questions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title TEXT NOT NULL,
    difficulty TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'Draft',
    attempts INT NOT NULL DEFAULT 0,
    max_points INT NOT NULL DEFAULT 0,
    dataset TEXT NOT NULL DEFAULT 'Default',
    prompt TEXT NOT NULL DEFAULT '',
    model_sql TEXT NOT NULL DEFAULT '',
    hints TEXT NOT NULL DEFAULT '',
    model_queries TEXT[] NOT NULL DEFAULT '{}',
    init_sql TEXT NOT NULL DEFAULT '',
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    use_default_grading BOOLEAN NOT NULL DEFAULT true,
    grading_options JSONB NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

  // Reusable datasets containing init SQL that can be referenced by questions
  await query(`CREATE TABLE IF NOT EXISTS datasets (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      name TEXT NOT NULL UNIQUE,
      sql TEXT NOT NULL DEFAULT '',
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
    );`);

  await query(`CREATE TABLE IF NOT EXISTS assignments (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title TEXT NOT NULL,
    course TEXT NOT NULL,
    difficulty TEXT NOT NULL,
    points INT NOT NULL DEFAULT 0,
    due TEXT NOT NULL,
    tags TEXT[] NOT NULL DEFAULT '{}',
    attempts_allowed INT NOT NULL DEFAULT 3 CHECK (attempts_allowed >= 1),
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

  await query(`CREATE TABLE IF NOT EXISTS submissions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    student TEXT NOT NULL,
    assignment TEXT NOT NULL,
    date TEXT NOT NULL,
    grade NUMERIC NOT NULL,
    status TEXT NOT NULL,
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

  // New join table for linking questions to assignments (quizzes)
  await query(`CREATE TABLE IF NOT EXISTS assignment_questions (
      assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE,
      question_id UUID REFERENCES questions(id) ON DELETE CASCADE,
      position INT NOT NULL DEFAULT 0,
      points_override INT NULL,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      PRIMARY KEY (assignment_id, question_id)
    );`);

  await query(`CREATE TABLE IF NOT EXISTS question_submissions (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      submission_id UUID REFERENCES submissions(id) ON DELETE CASCADE,
      assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE,
      question_id UUID REFERENCES questions(id) ON DELETE CASCADE,
      student TEXT NOT NULL,
      sql TEXT NOT NULL DEFAULT '',
      grade NUMERIC NULL,
      status TEXT NOT NULL DEFAULT 'Pending',
      rubric JSONB NULL,
      feedback TEXT[] NOT NULL DEFAULT '{}',
      attempt INT NOT NULL DEFAULT 1,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      UNIQUE (submission_id, question_id)
    );`);

  // per-attempt history table (immutable entries per auto-grade run)
  await query(`CREATE TABLE IF NOT EXISTS question_submission_attempts (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      question_submission_id UUID NULL REFERENCES question_submissions(id) ON DELETE CASCADE,
      submission_id UUID REFERENCES submissions(id) ON DELETE CASCADE,
      assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE,
      question_id UUID REFERENCES questions(id) ON DELETE CASCADE,
      student TEXT NOT NULL,
      sql TEXT NOT NULL DEFAULT '',
      grade NUMERIC NULL,
      status TEXT NOT NULL DEFAULT 'Auto-graded',
      rubric JSONB NULL,
      feedback TEXT[] NOT NULL DEFAULT '{}',
      attempt INT NOT NULL DEFAULT 1,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now()
    );`);

  // Post-quiz micro-survey (one row per student per assignment)
  await query(`CREATE TABLE IF NOT EXISTS assignment_feedback_survey (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE,
      student TEXT NOT NULL,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      helped_fix INT NOT NULL CHECK (helped_fix BETWEEN 1 AND 5),
      improved_understanding INT NOT NULL CHECK (improved_understanding BETWEEN 1 AND 5),
      comment TEXT NULL,
      first_score NUMERIC NULL,
      final_score NUMERIC NULL,
      attempt_count INT NULL,
      improvement NUMERIC NULL,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      UNIQUE(assignment_id, student, owner_id)
    );`);

  // Instructor must be created via signup
  const defaultInstructorId: string | null = null;

  // Class roster per instructor
  await query(`CREATE TABLE IF NOT EXISTS roster (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    email TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'Invited',
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE(owner_id, email)
  );`);

  // Invite tokens for students to join a class roster
  await query(`CREATE TABLE IF NOT EXISTS invites (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    roster_id UUID REFERENCES roster(id) ON DELETE CASCADE,
    token TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL,
    name TEXT NOT NULL,
    expires_at TIMESTAMPTZ NULL,
    used_at TIMESTAMPTZ NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

  // Student secrets: password per roster entry (scoped to instructor via owner_id on roster)
  await query(`CREATE TABLE IF NOT EXISTS student_secrets (
    roster_id UUID PRIMARY KEY REFERENCES roster(id) ON DELETE CASCADE,
    password TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);
  await query(`CREATE INDEX IF NOT EXISTS idx_roster_owner_email ON roster(owner_id, email);`);

  // Ensure expires_at defaults to 14 days if null on insert via trigger-like behavior in app

  // Indexes for tenant isolation performance
  await query(`CREATE INDEX IF NOT EXISTS idx_questions_owner ON questions(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_assignments_owner ON assignments(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_datasets_owner ON datasets(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_submissions_owner ON submissions(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_assignment_questions_owner ON assignment_questions(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_question_submissions_owner ON question_submissions(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_question_submission_attempts_owner ON question_submission_attempts(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_roster_owner ON roster(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_invites_owner ON invites(owner_id);`);
  await query(`CREATE INDEX IF NOT EXISTS idx_invites_token ON invites(token);`);

  // Enable Row Level Security and policies (idempotent)
  const tables = ['questions', 'datasets', 'assignments', 'submissions', 'assignment_questions', 'question_submissions', 'question_submission_attempts', 'roster'];
  for (const t of tables) {
    await query(`ALTER TABLE ${t} ENABLE ROW LEVEL SECURITY;`).catch(() => { });
    // Replace unsupported IF NOT EXISTS with explicit drop/create (idempotent)
    await query(`DROP POLICY IF EXISTS ${t}_select_owner ON ${t};`).catch(() => { });
    await query(`CREATE POLICY ${t}_select_owner ON ${t} FOR SELECT USING (owner_id = current_setting('app.current_instructor', true)::uuid);`).catch(() => { });
    await query(`DROP POLICY IF EXISTS ${t}_mod_owner ON ${t};`).catch(() => { });
    await query(`CREATE POLICY ${t}_mod_owner ON ${t} FOR ALL USING (owner_id = current_setting('app.current_instructor', true)::uuid) WITH CHECK (owner_id = current_setting('app.current_instructor', true)::uuid);`).catch(() => { });
  }

  // Seed demo data once (simple check by counting assignments & submissions)
  const { rows: aCount } = await query<{ count: string }>(`SELECT COUNT(*)::text as count FROM assignments;`);
  if (Number(aCount[0].count) === 0) {
    if (defaultInstructorId) {
      await query(`INSERT INTO assignments (title, course, difficulty, points, due, tags, owner_id) VALUES
        ('Basic SELECT & WHERE', 'DB101 — Intro to SQL', 'Beginner', 10, '2025-09-05', ARRAY['SELECT','WHERE','FILTERS'], $1),
        ('JOINs & Aggregates', 'DB201 — Intermediate SQL', 'Intermediate', 20, '2025-09-12', ARRAY['JOIN','GROUP BY','HAVING'], $1);`, [defaultInstructorId]);
    }
  }
  const { rows: sCount } = await query<{ count: string }>(`SELECT COUNT(*)::text as count FROM submissions;`);
  if (Number(sCount[0].count) === 0) {
    if (defaultInstructorId) {
      await query(`INSERT INTO submissions (student, assignment, date, grade, status, owner_id) VALUES
        ('A. Janssen', 'Basic SELECT & WHERE', '2025-08-28 16:02', 8.5, 'Auto-graded', $1),
        ('M. de Vries', 'JOINs & Aggregates', '2025-08-27 10:41', 6.0, 'Needs review', $1);`, [defaultInstructorId]);
    }
  }
}

// Lightweight ensure for minimal auth bootstrap (so users don't have to manually create instructors table)
export async function ensureInstructorTables() {
  // instructors table (slim version matching initSchema definition)
  await query(`CREATE EXTENSION IF NOT EXISTS pgcrypto;`).catch(() => { });
  await query(`CREATE TABLE IF NOT EXISTS instructors (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);
  await query(`CREATE TABLE IF NOT EXISTS instructor_secrets (
    instructor_id UUID PRIMARY KEY REFERENCES instructors(id) ON DELETE CASCADE,
    password TEXT NOT NULL
  );`);
}
