import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withCourseContext, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
// Ensure DB schema is initialized once
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/assignment-questions?assignmentId=... -> list linked questions (ordered)
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        const { searchParams } = new URL(req.url);
        const assignmentId = searchParams.get('assignmentId');
        if (!assignmentId) return NextResponse.json({ error: 'assignmentId required' }, { status: 400 });
        const { rows } = await run(() => query(`SELECT aq.assignment_id as "assignmentId", aq.question_id as "questionId", aq.position, aq.points_override as "pointsOverride",
             q.title, q.difficulty, q.status, q.attempts, q.max_points as "maxPoints"
             FROM assignment_questions aq
             JOIN questions q ON q.id = aq.question_id
             WHERE aq.assignment_id = $1 AND aq.owner_id = current_setting('app.current_instructor')::uuid
             ORDER BY aq.position, q.created_at DESC`, [assignmentId]));
        return NextResponse.json(rows);
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// POST body: { assignmentId, questionId, position?, pointsOverride? }
// Add or update link between assignment & question
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        const body = await req.json();
        const { assignmentId, questionId, position, pointsOverride } = body || {};
        if (!assignmentId || !questionId) return NextResponse.json({ error: 'assignmentId & questionId required' }, { status: 400 });
        await run(() => query(`INSERT INTO assignment_questions (assignment_id, question_id, position, points_override, owner_id)
            VALUES ($1,$2,COALESCE($3,0),$4,current_setting('app.current_instructor')::uuid)
            ON CONFLICT (assignment_id, question_id)
            DO UPDATE SET position = EXCLUDED.position, points_override = EXCLUDED.points_override;`, [assignmentId, questionId, position ?? 0, pointsOverride ?? null]));
        const { rows } = await run(() => query(`SELECT aq.assignment_id as "assignmentId", aq.question_id as "questionId", aq.position, aq.points_override as "pointsOverride",
             q.title, q.difficulty, q.status, q.attempts, q.max_points as "maxPoints"
             FROM assignment_questions aq JOIN questions q ON q.id = aq.question_id
             WHERE aq.assignment_id = $1 AND aq.question_id = $2 AND aq.owner_id = current_setting('app.current_instructor')::uuid`, [assignmentId, questionId]));
        return NextResponse.json(rows[0]);
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// DELETE /api/assignment-questions?assignmentId=...&questionId=...
export async function DELETE(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        const { searchParams } = new URL(req.url);
        const assignmentId = searchParams.get('assignmentId');
        const questionId = searchParams.get('questionId');
        if (!assignmentId || !questionId) return NextResponse.json({ error: 'assignmentId & questionId required' }, { status: 400 });
        await run(() => query(`DELETE FROM assignment_questions WHERE assignment_id=$1 AND question_id=$2 AND owner_id = current_setting('app.current_instructor')::uuid`, [assignmentId, questionId]));
        return NextResponse.json({ ok: true });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// PATCH for reordering multiple
export async function PATCH(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        const body = await req.json();
        const { assignmentId, order } = body || {};
        if (!assignmentId || !Array.isArray(order)) return NextResponse.json({ error: 'assignmentId & order[] required' }, { status: 400 });
        await run(async () => {
            for (const item of order) {
                if (!item?.questionId || typeof item.position !== 'number') continue;
                await query(`UPDATE assignment_questions SET position=$1 WHERE assignment_id=$2 AND question_id=$3 AND owner_id = current_setting('app.current_instructor')::uuid`, [item.position, assignmentId, item.questionId]);
            }
        });
        return NextResponse.json({ ok: true });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
