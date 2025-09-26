import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query } from '@/lib/db';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

export async function GET(_req: NextRequest, { params }: { params: Promise<{ token: string }> }) {
    try {
        await ensureInit();
        const { token } = await params;
        const { rows } = await query<{ email: string; name: string; expires_at: string | null; used_at: string | null }>(`SELECT email, name, expires_at, used_at FROM invites WHERE token=$1 LIMIT 1`, [String(token)]);
        if (!rows.length) return NextResponse.json({ error: 'not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e: unknown) {
        return NextResponse.json({ error: e instanceof Error ? e.message : 'failed' }, { status: 500 });
    }
}
