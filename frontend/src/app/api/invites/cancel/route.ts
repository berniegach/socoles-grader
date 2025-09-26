import { NextRequest, NextResponse } from 'next/server';
import { initSchema, withInstructorContext, query } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// POST /api/invites/cancel { rosterId }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const body = await req.json().catch(() => ({}));
        const { rosterId } = body || {};
        if (!rosterId) return NextResponse.json({ error: 'rosterId required' }, { status: 400 });

        await withInstructorContext(payload.sub, async () => {
            // Ensure row exists and is owned
            const r = await query<{ id: string; status: string }>(`SELECT id, status FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [rosterId]);
            if (!r.rows.length) throw new Error('Roster entry not found');
            // Mark any active invites as used/canceled (where not already used)
            await query(`UPDATE invites SET used_at = COALESCE(used_at, now()) WHERE roster_id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [rosterId]);
            // Revert roster to Pending
            await query(`UPDATE roster SET status='Pending' WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [rosterId]);
        });

        return NextResponse.json({ ok: true });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
