import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query } from '@/lib/db';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function GET(_req: NextRequest, { params }: { params: Promise<{ token: string }> }) {
    try {
        await ensureInit();
        const { token } = await params;
        const { rows } = await query<{ email: string; name: string; expires_at: string | null; used_at: string | null; owner_id: string | null }>(
            `SELECT i.email, i.name, i.expires_at, i.used_at, i.owner_id
             FROM invites i
             WHERE i.token=$1 LIMIT 1`,
            [String(token)]
        );
        if (!rows.length) return NextResponse.json({ error: 'not found' }, { status: 404 });
        const row = rows[0];
        // Fetch a representative course name: latest assignment course for this instructor
        let courseName: string | null = null;
        if (row.owner_id) {
            try {
                const a = await query<{ course: string }>(`SELECT course FROM assignments WHERE owner_id=$1 ORDER BY created_at DESC LIMIT 1`, [row.owner_id]);
                if (a.rows.length) courseName = a.rows[0].course;
            } catch { /* ignore */ }
        }
        return NextResponse.json({ email: row.email, name: row.name, expires_at: row.expires_at, used_at: row.used_at, courseName });
    } catch (e: unknown) {
        return NextResponse.json({ error: e instanceof Error ? e.message : 'failed' }, { status: 500 });
    }
}
