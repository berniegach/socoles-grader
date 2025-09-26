import { NextRequest, NextResponse } from 'next/server';
import { initSchema, withInstructorContext, query } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';
import type { InviteResponse } from '@/lib/types';
import { randomUUID } from 'crypto';
import { sendInviteEmail } from '@/lib/email';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

function makeLink(token: string, req: NextRequest) {
    const url = new URL(req.url);
    const base = `${url.protocol}//${url.host}`;
    return `${base}/invite/${encodeURIComponent(token)}`;
}

export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { rows } = await withInstructorContext(payload.sub, () => query<any>(
            `SELECT i.id, i.roster_id as "rosterId", i.token, i.email, i.name, i.expires_at as "expiresAt", i.used_at as "usedAt" 
       FROM invites i WHERE i.owner_id = current_setting('app.current_instructor')::uuid ORDER BY i.created_at DESC LIMIT 200`
        ));
        const list: InviteResponse[] = rows.map((r: any) => ({ ...r, link: makeLink(r.token, req) }));
        return NextResponse.json(list);
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const body = await req.json().catch(() => ({}));
        const { rosterId, email, name, expiresInDays } = body || {};
        if (!rosterId || !email || !name) return NextResponse.json({ error: 'rosterId, email, name required' }, { status: 400 });

        const exp = Number(expiresInDays) > 0 ? Number(expiresInDays) : 14;
        const rows = await withInstructorContext(payload.sub, async () => {
            // ensure roster row belongs to instructor and get its status
            const r = await query<{ id: string; status: string }>(`SELECT id, status FROM roster WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [rosterId]);
            if (!r.rows.length) throw new Error('Roster entry not found');
            const currentStatus = (r.rows[0] as any).status as string;
            if (currentStatus !== 'Pending') {
                throw new Error('Invite can only be created for Pending students');
            }
            const tokenStr = randomUUID().replace(/-/g, '') + Math.random().toString(36).slice(2, 10);
            const ins = await query<any>(
                `INSERT INTO invites (owner_id, roster_id, token, email, name, expires_at)
         VALUES (current_setting('app.current_instructor')::uuid, $1, $2, $3, $4, now() + ($5 || ' days')::interval)
         RETURNING id, roster_id as "rosterId", token, email, name, expires_at as "expiresAt", used_at as "usedAt"`,
                [rosterId, tokenStr, String(email).trim(), String(name).trim(), String(exp)]
            );
            // Move roster status to Invited when generating an invite
            await query(`UPDATE roster SET status='Invited' WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [rosterId]);
            return ins.rows as any[];
        });

        const row = rows[0];
        const link = makeLink(row.token, req);
        // Try to send email (non-fatal on failure)
        const emailResult = await sendInviteEmail({ to: row.email, name: row.name, link });
        const resp: InviteResponse & { emailSent: boolean } = { ...row, link, emailSent: !!emailResult.sent };
        return NextResponse.json(resp, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
