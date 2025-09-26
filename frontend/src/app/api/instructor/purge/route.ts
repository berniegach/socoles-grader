import { NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

/**
 * DELETE /api/instructor/purge
 * Completely removes all instructor-owned data (assignments, questions, submissions, links, datasets, question submissions).
 * Irreversible. Requires valid instructor JWT.
 */
export async function DELETE(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        await withInstructorContext(payload.sub, async () => {
            // Order matters due to FK constraints; delete dependent rows first.
            await query(`DELETE FROM question_submissions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM submissions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM assignment_questions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM questions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM assignments WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM datasets WHERE owner_id = current_setting('app.current_instructor')::uuid`);
        });

        return NextResponse.json({ ok: true });
    } catch (e) {
        console.error('[instructor.purge.DELETE] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
