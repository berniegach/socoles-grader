import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false; async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/review-request-messages?requestId=...
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const requestId = searchParams.get('requestId');
        if (!requestId) return NextResponse.json({ error: 'requestId required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            // Authorize: ensure request belongs to instructor and if student ensure they are owner of request,
            // unless the student is an evaluator (TA) for this instructor, in which case allow access.
            // NOTE: Older records may have stored either roster id (payload.sub) OR display name (payload.name) OR email as 'student'. Accept any.
            const { rows: reqRows } = await query(`SELECT id, student FROM question_review_requests WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [requestId]);
            if (!reqRows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
            if (payload.role === 'student') {
                // Is this student an evaluator (TA)?
                const meId = payload.sub as string;
                const { rows: me } = await query<{ evaluator: boolean }>(
                    `SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`,
                    [meId]
                );
                const isTA = !!(me.length && me[0].evaluator);
                if (!isTA) {
                    const s = (reqRows[0].student || '').trim();
                    const allowed = [payload.sub, payload.name, payload.email].filter(Boolean).map(v => String(v).trim());
                    if (!allowed.includes(s)) {
                        return NextResponse.json({ error: 'forbidden' }, { status: 403 });
                    }
                }
            }
            const { rows } = await query(`SELECT id, request_id as "requestId", sender_role as "senderRole", sender, message, to_char(created_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt" FROM question_review_request_messages WHERE request_id=$1 AND owner_id = current_setting('app.current_instructor')::uuid ORDER BY created_at ASC`, [requestId]);
            return NextResponse.json(rows);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// POST { requestId, message }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { requestId, message } = body || {};
        if (!requestId || !message) return NextResponse.json({ error: 'requestId and message required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            const { rows: reqRows } = await query(`SELECT id, student, status FROM question_review_requests WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [requestId]);
            if (!reqRows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
            const reqRow = reqRows[0];
            if (reqRow.status === 'Resolved') return NextResponse.json({ error: 'Request resolved' }, { status: 400 });
            if (payload.role === 'student') {
                // If student is an evaluator (TA), allow posting as staff; else only allow the owning student
                const meId = payload.sub as string;
                const { rows: me } = await query<{ evaluator: boolean }>(
                    `SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`,
                    [meId]
                );
                const isTA = !!(me.length && me[0].evaluator);
                if (!isTA) {
                    const s = (reqRow.student || '').trim();
                    const allowed = [payload.sub, payload.name, payload.email].filter(Boolean).map(v => String(v).trim());
                    if (!allowed.includes(s)) {
                        return NextResponse.json({ error: 'forbidden' }, { status: 403 });
                    }
                }
            }
            // Treat evaluator TA messages as instructor-side for clarity in UI
            let senderRole: string = payload.role;
            if (payload.role === 'student') {
                const meId = payload.sub as string;
                const { rows: me } = await query<{ evaluator: boolean }>(
                    `SELECT evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`,
                    [meId]
                );
                if (me.length && me[0].evaluator) senderRole = 'instructor';
            }
            const sender = payload.name || payload.sub;
            const { rows } = await query(`INSERT INTO question_review_request_messages (request_id, sender_role, sender, message, owner_id) VALUES ($1,$2,$3,$4,current_setting('app.current_instructor')::uuid) RETURNING id, request_id as "requestId", sender_role as "senderRole", sender, message, to_char(created_at,'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') as "createdAt"`, [requestId, senderRole, String(sender), String(message).slice(0, 4000)]);
            return NextResponse.json(rows[0]);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
