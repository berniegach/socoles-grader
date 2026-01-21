import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query } from '@/lib/db';
import bcrypt from 'bcryptjs';
import { signStudentJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// POST { email, password, instructorId? }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const body = await req.json().catch(() => ({}));
        const { email, password, instructorId } = body || {};
        if (!email || !password) return NextResponse.json({ error: 'email & password required' }, { status: 400 });
        const norm = String(email).trim().toLowerCase();
        // Find roster rows with this email; require Active status
        const whereOwner = instructorId ? 'AND owner_id=$2' : '';
        const params = instructorId ? [norm, instructorId] : [norm];
        const { rows } = await query<any>(
            `SELECT r.id as "rosterId", r.owner_id as "ownerId", r.course_id as "courseId", r.name, r.email
       FROM roster r
       WHERE lower(r.email)=$1 ${whereOwner} AND r.status='Active'
       ORDER BY r.created_at DESC`,
            params as any[]
        );
        if (!rows.length) return NextResponse.json({ error: 'account not found' }, { status: 404 });
        // If multiple instructors, must specify instructorId
        if (!instructorId && rows.length > 1) {
            return NextResponse.json({ error: 'multiple accounts found; specify instructorId' }, { status: 400, headers: { 'X-Multiple-Accounts': 'true' } });
        }
        const row = rows[0];
        const sec = await query<{ password: string }>(`SELECT password FROM student_secrets WHERE roster_id=$1`, [row.rosterId]);
        if (!sec.rows.length) return NextResponse.json({ error: 'credentials not set' }, { status: 400 });
        const ok = await bcrypt.compare(String(password), sec.rows[0].password);
        if (!ok) return NextResponse.json({ error: 'invalid credentials' }, { status: 401 });
        const token = signStudentJwt({ rosterId: row.rosterId, instructorId: row.ownerId, courseId: row.courseId || undefined, email: row.email, name: row.name }, { expiresInSec: 60 * 60 * 24 * 7 });
        return NextResponse.json({ token, student: { id: row.rosterId, instructorId: row.ownerId, email: row.email, name: row.name } });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
