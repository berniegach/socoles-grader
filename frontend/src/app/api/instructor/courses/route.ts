import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withCourseContext, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

function getCourseIdFromRequest(req: NextRequest): string | null {
    const hdr = req.headers.get('x-course-id');
    if (hdr && hdr.trim()) return hdr.trim();
    const url = new URL(req.url);
    const q = url.searchParams.get('courseId');
    return q && q.trim() ? q.trim() : null;
}

// GET /api/instructor/courses -> list courses for instructor
export async function GET(req: NextRequest) {
    try {
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload || payload.role !== 'instructor' || !payload.sub) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }

        await initSchema();
        const instructorId = payload.sub;

        const result = await withInstructorContext(instructorId, async () => {
            const { rows: courses } = await query<any>(
                `SELECT c.id, c.name, c.enrollment_code as "enrollmentCode", c.attempts, c.late_penalty as "latePenalty", c.pass_threshold as "passThreshold", c.grading_defaults as "gradingDefaults",
                (SELECT default_course_id FROM instructor_settings WHERE instructor_id=current_setting('app.current_instructor', true)::uuid LIMIT 1) as "defaultCourseId"
         FROM courses c
         WHERE c.owner_id = current_setting('app.current_instructor', true)::uuid
         ORDER BY c.created_at ASC`
            );
            const defaultCourseId = courses[0]?.defaultCourseId || null;
            const cleaned = courses.map((c: any) => ({
                id: c.id,
                name: c.name,
                enrollmentCode: c.enrollmentCode,
                attempts: c.attempts,
                latePenalty: c.latePenalty,
                passThreshold: c.passThreshold,
                gradingDefaults: c.gradingDefaults,
                isDefault: defaultCourseId ? c.id === defaultCourseId : false
            }));
            return { courses: cleaned, defaultCourseId };
        });

        return NextResponse.json(result);
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}

// POST /api/instructor/courses -> create a new course (optionally set as default)
export async function POST(req: NextRequest) {
    try {
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload || payload.role !== 'instructor' || !payload.sub) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }

        await initSchema();
        const instructorId = payload.sub;

        const body = await req.json().catch(() => ({}));
        const name = String(body?.name || '').trim();
        const enrollmentCode = String(body?.enrollmentCode || '').trim();
        const makeDefault = Boolean(body?.makeDefault);

        if (!name) return NextResponse.json({ error: 'name is required' }, { status: 400 });

        const created = await withInstructorContext(instructorId, async () => {
            const { rows } = await query<any>(
                `INSERT INTO courses (owner_id, name, enrollment_code)
         VALUES (current_setting('app.current_instructor', true)::uuid, $1, $2)
         RETURNING id, name, enrollment_code as "enrollmentCode"`,
                [name, enrollmentCode]
            );
            const course = rows[0];
            if (makeDefault && course?.id) {
                await query(
                    `INSERT INTO instructor_settings (instructor_id, default_course_id)
           VALUES (current_setting('app.current_instructor', true)::uuid, $1)
           ON CONFLICT (instructor_id) DO UPDATE SET default_course_id=$1`,
                    [course.id]
                );
            }
            return course;
        });

        return NextResponse.json({ course: created }, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}

// PATCH /api/instructor/courses -> set the default course
export async function PATCH(req: NextRequest) {
    try {
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload || payload.role !== 'instructor' || !payload.sub) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }

        await initSchema();
        const instructorId = payload.sub;
        const body = await req.json().catch(() => ({}));
        const courseId = String(body?.courseId || getCourseIdFromRequest(req) || '').trim();
        if (!courseId) return NextResponse.json({ error: 'courseId is required' }, { status: 400 });

        await withCourseContext(instructorId, courseId, async () => {
            // ensure course belongs to instructor + courseId is visible under RLS
            const { rows } = await query<{ id: string }>(
                `SELECT id FROM courses WHERE id=$1 AND owner_id=current_setting('app.current_instructor', true)::uuid LIMIT 1`,
                [courseId]
            );
            if (!rows.length) throw new Error('course not found');

            await query(
                `INSERT INTO instructor_settings (instructor_id, default_course_id)
         VALUES (current_setting('app.current_instructor', true)::uuid, $1)
         ON CONFLICT (instructor_id) DO UPDATE SET default_course_id=$1`,
                [courseId]
            );
        });

        return NextResponse.json({ ok: true });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}
