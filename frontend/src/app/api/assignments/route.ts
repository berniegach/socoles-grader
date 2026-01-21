import { NextResponse } from 'next/server';
import { initSchema, withCourseContext, withInstructorContext, query } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';
import type { Assignment, AssignmentWithQuestions, AssignmentPatch, NewAssignmentPayload } from '@/lib/types';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function GET(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const includeQuestions = new URL(req.url).searchParams.get('include') === 'questions';
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const withContext = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        let rosterActive = true;
        if (payload.role === 'student') {
            // Validate that roster row still exists and is Active; deny access otherwise
            // NOTE: For student tokens we encode the roster id in `sub` (see signStudentJwt).
            const rosterId = payload.sub; // roster id
            if (!rosterId) return NextResponse.json({ error: 'invalid student token' }, { status: 401 });
            const { rows: rosterRows } = await withContext(() => query<{ status: string }>(`SELECT status FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`, [rosterId]));
            if (!rosterRows.length || rosterRows[0].status !== 'Active') {
                rosterActive = false;
            }
        }
        if (!rosterActive) return NextResponse.json({ error: 'access revoked' }, { status: 403 });
        const baseSql = `SELECT a.id, a.title, c.name as course, a.difficulty, a.points, a.due, a.tags, a.attempts_allowed as "attemptsAllowed", a.published
                        FROM assignments a
                        JOIN courses c ON c.id = a.course_id
                        WHERE a.owner_id = current_setting('app.current_instructor')::uuid
                            AND a.course_id = current_setting('app.current_course', true)::uuid`;
        const sql = payload.role === 'student'
            ? `${baseSql} AND a.published = true ORDER BY a.created_at DESC LIMIT 500`
            : `${baseSql} ORDER BY a.created_at DESC LIMIT 500`;
        const { rows: assignments } = await withContext(() =>
            query<Assignment>(sql)
        );
        if (!includeQuestions) return NextResponse.json(assignments);

        // fetch question links for these assignments
        const ids = assignments.map(a => a.id);
        if (!ids.length) return NextResponse.json([]);
        interface LinkRow { assignmentId: string; questionId: string; position: number; pointsOverride: number | null; title: string; difficulty: string; status: string; attempts: number; maxPoints: number }
        const { rows: links } = await withContext(() => query<LinkRow>(`SELECT aq.assignment_id as "assignmentId", aq.question_id as "questionId", aq.position, aq.points_override as "pointsOverride",
            q.title, q.difficulty, q.status, q.attempts, q.max_points as "maxPoints"
            FROM assignment_questions aq JOIN questions q ON q.id = aq.question_id
                        WHERE aq.assignment_id = ANY($1)
                            AND aq.owner_id = current_setting('app.current_instructor')::uuid
                            AND aq.course_id = current_setting('app.current_course', true)::uuid
                            AND q.course_id = current_setting('app.current_course', true)::uuid
            ORDER BY aq.position ASC, q.created_at DESC`, [ids]));
        const byAssign: Record<string, LinkRow[]> = {};
        for (const l of links) { (byAssign[l.assignmentId] ||= []).push(l); }
        const enriched: AssignmentWithQuestions[] = assignments.map(a => ({
            ...a, questions: (byAssign[a.id] || []).map(q => ({
                id: q.questionId,
                title: q.title,
                difficulty: q.difficulty,
                status: q.status,
                attempts: q.attempts,
                maxPoints: q.maxPoints,
                position: q.position,
                pointsOverride: q.pointsOverride,
            }))
        }));
        return NextResponse.json(enriched);
    } catch (e) {
        console.error('[assignments.GET] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

// Create new assignment
export async function POST(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) { console.error('[assignments.POST] missing token'); return NextResponse.json({ error: 'unauthorized' }, { status: 401 }); }
        const payload = verifyJwt(token);
        if (!payload) { console.error('[assignments.POST] invalid token'); return NextResponse.json({ error: 'invalid token' }, { status: 401 }); }
        const body: NewAssignmentPayload & { attemptsAllowed?: number } = await req.json();
        const { title, difficulty, points, due, tags, attemptsAllowed, published } = body;
        if (!title) return NextResponse.json({ error: 'Title required' }, { status: 400 });
        const minAttempts = (attemptsAllowed === undefined || attemptsAllowed === null)
            ? 3
            : Math.max(1, Math.floor(Number(attemptsAllowed)));
        const { rows } = await withInstructorContext(payload.sub, () => query<Assignment>(
            `WITH ins AS (
                INSERT INTO assignments (title, difficulty, points, due, tags, attempts_allowed, published, owner_id)
                VALUES ($1,$2,$3,$4,$5,$6,$7,current_setting('app.current_instructor')::uuid)
                RETURNING *
             )
             SELECT ins.id, ins.title, c.name as course, ins.difficulty, ins.points, ins.due, ins.tags,
                    ins.attempts_allowed as "attemptsAllowed", ins.published
               FROM ins
               JOIN courses c ON c.id = ins.course_id`,
            [title, difficulty || 'Beginner', points || 0, due || '', Array.isArray(tags) ? tags : [], minAttempts, published === true]
        ));
        return NextResponse.json(rows[0]);
    } catch (e) {
        console.error('[assignments.POST] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function PATCH(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) { console.error('[assignments.PATCH] missing token'); return NextResponse.json({ error: 'unauthorized' }, { status: 401 }); }
        const payload = verifyJwt(token);
        if (!payload) { console.error('[assignments.PATCH] invalid token'); return NextResponse.json({ error: 'invalid token' }, { status: 401 }); }
        const body: Partial<AssignmentPatch> = await req.json();
        const { id, title, difficulty, points, due, tags, attemptsAllowed, published } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const nextTitle = title === undefined ? null : title;
        const nextDifficulty = difficulty === undefined ? null : difficulty;
        const nextPoints = points === undefined || points === null ? null : Number(points);
        const nextDue = due === undefined ? null : due;
        const nextTags = tags === undefined ? null : (Array.isArray(tags) ? tags : []);
        const nextAttemptsAllowed = (attemptsAllowed === undefined || attemptsAllowed === null)
            ? null
            : Math.max(1, Math.floor(Number(attemptsAllowed)));
        const nextPublished = published === undefined ? null : !!published;
        const { rows } = await withInstructorContext(payload.sub, () => query<Assignment>(
            `WITH upd AS (
                UPDATE assignments SET
                    title=COALESCE($2, title),
                    difficulty=COALESCE($3, difficulty),
                    points=COALESCE($4, points),
                    due=COALESCE($5, due),
                    tags=COALESCE($6, tags),
                    attempts_allowed=COALESCE($7, attempts_allowed),
                    published=COALESCE($8, published)
                WHERE id=$1
                  AND owner_id = current_setting('app.current_instructor')::uuid
                  AND course_id = current_setting('app.current_course', true)::uuid
                RETURNING *
             )
             SELECT upd.id, upd.title, c.name as course, upd.difficulty, upd.points, upd.due, upd.tags,
                    upd.attempts_allowed as "attemptsAllowed", upd.published
               FROM upd
               JOIN courses c ON c.id = upd.course_id`,
            [id, nextTitle, nextDifficulty, nextPoints, nextDue, nextTags, nextAttemptsAllowed, nextPublished]
        ));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e) {
        console.error('[assignments.PATCH] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function DELETE(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) { console.error('[assignments.DELETE] missing token'); return NextResponse.json({ error: 'unauthorized' }, { status: 401 }); }
        const payload = verifyJwt(token);
        if (!payload) { console.error('[assignments.DELETE] invalid token'); return NextResponse.json({ error: 'invalid token' }, { status: 401 }); }
        const url = new URL(req.url);
        let id = url.searchParams.get('id');
        if (!id) {
            try { const body = await req.json(); id = body?.id; } catch { /* ignore body parse */ }
        }
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        await withInstructorContext(payload.sub, () => query(`DELETE FROM assignments WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid AND course_id = current_setting('app.current_course', true)::uuid`, [id]));
        return NextResponse.json({ ok: true });
    } catch (e) {
        console.error('[assignments.DELETE] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
