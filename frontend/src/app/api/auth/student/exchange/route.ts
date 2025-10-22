import { NextRequest, NextResponse } from 'next/server';
import { auth } from '@/auth';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { signStudentJwt } from '@/lib/auth';

// POST /api/auth/student/exchange
// Body: { token }
// If there is a valid NextAuth (Keycloak) session, validate the invite token,
// activate the roster entry, mark invite used, and return a legacy student JWT.
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
        const { token } = body || {};
        if (!token) return NextResponse.json({ error: 'token required' }, { status: 400 });

        // Lookup invite by token
        const invQ = await query<any>(
            `SELECT i.id, i.owner_id as "ownerId", i.roster_id as "rosterId", i.email, i.name, i.expires_at, i.used_at
             FROM invites i WHERE i.token=$1 LIMIT 1`,
            [String(token)]
        );
        if (!invQ.rows.length) return NextResponse.json({ error: 'invalid token' }, { status: 404 });
        const inv = invQ.rows[0];
        if (inv.used_at) return NextResponse.json({ error: 'invite already used' }, { status: 400 });
        if (inv.expires_at && new Date(inv.expires_at) < new Date()) return NextResponse.json({ error: 'invite expired' }, { status: 400 });

        // Enforce that the SSO email matches the invite email by default
        if (String(inv.email).trim().toLowerCase() !== ssoEmail) {
            return NextResponse.json({ error: 'email mismatch' }, { status: 403 });
        }

        // Activate roster and mark invite as used within instructor context
        await withInstructorContext(inv.ownerId, async () => {
            await query(`UPDATE roster SET status='Active', email=COALESCE($2,email), name=COALESCE($3,name) WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [inv.rosterId, ssoEmail, ssoName]);
            await query(`UPDATE invites SET used_at=now() WHERE id=$1`, [inv.id]);
        });

        const jwt = signStudentJwt({ rosterId: inv.rosterId, instructorId: inv.ownerId, email: ssoEmail, name: ssoName }, { expiresInSec: 60 * 60 * 24 * 7 });
        return NextResponse.json({ token: jwt, student: { id: inv.rosterId, instructorId: inv.ownerId, email: ssoEmail, name: ssoName } }, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}
