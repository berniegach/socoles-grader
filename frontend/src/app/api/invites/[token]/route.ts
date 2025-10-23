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
        // Fetch course name from instructor_settings for this instructor
        let courseName: string | null = null;
        if (row.owner_id) {
            try {
                const settingsRes = await query<{ course_name: string }>(
                    `SELECT course_name FROM instructor_settings WHERE instructor_id=$1 LIMIT 1`,
                    [row.owner_id]
                );
                if (settingsRes.rows.length && settingsRes.rows[0].course_name && settingsRes.rows[0].course_name.trim()) {
                    courseName = settingsRes.rows[0].course_name.trim();
                }
            } catch { /* ignore */ }
        }
        return NextResponse.json({ email: row.email, name: row.name, expires_at: row.expires_at, used_at: row.used_at, courseName });
    } catch (e: unknown) {
        return NextResponse.json({ error: e instanceof Error ? e.message : 'failed' }, { status: 500 });
    }
}
