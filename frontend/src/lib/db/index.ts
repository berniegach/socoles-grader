import { Pool } from 'pg';
import { AsyncLocalStorage } from 'node:async_hooks';

// Basic Pool singleton. Expects environment variables:
// PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE or DATABASE_URL
// For local dev you can create a .env.local with:
// DATABASE_URL=postgres://user:password@localhost:5432/sqlgrader

let _pool: Pool | null = null;
// AsyncLocalStorage to keep the current PG client inside a scoped instructor context.
const clientStorage = new AsyncLocalStorage<{ client: any }>();

/**
 * Get the shared Postgres connection pool.
 * 1. Creating a new DB connection for every query is slow.
 * 2. A pool reuses connections, so requests are faster and more stable.
 * 3. This function keeps a single pool for the whole app.
 *
 * In short: it gives us one reusable pool to run SQL queries.
 */
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

/**
 * Runs a SQL query using the current request-scoped DB connection if we have one.
 *
 * Why do this: when a request is running inside `withInstructorContext` / `withCourseContext`,
 * we want every query to use the same transaction (and the same RLS/session settings).
 * If there is no scoped connection, we fall back to the shared pool.
 */
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

/**
 * Runs a function inside an instructor-scoped DB transaction.
 *
 * - RLS policies read `app.current_instructor` (and often `app.current_course`).
 * - We set those values using `SET LOCAL` so they only apply to this transaction.
 * - We also reuse one connection for the whole request, so all queries see the same settings.
 */
export async function withInstructorContext<T>(instructorId: string, fn: () => Promise<T>): Promise<T> {
  const pool = getPool();
  const client = await pool.connect();
  return clientStorage.run({ client }, async () => {
    try {
      await client.query('BEGIN');
      // set_config(name, value, is_local=true) -> LOCAL to this transaction
      // For the duration of this transaction, store the current instructor id in app.current_instructor
      await client.query(`SELECT set_config($1, $2, true);`, ['app.current_instructor', instructorId]);

      // Also scope to a default course for this instructor.
      // This keeps older API routes working without explicitly passing a course id.
      try {
        const { rows: courseRows } = await client.query(
          `SELECT COALESCE(
              (SELECT default_course_id FROM instructor_settings WHERE instructor_id=$1 LIMIT 1),
              (SELECT id FROM courses WHERE owner_id=$1 ORDER BY created_at ASC LIMIT 1)
            ) AS course_id`,
          [instructorId]
        );
        const courseId = courseRows?.[0]?.course_id as string | null | undefined;
        if (courseId) {
          await client.query(`SELECT set_config($1, $2, true);`, ['app.current_course', courseId]);
        }
      } catch {
        // ignore: during early bootstrap (before courses exist)
      }

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

/**
 * Runs a function inside a course-scoped DB transaction.
 *
 * Why do this: course data is protected by RLS using `app.current_instructor` and
 * `app.current_course`. Setting both values on a single connection/transaction makes
 * sure every query in `fn()` only sees rows for that instructor + course.
 */
export async function withCourseContext<T>(instructorId: string, courseId: string, fn: () => Promise<T>): Promise<T> {
  const pool = getPool();
  const client = await pool.connect();
  return clientStorage.run({ client }, async () => {
    try {
      await client.query('BEGIN');
      // For the duration of this transaction, store the current instructor id in app.current_instructor
      await client.query(`SELECT set_config($1, $2, true);`, ['app.current_instructor', instructorId]);
      // Also set the current course id in app.current_course
      await client.query(`SELECT set_config($1, $2, true);`, ['app.current_course', courseId]);
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

/**
 * Runs a query inside an optional instructor + course context.
 * If courseId is provided, the query runs scoped to that course.
 * If not, it runs scoped only to the instructor.
 * */
export async function scopedQuery<T = any>(
  instructorId: string,
  text: string,
  params?: any[],
  courseId?: string
): Promise<{ rows: T[] }> {
  if (courseId) {
    return withCourseContext(instructorId, courseId, () => query<T>(text, params));
  }
  return withInstructorContext(instructorId, () => query<T>(text, params));
}

export async function initSchema() {
  // Serialize schema initialization to avoid concurrent DDL deadlocks
  await query(`SELECT pg_advisory_lock(12345, hashtext('sqlgrader.init.v1'))`);
  try {
    // Fresh schema creation (no migrations)
    await query(`CREATE EXTENSION IF NOT EXISTS pgcrypto;`);

    // Instructors (owners / tenants) is created by Docker init SQL.
    // We only validate it exists here, because many other tables depend on it.
    const { rows: instructorsTable } = await query<{ ok: boolean }>(
      `SELECT EXISTS (
         SELECT 1
         FROM information_schema.tables
         WHERE table_schema='public' AND table_name='instructors'
       ) AS ok;`
    );
    if (!instructorsTable?.[0]?.ok) {
      throw new Error(
        "Missing required table 'instructors'. If you're running locally, ensure the Postgres schema init ran (see docker/db-init/00-manageclass-schema.sql)."
      );
    }

    // Instructor settings for cross-device persistence
    await query(`CREATE TABLE IF NOT EXISTS instructor_settings (
    instructor_id UUID PRIMARY KEY REFERENCES instructors(id) ON DELETE CASCADE,
    course_name TEXT NOT NULL DEFAULT '',
    enrollment_code TEXT NOT NULL DEFAULT '',
    attempts INT NOT NULL DEFAULT 3,
    late_penalty NUMERIC NOT NULL DEFAULT 0.2,
    pass_threshold NUMERIC NOT NULL DEFAULT 0.6,
    grading_defaults JSONB
  );`);

    // Add multi-course pointer (idempotent)
    await query(`ALTER TABLE instructor_settings ADD COLUMN IF NOT EXISTS default_course_id UUID NULL;`);

    // Migrate late_penalty to NUMERIC if needed
    try {
      await query('ALTER TABLE instructor_settings ALTER COLUMN late_penalty TYPE NUMERIC USING late_penalty::numeric;');
      await query('ALTER TABLE instructor_settings ALTER COLUMN late_penalty SET DEFAULT 0.2;');
    } catch (e) { /* ignore if already numeric */ }

    // Courses: instructors can own multiple independent courses
    await query(`CREATE TABLE IF NOT EXISTS courses (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    name TEXT NOT NULL DEFAULT '',
    enrollment_code TEXT NOT NULL DEFAULT '',
    attempts INT NOT NULL DEFAULT 3,
    late_penalty NUMERIC NOT NULL DEFAULT 0.2,
    pass_threshold NUMERIC NOT NULL DEFAULT 0.6,
    grading_defaults JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);
    await query(`CREATE INDEX IF NOT EXISTS idx_courses_owner ON courses(owner_id);`);
    await query(`CREATE UNIQUE INDEX IF NOT EXISTS idx_courses_owner_enrollment_code ON courses(owner_id, enrollment_code);`);

    // Ensure default course exists for every instructor (idempotent)
    await query(`INSERT INTO courses (owner_id, name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults)
      SELECT i.id,
             COALESCE(NULLIF(s.course_name, ''), 'Course'),
             COALESCE(NULLIF(s.enrollment_code, ''), ''),
             COALESCE(s.attempts, 3),
             COALESCE(s.late_penalty, 0.2),
             COALESCE(s.pass_threshold, 0.6),
             s.grading_defaults
      FROM instructors i
      LEFT JOIN instructor_settings s ON s.instructor_id = i.id
      WHERE NOT EXISTS (SELECT 1 FROM courses c WHERE c.owner_id = i.id);
    `);

    // Backfill instructor_settings.default_course_id (idempotent)
    await query(`UPDATE instructor_settings s
      SET default_course_id = c.id
      FROM courses c
      WHERE s.default_course_id IS NULL
        AND c.owner_id = s.instructor_id
        AND c.id = (SELECT id FROM courses c2 WHERE c2.owner_id = s.instructor_id ORDER BY created_at ASC LIMIT 1);
    `);

    // If instructor_settings row is missing, create it and point to the earliest course
    await query(`INSERT INTO instructor_settings (instructor_id, course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults, default_course_id)
      SELECT i.id,
             COALESCE(NULLIF(c.name, ''), ''),
             COALESCE(NULLIF(c.enrollment_code, ''), ''),
             COALESCE(c.attempts, 3),
             COALESCE(c.late_penalty, 0.2),
             COALESCE(c.pass_threshold, 0.6),
             c.grading_defaults,
             c.id
      FROM instructors i
      JOIN LATERAL (
        SELECT id, name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults
        FROM courses
        WHERE owner_id=i.id
        ORDER BY created_at ASC
        LIMIT 1
      ) c ON true
      WHERE NOT EXISTS (SELECT 1 FROM instructor_settings s WHERE s.instructor_id=i.id);
    `);

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

    // Course scoping
    await query(`ALTER TABLE questions ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE questions ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Reusable datasets containing init SQL that can be referenced by questions
    await query(`CREATE TABLE IF NOT EXISTS datasets (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      name TEXT NOT NULL UNIQUE,
      sql TEXT NOT NULL DEFAULT '',
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
    );`);

    await query(`ALTER TABLE datasets ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE datasets ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    await query(`CREATE TABLE IF NOT EXISTS assignments (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title TEXT NOT NULL,
    difficulty TEXT NOT NULL,
    points INT NOT NULL DEFAULT 0,
    due TEXT NOT NULL,
    tags TEXT[] NOT NULL DEFAULT '{}',
    attempts_allowed INT NOT NULL DEFAULT 3 CHECK (attempts_allowed >= 1),
    published BOOLEAN NOT NULL DEFAULT false,
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);

    await query(`ALTER TABLE assignments ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE assignments ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Legacy column: course name is now stored on courses.name
    await query(`ALTER TABLE assignments DROP COLUMN IF EXISTS course;`).catch(() => { });

    await query(`ALTER TABLE assignments ADD COLUMN IF NOT EXISTS published BOOLEAN NOT NULL DEFAULT false;`);

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

    await query(`ALTER TABLE submissions ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE submissions ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Add stable foreign key to assignments and backfill
    await query(`ALTER TABLE submissions ADD COLUMN IF NOT EXISTS assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE;`);
    await query(`CREATE INDEX IF NOT EXISTS idx_submissions_assignment_id ON submissions(assignment_id);`);
    // Backfill assignment_id using title + owner_id match
    await query(`UPDATE submissions s
               SET assignment_id = a.id
               FROM assignments a
               WHERE s.assignment_id IS NULL
                 AND a.title = s.assignment
                 AND a.owner_id = s.owner_id;`);

    // If no rows remain with NULL assignment_id, enforce NOT NULL constraint
    try {
      const { rows: nullCheck } = await query<{ count: string }>(`SELECT COUNT(*)::text as count FROM submissions WHERE assignment_id IS NULL`);
      if (nullCheck[0] && nullCheck[0].count === '0') {
        await query(`ALTER TABLE submissions ALTER COLUMN assignment_id SET NOT NULL`);
      }
    } catch (e) {
      // ignore errors here to keep init idempotent
    }

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

    await query(`ALTER TABLE assignment_questions ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE assignment_questions ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

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

    await query(`ALTER TABLE question_submissions ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE question_submissions ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

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
  manual BOOLEAN NOT NULL DEFAULT false,
      attempt INT NOT NULL DEFAULT 1,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now()
    );`);
    // Backfill new columns for existing deployments (idempotent)
    await query(`ALTER TABLE question_submission_attempts ADD COLUMN IF NOT EXISTS manual BOOLEAN NOT NULL DEFAULT false;`);

    await query(`ALTER TABLE question_submission_attempts ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE question_submission_attempts ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Review requests (students can flag a graded question for re-evaluation)
    await query(`CREATE TABLE IF NOT EXISTS question_review_requests (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      assignment_id UUID REFERENCES assignments(id) ON DELETE CASCADE,
      question_id UUID REFERENCES questions(id) ON DELETE CASCADE,
      submission_id UUID REFERENCES submissions(id) ON DELETE CASCADE,
      student TEXT NOT NULL,
      comment TEXT NOT NULL DEFAULT '',
  status TEXT NOT NULL DEFAULT 'Pending', -- Pending | Resolved
  instructor_reply TEXT NULL,
  reply_at TIMESTAMPTZ NULL,
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      UNIQUE(assignment_id, question_id, submission_id, student, owner_id)
    );`);
    await query(`ALTER TABLE question_review_requests ADD COLUMN IF NOT EXISTS instructor_reply TEXT NULL;`);
    await query(`ALTER TABLE question_review_requests ADD COLUMN IF NOT EXISTS reply_at TIMESTAMPTZ NULL;`);

    await query(`ALTER TABLE question_review_requests ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE question_review_requests ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Threaded messaging for review requests
    await query(`CREATE TABLE IF NOT EXISTS question_review_request_messages (
      id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
      request_id UUID REFERENCES question_review_requests(id) ON DELETE CASCADE,
      sender_role TEXT NOT NULL, -- 'student' | 'instructor'
      sender TEXT NOT NULL, -- identifier (email/name)
      message TEXT NOT NULL,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
      owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE
  );`);
    await query(`CREATE INDEX IF NOT EXISTS idx_qrrm_request ON question_review_request_messages(request_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_qrrm_owner ON question_review_request_messages(owner_id);`);

    await query(`ALTER TABLE question_review_request_messages ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE question_review_request_messages ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

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

    await query(`ALTER TABLE assignment_feedback_survey ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE assignment_feedback_survey ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Instructor must be created via signup
    const defaultInstructorId: string | null = null;

    // Class roster per instructor
    await query(`CREATE TABLE IF NOT EXISTS roster (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    owner_id UUID REFERENCES instructors(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    email TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'Invited',
    evaluator BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE(owner_id, email)
  );`);
    // Backfill new evaluator column for existing deployments
    await query(`ALTER TABLE roster ADD COLUMN IF NOT EXISTS evaluator BOOLEAN NOT NULL DEFAULT false;`);

    await query(`ALTER TABLE roster ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE roster ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);
    // Allow same email in different courses
    await query(`ALTER TABLE roster DROP CONSTRAINT IF EXISTS roster_owner_id_email_key;`).catch(() => { });
    await query(`CREATE UNIQUE INDEX IF NOT EXISTS idx_roster_owner_course_email ON roster(owner_id, course_id, lower(email));`);

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

    await query(`ALTER TABLE invites ADD COLUMN IF NOT EXISTS course_id UUID NULL REFERENCES courses(id) ON DELETE CASCADE;`);
    await query(`ALTER TABLE invites ALTER COLUMN course_id SET DEFAULT current_setting('app.current_course', true)::uuid;`);

    // Student secrets: password per roster entry (scoped to instructor via owner_id on roster)
    await query(`CREATE TABLE IF NOT EXISTS student_secrets (
    roster_id UUID PRIMARY KEY REFERENCES roster(id) ON DELETE CASCADE,
    password TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
  );`);
    await query(`CREATE INDEX IF NOT EXISTS idx_roster_owner_email ON roster(owner_id, email);`);

    // Backfill course_id across existing rows (idempotent)
    await query(`UPDATE questions q SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE q.course_id IS NULL AND q.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE datasets d SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE d.course_id IS NULL AND d.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE assignments a SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE a.course_id IS NULL AND a.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE submissions sub SET course_id = a.course_id
      FROM assignments a
      WHERE sub.course_id IS NULL AND sub.assignment_id = a.id AND a.course_id IS NOT NULL;`);
    await query(`UPDATE submissions sub SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE sub.course_id IS NULL AND sub.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE assignment_questions aq SET course_id = a.course_id
      FROM assignments a
      WHERE aq.course_id IS NULL AND aq.assignment_id = a.id AND a.course_id IS NOT NULL;`);
    await query(`UPDATE question_submissions qs SET course_id = sub.course_id
      FROM submissions sub
      WHERE qs.course_id IS NULL AND qs.submission_id = sub.id AND sub.course_id IS NOT NULL;`);
    await query(`UPDATE question_submission_attempts qsa SET course_id = sub.course_id
      FROM submissions sub
      WHERE qsa.course_id IS NULL AND qsa.submission_id = sub.id AND sub.course_id IS NOT NULL;`);
    await query(`UPDATE roster r SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE r.course_id IS NULL AND r.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE invites i SET course_id = s.default_course_id
      FROM instructor_settings s
      WHERE i.course_id IS NULL AND i.owner_id = s.instructor_id AND s.default_course_id IS NOT NULL;`);
    await query(`UPDATE question_review_requests qrr SET course_id = a.course_id
      FROM assignments a
      WHERE qrr.course_id IS NULL AND qrr.assignment_id = a.id AND a.course_id IS NOT NULL;`);
    await query(`UPDATE question_review_request_messages m SET course_id = r.course_id
      FROM question_review_requests r
      WHERE m.course_id IS NULL AND m.request_id = r.id AND r.course_id IS NOT NULL;`);
    await query(`UPDATE assignment_feedback_survey s SET course_id = a.course_id
      FROM assignments a
      WHERE s.course_id IS NULL AND s.assignment_id = a.id AND a.course_id IS NOT NULL;`);


    // Indexes for tenant isolation performance
    await query(`CREATE INDEX IF NOT EXISTS idx_questions_owner ON questions(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_questions_course ON questions(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_assignments_owner ON assignments(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_assignments_course ON assignments(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_datasets_owner ON datasets(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_datasets_course ON datasets(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_submissions_owner ON submissions(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_submissions_course ON submissions(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_assignment_questions_owner ON assignment_questions(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_assignment_questions_course ON assignment_questions(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_question_submissions_owner ON question_submissions(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_question_submissions_course ON question_submissions(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_question_submission_attempts_owner ON question_submission_attempts(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_question_submission_attempts_course ON question_submission_attempts(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_roster_owner ON roster(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_roster_course ON roster(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_invites_owner ON invites(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_invites_course ON invites(course_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_invites_token ON invites(token);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_review_requests_owner ON question_review_requests(owner_id);`);
    await query(`CREATE INDEX IF NOT EXISTS idx_review_requests_course ON question_review_requests(course_id);`);

    // Enable Row Level Security and policies (idempotent)
    const instructorScopedTables = ['courses'];
    const courseScopedTables = [
      'questions',
      'datasets',
      'assignments',
      'submissions',
      'assignment_questions',
      'question_submissions',
      'question_submission_attempts',
      'roster',
      'invites',
      'question_review_requests',
      'question_review_request_messages',
      'assignment_feedback_survey',
    ];

    for (const t of instructorScopedTables) {
      await query(`ALTER TABLE ${t} ENABLE ROW LEVEL SECURITY;`).catch(() => { });
      await query(`DROP POLICY IF EXISTS ${t}_select_owner ON ${t};`).catch(() => { });
      await query(`CREATE POLICY ${t}_select_owner ON ${t} FOR SELECT USING (owner_id = current_setting('app.current_instructor', true)::uuid);`).catch(() => { });
      await query(`DROP POLICY IF EXISTS ${t}_mod_owner ON ${t};`).catch(() => { });
      await query(`CREATE POLICY ${t}_mod_owner ON ${t} FOR ALL USING (owner_id = current_setting('app.current_instructor', true)::uuid) WITH CHECK (owner_id = current_setting('app.current_instructor', true)::uuid);`).catch(() => { });
    }

    for (const t of courseScopedTables) {
      await query(`ALTER TABLE ${t} ENABLE ROW LEVEL SECURITY;`).catch(() => { });
      await query(`DROP POLICY IF EXISTS ${t}_select_owner ON ${t};`).catch(() => { });
      await query(`CREATE POLICY ${t}_select_owner ON ${t} FOR SELECT USING (
        owner_id = current_setting('app.current_instructor', true)::uuid
        AND course_id = current_setting('app.current_course', true)::uuid
      );`).catch(() => { });
      await query(`DROP POLICY IF EXISTS ${t}_mod_owner ON ${t};`).catch(() => { });
      await query(`CREATE POLICY ${t}_mod_owner ON ${t} FOR ALL USING (
        owner_id = current_setting('app.current_instructor', true)::uuid
        AND course_id = current_setting('app.current_course', true)::uuid
      ) WITH CHECK (
        owner_id = current_setting('app.current_instructor', true)::uuid
        AND course_id = current_setting('app.current_course', true)::uuid
      );`).catch(() => { });
    }

    // If no rows remain with NULL course_id, enforce NOT NULL constraints (best effort)
    try {
      const mustNotNull = ['questions', 'datasets', 'assignments', 'submissions', 'assignment_questions', 'question_submissions', 'question_submission_attempts', 'roster', 'invites', 'question_review_requests', 'question_review_request_messages', 'assignment_feedback_survey'];
      for (const t of mustNotNull) {
        const { rows: nullCheck } = await query<{ count: string }>(`SELECT COUNT(*)::text as count FROM ${t} WHERE course_id IS NULL`);
        if (nullCheck[0] && nullCheck[0].count === '0') {
          await query(`ALTER TABLE ${t} ALTER COLUMN course_id SET NOT NULL`).catch(() => { });
        }
      }
    } catch {
      // ignore
    }

    // Seed demo data once (simple check by counting assignments & submissions)
    const { rows: aCount } = await query<{ count: string }>(`SELECT COUNT(*)::text as count FROM assignments;`);
    if (Number(aCount[0].count) === 0) {
      if (defaultInstructorId) {
        await query(`INSERT INTO assignments (title, difficulty, points, due, tags, owner_id) VALUES
        ('Basic SELECT & WHERE', 'Beginner', 10, '2025-09-05', ARRAY['SELECT','WHERE','FILTERS'], $1),
        ('JOINs & Aggregates', 'Intermediate', 20, '2025-09-12', ARRAY['JOIN','GROUP BY','HAVING'], $1);`, [defaultInstructorId]);
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
  } finally {
    // Always release the advisory lock
    await query(`SELECT pg_advisory_unlock(12345, hashtext('sqlgrader.init.v1'))`).catch(() => { });
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
