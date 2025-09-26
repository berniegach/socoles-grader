import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';
import bcrypt from 'bcryptjs';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// POST { currentPassword, newPassword }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload || payload.role !== 'student' || !payload.instructorId) return NextResponse.json({ error: 'forbidden' }, { status: 403 });
        const body = await req.json().catch(() => ({}));
        const { currentPassword, newPassword } = body || {};
        if (!currentPassword || !newPassword) return NextResponse.json({ error: 'currentPassword & newPassword required' }, { status: 400 });

        // Verify current password and update inside instructor context
        await withInstructorContext(payload.instructorId, async () => {
            const sec = await query<{ password: string }>(`SELECT password FROM student_secrets WHERE roster_id=$1`, [payload.sub]);
            if (!sec.rows.length) throw new Error('credentials not set');
            const ok = await bcrypt.compare(String(currentPassword), sec.rows[0].password);
            if (!ok) throw new Error('invalid current password');
            const hash = await bcrypt.hash(String(newPassword), 10);
            await query(`UPDATE student_secrets SET password=$2 WHERE roster_id=$1`, [payload.sub, hash]);
        });

        return NextResponse.json({ ok: true });
    } catch (e: unknown) {
        const msg = e instanceof Error ? e.message : 'failed';
        const status = msg.includes('invalid current password') ? 401 : 500;
        return NextResponse.json({ error: msg }, { status });
    }
}
