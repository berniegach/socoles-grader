import { NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

/**
 * DELETE /api/instructor/purge
 * Completely removes ALL instructor-owned data and the instructor account itself.
 * Irreversible. Requires valid instructor JWT.
 *
 * Order matters due to FK constraints:
 * - question_review_request_messages -> question_review_requests
 * - question_submission_attempts -> question_submissions -> submissions
 * - assignment_questions -> questions, assignments
 * - invites -> roster -> student_secrets (or vice-versa depending on FK) — we delete secrets first safely via IN clause
 * - datasets standalone
 * - feedback surveys standalone
 * - instructor_secrets -> instructors (account)
 */
export async function DELETE(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        await withInstructorContext(payload.sub, async () => {
            // Delete dependent records in safe order
            await query(`DELETE FROM question_review_request_messages WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM question_review_requests WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            await query(`DELETE FROM question_submission_attempts WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM question_submissions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM submissions WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            await query(`DELETE FROM assignment_questions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM questions WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM assignments WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            await query(`DELETE FROM datasets WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            // Feedback surveys
            await query(`DELETE FROM assignment_feedback_survey WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            // Invites first, then student secrets (by roster), then roster
            await query(`DELETE FROM invites WHERE owner_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM student_secrets WHERE roster_id IN (SELECT id FROM roster WHERE owner_id = current_setting('app.current_instructor')::uuid)`);
            await query(`DELETE FROM roster WHERE owner_id = current_setting('app.current_instructor')::uuid`);

            // Remove instructor app secrets then the instructor row itself
            await query(`DELETE FROM instructor_secrets WHERE instructor_id = current_setting('app.current_instructor')::uuid`);
            await query(`DELETE FROM instructors WHERE id = current_setting('app.current_instructor')::uuid`);
        });

        return NextResponse.json({ ok: true });
    } catch (e) {
        console.error('[instructor.purge.DELETE] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
