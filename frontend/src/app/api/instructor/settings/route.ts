import { NextRequest, NextResponse } from 'next/server';
import { query } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

// GET: fetch instructor settings
export async function GET(req: NextRequest) {
    const token = parseAuthHeader(req.headers.get('authorization') || undefined);
    if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const payload = verifyJwt(token);
    if (!payload || !payload.sub) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const instructorId = payload.sub;
    const { rows } = await query<any>(
        `SELECT course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults FROM instructor_settings WHERE instructor_id=$1 LIMIT 1`,
        [instructorId]
    );
    if (!rows.length) return NextResponse.json({ error: 'not found' }, { status: 404 });
    return NextResponse.json(rows[0]);
}

// POST: update instructor settings
export async function POST(req: NextRequest) {
    const token = parseAuthHeader(req.headers.get('authorization') || undefined);
    if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const payload = verifyJwt(token);
    if (!payload || !payload.sub) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
    const instructorId = payload.sub;
    const body = await req.json();
    const { courseName, enrollmentCode, attempts, latePenalty, passThreshold, gradingDefaults } = body || {};
    await query(
        `INSERT INTO instructor_settings (instructor_id, course_name, enrollment_code, attempts, late_penalty, pass_threshold, grading_defaults)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         ON CONFLICT (instructor_id) DO UPDATE SET course_name=$2, enrollment_code=$3, attempts=$4, late_penalty=$5, pass_threshold=$6, grading_defaults=$7`,
        [instructorId, courseName, enrollmentCode, attempts, latePenalty, passThreshold, gradingDefaults]
    );
    return NextResponse.json({ ok: true });
}
