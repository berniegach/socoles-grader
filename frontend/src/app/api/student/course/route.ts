import { NextRequest, NextResponse } from 'next/server';
import { query } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

// GET: fetch course name for a student (by instructorId in JWT)
export async function GET(req: NextRequest) {
    const token = parseAuthHeader(req.headers.get('authorization') || undefined);
    if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const payload = verifyJwt(token);
    if (!payload || !payload.instructorId) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const instructorId = payload.instructorId;
    const { rows } = await query<any>(
        `SELECT course_name FROM instructor_settings WHERE instructor_id=$1 LIMIT 1`,
        [instructorId]
    );
    if (!rows.length) return NextResponse.json({ error: 'not found' }, { status: 404 });
    return NextResponse.json({ course_name: rows[0].course_name });
}
