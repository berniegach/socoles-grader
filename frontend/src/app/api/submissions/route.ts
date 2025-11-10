import { NextResponse, NextRequest } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import type { Submission } from '@/lib/types';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const scope = (searchParams.get('scope') || '').toLowerCase();
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        // If student is a TA (evaluator), allow listing all submissions; else restrict to own by email OR full name
        let studentParams: string[] | undefined = undefined;
        if (payload.role === 'student') {
            const meId = payload.sub as string;
            const me = await withInstructorContext(instructorId, () => query<{ evaluator: boolean }>(`SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`, [meId]));
            const isTA = !!me.rows[0]?.evaluator;
            if (scope === 'mine' || !isTA) {
                const ids = [payload.email, payload.name].filter((v, i, a) => !!v && a.indexOf(v) === i) as string[];
                studentParams = ids.length ? ids : [payload.email];
            }
        }
        const filterClause = studentParams?.length === 1
            ? 'AND student = $1'
            : studentParams?.length === 2
                ? 'AND (student = $1 OR student = $2)'
                : '';
        const { rows } = await withInstructorContext(instructorId, () => query(`
            SELECT id, student, assignment_id as "assignmentId", date, grade::float8 as grade, status
                        FROM submissions
                        WHERE owner_id = current_setting('app.current_instructor')::uuid
                            ${filterClause}
                        ORDER BY created_at DESC
                        LIMIT 1000`, studentParams as any[] | undefined));
        //console.log("Returned rows:", rows);
        return NextResponse.json(rows);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function POST(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body: Partial<Submission> & { date?: string; assignment?: string } = await req.json();
        const { student, assignment: assignmentTitle, date, grade, status } = body as any;
        if (!assignmentTitle) return NextResponse.json({ error: 'assignment required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        // Resolve assignment_id by title for this owner; insert both assignment (title) and assignment_id
        const { rows: aRows } = await withInstructorContext(instructorId, () => query<{ id: string }>(
            `SELECT id FROM assignments WHERE title=$1 AND owner_id = current_setting('app.current_instructor')::uuid ORDER BY created_at DESC LIMIT 1`, [assignmentTitle]
        ));
        const assignmentId = aRows[0]?.id || null;
        // Canonicalize student identity: for student role, always use email from JWT; otherwise use provided value
        const effectiveStudent = payload.role === 'student' ? payload.email : (student || '');
        if (!effectiveStudent) return NextResponse.json({ error: 'student required' }, { status: 400 });
        const { rows } = await withInstructorContext(instructorId, () => query<Submission>(
            `INSERT INTO submissions (student, assignment, assignment_id, date, grade, status, owner_id)
             VALUES ($1,$2,$3,$4,$5,$6,current_setting('app.current_instructor')::uuid)
             RETURNING id, student, assignment_id as "assignmentId", date, grade::float8 as grade, status`,
            [effectiveStudent, assignmentTitle, assignmentId, date || new Date().toISOString(), grade ?? 0, status || 'Pending']
        ));
        return NextResponse.json(rows[0]);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

// PATCH /api/submissions  body: { id, grade?, status? }
export async function PATCH(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body: Partial<Submission> & { id?: string } = await req.json();
        const { id, grade, status } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const { rows } = await withInstructorContext(instructorId, () => query<Submission>(`UPDATE submissions SET grade=COALESCE($2, grade), status=COALESCE($3,status) WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid RETURNING id, student, assignment_id as "assignmentId", date, grade::float8 as grade, status`, [id, grade, status]));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

// DELETE /api/submissions?id=SUBMISSION_ID  (allowed only when status is In Progress)
export async function DELETE(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const id = searchParams.get('id');
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const { rows: existing } = await withInstructorContext(instructorId, () => query<{ id: string; status: string }>(`SELECT id, status FROM submissions WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [id]));
        if (!existing.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        if (existing[0].status !== 'In Progress') return NextResponse.json({ error: 'Only in-progress submissions can be deleted.' }, { status: 400 });
        await withInstructorContext(instructorId, () => query(`DELETE FROM submissions WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [id]));
        return NextResponse.json({ ok: true });
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
