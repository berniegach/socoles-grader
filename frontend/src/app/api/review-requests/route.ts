import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false; async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/review-requests?assignmentId=...&questionId=...&submissionId=...&student=...
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const assignmentId = searchParams.get('assignmentId');
        const questionId = searchParams.get('questionId');
        const submissionId = searchParams.get('submissionId');
        const student = searchParams.get('student');
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            // Only allow Teaching Assistants (evaluator) to list review requests when role is student
            if (payload.role === 'student') {
                const meId = payload.sub as string;
                const { rows: me } = await query<{ evaluator: boolean }>(
                    `SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`,
                    [meId]
                );
                if (!me.length || !me[0].evaluator) return NextResponse.json({ error: 'forbidden' }, { status: 403 });
            }
            const clauses: string[] = [];
            const params: any[] = [];
            function add(c: string, v: any) { params.push(v); clauses.push(`${c}=$${params.length}`); }
            if (assignmentId) add('assignment_id', assignmentId);
            if (questionId) add('question_id', questionId);
            if (submissionId) add('submission_id', submissionId);
            if (student) add('student', student);
            const where = clauses.length ? 'WHERE ' + clauses.join(' AND ') + ' AND owner_id = current_setting(\'app.current_instructor\')::uuid' : 'WHERE owner_id = current_setting(\'app.current_instructor\')::uuid';
            const { rows } = await query(`SELECT id, assignment_id as "assignmentId", question_id as "questionId", submission_id as "submissionId", student, comment, status, instructor_reply as "instructorReply", to_char(reply_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "replyAt", to_char(created_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt", to_char(updated_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "updatedAt" FROM question_review_requests ${where} ORDER BY created_at DESC`, params);
            return NextResponse.json(rows);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// POST body { assignmentId, questionId, submissionId, student, comment }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { assignmentId, questionId, submissionId, student, comment } = body || {};
        if (!assignmentId || !questionId || !submissionId || !student || !comment) return NextResponse.json({ error: 'assignmentId, questionId, submissionId, student, comment required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            const { rows } = await query(`INSERT INTO question_review_requests (assignment_id, question_id, submission_id, student, comment, owner_id)
        VALUES ($1,$2,$3,$4,$5,current_setting('app.current_instructor')::uuid)
        ON CONFLICT (assignment_id, question_id, submission_id, student, owner_id)
        DO UPDATE SET comment=EXCLUDED.comment, status='Pending', updated_at=now(), instructor_reply=NULL, reply_at=NULL
        RETURNING id, assignment_id as "assignmentId", question_id as "questionId", submission_id as "submissionId", student, comment, status, instructor_reply as "instructorReply", to_char(reply_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "replyAt", to_char(created_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt", to_char(updated_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "updatedAt"`, [assignmentId, questionId, submissionId, student, String(comment).slice(0, 2000)]);
            const reqRow = rows[0];
            if (reqRow) {
                // Insert initial message (student)
                await query(`INSERT INTO question_review_request_messages (request_id, sender_role, sender, message, owner_id)
                    VALUES ($1,'student',$2,$3,current_setting('app.current_instructor')::uuid)`, [reqRow.id, student, String(comment).slice(0, 4000)]);
            }
            return NextResponse.json(reqRow);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// PATCH body { id, status, comment? }
export async function PATCH(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { id, status, comment, instructorReply } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            // Only instructor or TA can update request status/comments
            if (payload.role === 'student') {
                const meId = payload.sub as string;
                const { rows: me } = await query<{ evaluator: boolean }>(
                    `SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`,
                    [meId]
                );
                if (!me.length || !me[0].evaluator) return NextResponse.json({ error: 'forbidden' }, { status: 403 });
            }
            const { rows } = await query(`UPDATE question_review_requests SET 
                status=COALESCE($2,status), 
                comment=COALESCE($3, comment), 
                instructor_reply=COALESCE($4, instructor_reply), 
                reply_at=CASE WHEN $4 IS NOT NULL THEN now() ELSE reply_at END,
                updated_at=now() 
                WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid 
                RETURNING id, assignment_id as "assignmentId", question_id as "questionId", submission_id as "submissionId", student, comment, status, instructor_reply as "instructorReply", to_char(reply_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "replyAt", to_char(created_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt", to_char(updated_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "updatedAt"`, [id, status || null, comment != null ? String(comment).slice(0, 2000) : null, instructorReply != null ? String(instructorReply).slice(0, 4000) : null]);
            if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
            return NextResponse.json(rows[0]);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
