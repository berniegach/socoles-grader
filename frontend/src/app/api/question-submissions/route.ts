import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withCourseContext, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false; async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/question-submissions?submissionId=... or ?assignmentId=...&student=... or ?historyOf=questionSubmissionId
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const submissionId = searchParams.get('submissionId');
        const assignmentId = searchParams.get('assignmentId');
        const student = searchParams.get('student');
        const historyOf = searchParams.get('historyOf');
        const allStudents = searchParams.get('allStudents');
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        return await run(async () => {
            if (historyOf) {
                const { rows } = await query(`
                    SELECT id,
                                 question_submission_id as "questionSubmissionId",
                                 submission_id as "submissionId",
                                 assignment_id as "assignmentId",
                                 question_id as "questionId",
                                 student,
                                 sql,
                                 grade::float8 as grade,
                                 status,
                                 rubric,
                                 feedback,
                                 ROW_NUMBER() OVER (ORDER BY created_at ASC, id ASC) AS attempt,
                                 to_char(created_at, 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt"
                    FROM question_submission_attempts
                    WHERE question_submission_id=$1 AND owner_id = current_setting('app.current_instructor')::uuid
                    ORDER BY created_at ASC, id ASC`, [historyOf]);
                return NextResponse.json(rows);
            }
            if (submissionId) {
                const { rows } = await query(`SELECT qs.id, qs.submission_id as "submissionId", qs.assignment_id as "assignmentId", qs.question_id as "questionId", qs.student, qs.sql, qs.grade::float8 as grade, qs.status, qs.rubric, qs.feedback, qs.attempt, q.title
                    FROM question_submissions qs JOIN questions q ON q.id = qs.question_id
                    WHERE qs.submission_id=$1 AND qs.owner_id = current_setting('app.current_instructor')::uuid ORDER BY q.title`, [submissionId]);
                return NextResponse.json(rows);
            }
            if (assignmentId && student) {
                const { rows } = await query(`SELECT qs.id, qs.submission_id as "submissionId", qs.assignment_id as "assignmentId", qs.question_id as "questionId", qs.student, qs.sql, qs.grade::float8 as grade, qs.status, qs.rubric, qs.feedback, qs.attempt, q.title
                    FROM question_submissions qs JOIN questions q ON q.id = qs.question_id
                    WHERE qs.assignment_id=$1 AND qs.student=$2 AND qs.owner_id = current_setting('app.current_instructor')::uuid ORDER BY q.title`, [assignmentId, student]);
                return NextResponse.json(rows);
            }
            if (assignmentId && allStudents === '1') {
                const { rows } = await query(`SELECT qs.id, qs.submission_id as "submissionId", qs.assignment_id as "assignmentId", qs.question_id as "questionId", qs.student, qs.sql, qs.grade::float8 as grade, qs.status, qs.rubric, qs.feedback, qs.attempt, q.title
                    FROM question_submissions qs JOIN questions q ON q.id = qs.question_id
                    WHERE qs.assignment_id=$1 AND qs.owner_id = current_setting('app.current_instructor')::uuid
                    ORDER BY q.title, qs.student, qs.created_at ASC`, [assignmentId]);
                return NextResponse.json(rows);
            }
            return NextResponse.json({ error: 'submissionId or (assignmentId & student) required' }, { status: 400 });
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// Helper: compute the next attempt number for a given question_submission id
async function getNextAttempt(questionSubmissionId: string): Promise<number> {
    const { rows } = await query(`SELECT COALESCE(MAX(attempt), 0) + 1 AS next FROM question_submission_attempts WHERE question_submission_id=$1`, [questionSubmissionId]);
    const next = rows?.[0]?.next;
    return typeof next === 'number' ? next : 1;
}

// POST body: { submissionId, assignmentId, questionId, student, sql, grade?, status?, rubric?, feedback?, incrementAttempt?, noAttemptIncrement? }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { submissionId, assignmentId, questionId, student, sql, grade, status, rubric, feedback, incrementAttempt, noAttemptIncrement } = body || {};
        if (!submissionId || !assignmentId || !questionId) return NextResponse.json({ error: 'submissionId, assignmentId, questionId required' }, { status: 400 });
        // Only increment attempts when explicitly requested by the caller.
        // Back-compat: treat noAttemptIncrement === false as a signal to increment.
        const shouldIncrement = incrementAttempt === true || noAttemptIncrement === false;
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        return await run(async () => {
            const effectiveStudent = payload.role === 'student' ? payload.email : (student || '');
            if (!effectiveStudent) return NextResponse.json({ error: 'student required' }, { status: 400 });
            const { rows } = await query(`INSERT INTO question_submissions (submission_id, assignment_id, question_id, student, sql, grade, status, rubric, feedback, attempt, owner_id)
                VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,COALESCE((SELECT attempt FROM question_submissions WHERE submission_id=$1 AND question_id=$3 LIMIT 1), 0), current_setting('app.current_instructor')::uuid)
                ON CONFLICT (submission_id, question_id)
                DO UPDATE SET sql=EXCLUDED.sql, grade=EXCLUDED.grade, status=EXCLUDED.status, rubric=EXCLUDED.rubric, feedback=EXCLUDED.feedback
                RETURNING id, submission_id as "submissionId", assignment_id as "assignmentId", question_id as "questionId", student, sql, grade::float8 as grade, status, rubric, feedback, attempt`, [submissionId, assignmentId, questionId, effectiveStudent, sql || '', grade ?? null, status || 'Pending', rubric || null, Array.isArray(feedback) ? feedback : []]);
            let qs = rows[0];
            if (shouldIncrement) {
                const nextAttempt = await getNextAttempt(qs.id);
                await query(`INSERT INTO question_submission_attempts (question_submission_id, submission_id, assignment_id, question_id, student, sql, grade, status, rubric, feedback, manual, attempt, owner_id)
                    VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,($8='Manual'),$11,current_setting('app.current_instructor')::uuid)`, [qs.id, submissionId, assignmentId, questionId, effectiveStudent, sql || '', grade ?? null, status || 'Auto-graded', rubric || null, Array.isArray(feedback) ? feedback : [], nextAttempt]);
                const upd = await query(`UPDATE question_submissions SET attempt=$2 WHERE id=$1 RETURNING id, submission_id as "submissionId", assignment_id as "assignmentId", question_id as "QuestionId", student, sql, grade::float8 as grade, status, rubric, feedback, attempt`, [qs.id, nextAttempt]);
                qs = upd.rows[0] || qs;
            }
            return NextResponse.json(qs);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

export async function PATCH(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { id, grade, rubric, feedback, status, incrementAttempt, noAttemptIncrement } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        return await run(async () => {
            const { rows } = await query(`UPDATE question_submissions SET grade=$2, rubric=$3, feedback=$4, status=$5 WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid RETURNING id, submission_id as "submissionId", assignment_id as "assignmentId", question_id as "questionId", student, sql, grade::float8 as grade, status, rubric, feedback, attempt`, [id, grade ?? null, rubric || null, Array.isArray(feedback) ? feedback : [], status || 'Pending']);
            if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
            let qs = rows[0];
            const shouldIncrement = incrementAttempt === true || noAttemptIncrement === false;
            if (shouldIncrement) {
                const nextAttempt = await getNextAttempt(id);
                const effectiveStudent = payload.role === 'student' ? payload.email : null;
                await query(`INSERT INTO question_submission_attempts (question_submission_id, submission_id, assignment_id, question_id, student, sql, grade, status, rubric, feedback, manual, attempt, owner_id)
                    SELECT $1, submission_id, assignment_id, question_id, COALESCE($7, student), sql, $2, $3, $4, $5, ($3='Manual'), $6, current_setting('app.current_instructor')::uuid
                    FROM question_submissions WHERE id=$1`, [id, grade ?? null, status || 'Auto-graded', rubric || null, Array.isArray(feedback) ? feedback : [], nextAttempt, effectiveStudent]);
                const upd = await query(`UPDATE question_submissions SET attempt=$2, student=COALESCE($3, student) WHERE id=$1 RETURNING id, submission_id as "submissionId", assignment_id as "assignmentId", question_id as "questionId", student, sql, grade::float8 as grade, status, rubric, feedback, attempt`, [id, nextAttempt, effectiveStudent]);
                qs = upd.rows[0] || qs;
            }
            return NextResponse.json(qs);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
