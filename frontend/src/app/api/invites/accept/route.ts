import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const body = await req.json().catch(() => ({}));
        const { token, studentEmail, studentName } = body || {};
        if (!token) return NextResponse.json({ error: 'token required' }, { status: 400 });
        const { rows } = await query<any>(
            `SELECT i.id, i.owner_id as "ownerId", i.roster_id as "rosterId", i.email, i.name, i.expires_at, i.used_at
       FROM invites i WHERE i.token=$1 LIMIT 1`,
            [String(token)]
        );
        if (!rows.length) return NextResponse.json({ error: 'invalid token' }, { status: 404 });
        const inv = rows[0];
        if (inv.used_at) return NextResponse.json({ error: 'invite already used' }, { status: 400 });
        if (inv.expires_at && new Date(inv.expires_at) < new Date()) {
            return NextResponse.json({ error: 'invite expired' }, { status: 400 });
        }
        // Run updates within the instructor context to satisfy RLS on roster
        await withInstructorContext(inv.ownerId, async () => {
            await query(`UPDATE roster SET status='Active' WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [inv.rosterId]);
            await query(`UPDATE invites SET used_at=now() WHERE id=$1`, [inv.id]);
            if (studentEmail || studentName) {
                await query(`UPDATE roster SET email=COALESCE($2, email), name=COALESCE($3, name) WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [inv.rosterId, studentEmail || null, studentName || null]);
            }
        });
        return NextResponse.json({ ok: true, rosterId: inv.rosterId });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
