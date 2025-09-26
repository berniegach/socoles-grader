import { NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';
import type { RosterEntry, NewRosterEntry } from '@/lib/types';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function GET(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const { rows } = await withInstructorContext(payload.sub, () => query<RosterEntry>(
            `SELECT id, name, email, status FROM roster WHERE owner_id = current_setting('app.current_instructor')::uuid ORDER BY created_at DESC LIMIT 1000`
        ));
        return NextResponse.json(rows);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function POST(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const body: NewRosterEntry | { entries: NewRosterEntry[] } = await req.json();
        const entries = Array.isArray((body as any).entries) ? (body as any).entries as NewRosterEntry[] : [body as NewRosterEntry];
        if (!entries.length) return NextResponse.json({ error: 'no entries' }, { status: 400 });

        const inserted: RosterEntry[] = [];
        await withInstructorContext(payload.sub, async () => {
            for (const e of entries) {
                const name = (e.name || '').trim();
                const email = (e.email || '').trim();
                if (!name || !email) continue;
                const status = 'Pending';
                const { rows } = await query<RosterEntry>(
                    `INSERT INTO roster (name, email, status, owner_id)
           VALUES ($1,$2,$3,current_setting('app.current_instructor')::uuid)
           ON CONFLICT (owner_id, email) DO UPDATE SET name=EXCLUDED.name, status=EXCLUDED.status
           RETURNING id, name, email, status`,
                    [name, email, status]
                );
                if (rows[0]) inserted.push(rows[0]);
            }
        });

        return NextResponse.json(inserted);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function DELETE(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const url = new URL(req.url);
        let id = url.searchParams.get('id');
        if (!id) { try { const b = await req.json(); id = b?.id; } catch { /* ignore */ } }
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });

        // Soft delete: mark status Removed so existing student tokens can be invalidated by membership checks
        const { rows: removedRows } = await withInstructorContext(payload.sub, () => query<{ id: string }>(`UPDATE roster SET status='Removed' WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid RETURNING id`, [id]));
        if (!removedRows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json({ ok: true, status: 'Removed' });
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

export async function PATCH(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });

        const body = await req.json();
        const { id, name, email, status } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        if (typeof status !== 'undefined') {
            // status is system-managed via the invite lifecycle; block manual changes
            return NextResponse.json({ error: 'status is read-only' }, { status: 400 });
        }
        const { rows } = await withInstructorContext(payload.sub, () => query(
            `UPDATE roster SET 
               name=COALESCE($2, name),
               email=COALESCE($3, email)
             WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid
             RETURNING id, name, email, status`,
            [id, name ?? null, email ?? null]
        ));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
