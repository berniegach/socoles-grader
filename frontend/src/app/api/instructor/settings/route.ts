import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

// GET: fetch instructor settings
export async function GET(req: NextRequest) {
    const token = parseAuthHeader(req.headers.get('authorization') || undefined);
    if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const payload = verifyJwt(token);
    if (!payload || !payload.sub) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const instructorId = payload.sub;

    await initSchema();
    const settings = await withInstructorContext(instructorId, async () => {
        const { rows: sRows } = await query<any>(
            `SELECT course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults, default_course_id
             FROM instructor_settings WHERE instructor_id=$1 LIMIT 1`,
            [instructorId]
        );
        const s = sRows[0] || null;
        const defaultCourseId = (s?.default_course_id as string | null) || null;

        if (defaultCourseId) {
            const { rows: cRows } = await query<any>(
                `SELECT name as course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults
                 FROM courses
                 WHERE id=$1 AND owner_id=current_setting('app.current_instructor', true)::uuid
                 LIMIT 1`,
                [defaultCourseId]
            );
            if (cRows.length) {
                return { ...cRows[0], default_course_id: defaultCourseId };
            }
        }

        if (!s) return null;
        return {
            course_name: s.course_name,
            enrollment_code: s.enrollment_code,
            attempts: s.attempts,
            late_penalty: s.late_penalty,
            pass_threshold: s.pass_threshold,
            grading_defaults: s.grading_defaults,
            default_course_id: defaultCourseId,
        };
    });

    if (!settings) return NextResponse.json({ error: 'not found' }, { status: 404 });
    return NextResponse.json(settings);
}

// POST: update instructor settings
export async function POST(req: NextRequest) {
    const token = parseAuthHeader(req.headers.get('authorization') || undefined);
    if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const payload = verifyJwt(token);
    if (!payload || !payload.sub) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const instructorId = payload.sub;
    await initSchema();
    const body = await req.json().catch(() => ({}));
    const { courseName, enrollmentCode, attempts, latePenalty, passThreshold, gradingDefaults } = body || {};

    await withInstructorContext(instructorId, async () => {
        // Keep legacy instructor_settings updated (back-compat)
        await query(
            `INSERT INTO instructor_settings (instructor_id, course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults)
             VALUES ($1, $2, $3, $4, $5, $6, $7)
             ON CONFLICT (instructor_id) DO UPDATE SET course_name=$2, enrollment_code=$3, attempts=$4, late_penalty=$5, pass_threshold=$6, grading_defaults=$7`,
            [instructorId, courseName, enrollmentCode, attempts, latePenalty, passThreshold, gradingDefaults]
        );

        // Update default course settings too (new model)
        const { rows: sRows } = await query<{ default_course_id: string | null }>(
            `SELECT default_course_id FROM instructor_settings WHERE instructor_id=$1 LIMIT 1`,
            [instructorId]
        );
        const defaultCourseId = sRows[0]?.default_course_id || null;
        if (defaultCourseId) {
            await query(
                `UPDATE courses
                 SET name=COALESCE($2, name),
                     enrollment_code=COALESCE($3, enrollment_code),
                     attempts=COALESCE($4, attempts),
                     late_penalty=COALESCE($5, late_penalty),
                     pass_threshold=COALESCE($6, pass_threshold),
                     grading_defaults=COALESCE($7, grading_defaults),
                     updated_at=now()
                 WHERE id=$1 AND owner_id=current_setting('app.current_instructor', true)::uuid`,
                [defaultCourseId, courseName ?? null, enrollmentCode ?? null, attempts ?? null, latePenalty ?? null, passThreshold ?? null, gradingDefaults ?? null]
            );
        }
    });
    return NextResponse.json({ ok: true });
}
