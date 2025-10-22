import { NextRequest, NextResponse } from 'next/server';
import { auth } from '@/auth';
import { initSchema, query } from '@/lib/db';
import { signJwt } from '@/lib/auth';

// POST /api/auth/exchange
// If there is a valid NextAuth (Keycloak) session for a provisioned instructor,
// mint and return the legacy JWT so the rest of the app can keep using Bearer auth unchanged.
export async function POST(_req: NextRequest) {
    try {
        const session: any = await auth();
        if (!session?.user?.email) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }
        const email = String(session.user.email).trim().toLowerCase();
        const name = String(session.user.name || session.user.email || 'instructor').trim();

        await initSchema();
        const { rows } = await query<{ id: string; name: string }>(`SELECT id, name FROM instructors WHERE lower(email)=lower($1) LIMIT 1`, [email]);
        if (!rows.length) {
            return NextResponse.json({ error: 'instructor not provisioned' }, { status: 403 });
        }
        const instructorId = rows[0].id;
        const displayName = rows[0].name || name;
        const token = signJwt({ sub: instructorId, email, name: displayName, role: 'instructor' });
        return NextResponse.json({ token, instructor: { id: instructorId, email, name: displayName } });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}
