import { NextRequest, NextResponse } from 'next/server';
import { auth } from '@/auth';
import { initSchema, query } from '@/lib/db';
import { signStudentJwt } from '@/lib/auth';

// POST /api/auth/student/login-exchange
// Body: { instructorId?: string }
// Uses Keycloak session email to find an Active roster and mint a legacy student JWT.
export async function POST(req: NextRequest) {
    try {
        const session: any = await auth();
        if (!session?.user?.email) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }
        const ssoEmail = String(session.user.email).trim().toLowerCase();
        const ssoName = String(session.user.name || session.user.email || 'student').trim();
        await initSchema();
        const body = await req.json().catch(() => ({}));
        const { instructorId } = body || {};

        const whereOwner = instructorId ? 'AND owner_id=$2' : '';
        const params = instructorId ? [ssoEmail, instructorId] : [ssoEmail];
        const { rows } = await query<any>(
            `SELECT r.id as "rosterId", r.owner_id as "ownerId", r.name, r.email,
                    c.course as "courseName"
             FROM roster r
             LEFT JOIN LATERAL (
                 SELECT a.course
                 FROM assignments a
                 WHERE a.owner_id = r.owner_id
                 ORDER BY a.created_at DESC
                 LIMIT 1
             ) c ON true
             WHERE lower(r.email)=$1 ${whereOwner} AND r.status='Active'
             ORDER BY r.created_at DESC`,
            params as any[]
        );
        if (!rows.length) return NextResponse.json({ error: 'account not found' }, { status: 404 });
        if (!instructorId && rows.length > 1) {
            const choices = rows.map((r: any) => ({
                rosterId: r.rosterId,
                instructorId: r.ownerId,
                courseName: r.courseName || null
            }));
            return NextResponse.json({ error: 'multiple accounts', choices }, { status: 400 });
        }
        const row = rows[0];
        const email = String(row.email || ssoEmail);
        const name = String(row.name || ssoName);
        const token = signStudentJwt({ rosterId: row.rosterId, instructorId: row.ownerId, email, name }, { expiresInSec: 60 * 60 * 24 * 7 });
        return NextResponse.json({ token, student: { id: row.rosterId, instructorId: row.ownerId, email, name } });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}
