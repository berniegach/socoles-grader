import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withCourseContext, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/questions/:id  -> detailed question for student view
export async function GET(req: NextRequest, { params }: { params: Promise<{ id: string }> }) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        // IMPORTANT: When role=student, run queries under the owning instructor context
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const courseId = payload.role === 'student' ? (payload.courseId as string | undefined) : undefined;
        const run = <T>(fn: () => Promise<T>) => courseId
            ? withCourseContext(instructorId, courseId, fn)
            : withInstructorContext(instructorId, fn);
        const { id } = await params;
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const { rows } = await run(() => query(`SELECT q.id, q.title, q.difficulty, q.status,
                (SELECT COUNT(DISTINCT qs.student)::int FROM question_submissions qs WHERE qs.question_id = q.id AND qs.owner_id = current_setting('app.current_instructor')::uuid AND qs.grade IS NOT NULL AND ((q.max_points > 0 AND qs.grade >= q.max_points * 0.999) OR qs.status='Auto-graded')) AS attempts,
                q.max_points as "maxPoints", q.dataset, q.prompt, q.model_sql as "modelSql", q.hints, q.model_queries as "modelQueries", q.init_sql as "initSql",
                q.use_default_grading as "useDefaultGrading", q.grading_options as "gradingOptions"
            FROM questions q WHERE q.id=$1 AND q.owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`, [id]));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e: unknown) {
        const msg = e instanceof Error ? e.message : 'failed';
        return NextResponse.json({ error: msg }, { status: 500 });
    }
}
