import { withCourseContext, withInstructorContext, query } from '@/lib/db';

export interface ImprovementInput {
  instructorId: string;
  courseId?: string;
  assignmentId: string;
  studentName?: string;
  studentEmail?: string;
}

export interface ImprovementStats {
  firstScore: number | null;
  finalScore: number | null;
  attemptCount: number;
  improvement: number | null;
}

export async function computeAssignmentImprovement({ instructorId, courseId, assignmentId, studentName = '', studentEmail = '' }: ImprovementInput): Promise<ImprovementStats> {
  const run = <T>(fn: () => Promise<T>) => courseId
    ? withCourseContext(instructorId, courseId, fn)
    : withInstructorContext(instructorId, fn);
  // Resolve latest submission for this assignment+student (name or email)
  const { rows: subRows } = await run(() => query<{ id: string; grade: number | null }>(
    `SELECT id, grade::float8 as grade
       FROM submissions s
      WHERE s.owner_id = current_setting('app.current_instructor')::uuid
        AND s.assignment_id = $1
        AND (s.student = $2 OR s.student = $3)
      ORDER BY s.created_at DESC
      LIMIT 1`,
    [assignmentId, studentName, studentEmail]
  ));
  const submissionId = subRows[0]?.id;
  const submissionOverall = (subRows[0]?.grade ?? null) as number | null;

  let firstScore: number | null = null;
  let finalScore: number | null = null;
  let attemptCount = 0;
  let improvement: number | null = null;

  if (submissionId) {
    const { rows: firstRows } = await run(() => query<{ question_id: string; grade: number }>(
      `SELECT DISTINCT ON (question_id) question_id, grade::float8 as grade
         FROM question_submission_attempts
        WHERE owner_id = current_setting('app.current_instructor')::uuid
          AND submission_id = $1
          AND grade IS NOT NULL
        ORDER BY question_id, attempt ASC, created_at ASC`,
      [submissionId]
    ));
    const { rows: finalRows } = await run(() => query<{ question_id: string; grade: number }>(
      `SELECT DISTINCT ON (question_id) question_id, grade::float8 as grade
         FROM question_submission_attempts
        WHERE owner_id = current_setting('app.current_instructor')::uuid
          AND submission_id = $1
          AND grade IS NOT NULL
        ORDER BY question_id, attempt DESC, created_at DESC`,
      [submissionId]
    ));
    const { rows: cntRows } = await run(() => query<{ cnt: number }>(
      `SELECT COUNT(*)::int as cnt
         FROM question_submission_attempts
        WHERE owner_id = current_setting('app.current_instructor')::uuid
          AND submission_id = $1`,
      [submissionId]
    ));
    attemptCount = Number(cntRows[0]?.cnt || 0);
    const avg = (arr: number[]) => (arr.length ? arr.reduce((a, b) => a + b, 0) / arr.length : null);
    firstScore = avg(firstRows.map(r => Number(r.grade)).filter(Number.isFinite));
    finalScore = avg(finalRows.map(r => Number(r.grade)).filter(Number.isFinite));
    if (finalScore == null && typeof submissionOverall === 'number') finalScore = submissionOverall;
    if (firstScore != null && finalScore != null) improvement = finalScore - firstScore;
  }

  return { firstScore, finalScore, attemptCount, improvement };
}

export interface EligibilityResult {
  perfectOnFirstTry: boolean;
}

export async function computeSurveyEligibility({ instructorId, courseId, assignmentId, studentName = '', studentEmail = '' }: ImprovementInput): Promise<EligibilityResult> {
  const run = <T>(fn: () => Promise<T>) => courseId
    ? withCourseContext(instructorId, courseId, fn)
    : withInstructorContext(instructorId, fn);
  const { rows: subRows } = await run(() => query<{ id: string }>(
    `SELECT id
       FROM submissions s
      WHERE s.owner_id = current_setting('app.current_instructor')::uuid
        AND s.assignment_id = $1
        AND (s.student = $2 OR s.student = $3)
      ORDER BY s.created_at DESC
      LIMIT 1`,
    [assignmentId, studentName, studentEmail]
  ));
  const submissionId = subRows[0]?.id;
  if (!submissionId) return { perfectOnFirstTry: false };

  const { rows } = await run(() => query<{ question_id: string; min_attempt: number; max_attempt: number; first_grade: number | null; max_grade: number | null }>(
    `SELECT question_id,
            MIN(attempt) as min_attempt,
            MAX(attempt) as max_attempt,
            MAX(CASE WHEN attempt = 1 THEN grade::float8 END) as first_grade,
            MAX(grade::float8) as max_grade
       FROM question_submission_attempts
      WHERE owner_id = current_setting('app.current_instructor')::uuid
        AND submission_id = $1
        AND grade IS NOT NULL
      GROUP BY question_id`,
    [submissionId]
  ));
  if (!rows.length) return { perfectOnFirstTry: false };
  const eps = 1e-6;
  // Decide scale based on any max_grade observed
  const anyMax = rows.reduce((m, r) => Math.max(m, Number(r.max_grade || 0)), 0);
  const perfect = anyMax > 1 + eps ? 10 : 1; // 10-point vs 1.0 scale
  const perfectOnFirstTry = rows.every(r => {
    const first = Number(r.first_grade ?? -1);
    const maxAtt = Number(r.max_attempt || 0);
    const isPerfect = Math.abs(first - perfect) < (perfect === 10 ? 0.005 : 0.0005);
    return maxAtt === 1 && isPerfect;
  });
  return { perfectOnFirstTry };
}
