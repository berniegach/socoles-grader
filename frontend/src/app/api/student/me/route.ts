import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false; async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/student/me -> returns current student's roster info including evaluator flag
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload || payload.role !== 'student' || !payload.instructorId) return NextResponse.json({ error: 'forbidden' }, { status: 403 });
        const rosterId = payload.sub; // student token subject is roster id
        const instructorId = payload.instructorId;
        return await withInstructorContext(instructorId, async () => {
            const { rows } = await query(`SELECT id, name, email, status, evaluator FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`, [rosterId]);
            if (!rows.length) return NextResponse.json({ error: 'not found' }, { status: 404 });
            return NextResponse.json(rows[0]);
        });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
