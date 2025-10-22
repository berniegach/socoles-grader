import { NextRequest, NextResponse } from 'next/server';
import { auth } from '@/auth';
import { initSchema } from '@/lib/db';
import { query } from '@/lib/db';

// POST /api/instructor/provision { signupCode }
// Requires a valid NextAuth session (Keycloak SSO),
// enforces INSTRUCTOR_SIGNUP_CODE and optional INSTRUCTOR_ALLOWED_DOMAINS,
// creates instructor if not exists, and returns the instructor record.
export async function POST(req: NextRequest) {
    try {
        const session: any = await auth();
        if (!session || !session.user || !session.user.email) {
            return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        }

        const email = String(session.user.email).trim().toLowerCase();
        const name = String(session.user.name || session.user.email).trim();

        // Optional domain allowlist
        const allowlist = (process.env.INSTRUCTOR_ALLOWED_DOMAINS || '')
            .split(',')
            .map((s) => s.trim().toLowerCase())
            .filter(Boolean);
        if (allowlist.length) {
            const domain = email.split('@')[1] || '';
            if (!allowlist.includes(domain)) {
                return NextResponse.json({ error: 'email domain not allowed' }, { status: 403 });
            }
        }

        const body = await req.json().catch(() => ({}));
        const providedCode: string | undefined = body?.signupCode ? String(body.signupCode) : undefined;
        const requiredCode = process.env.INSTRUCTOR_SIGNUP_CODE;
        if (requiredCode && (!providedCode || providedCode !== requiredCode)) {
            return NextResponse.json({ error: 'invalid signup code' }, { status: 403 });
        }

        await initSchema();

        // Idempotent upsert: if instructor exists, return it
        const existing = await query<{ id: string }>(`SELECT id FROM instructors WHERE email=$1 LIMIT 1`, [email]);
        if (existing.rows.length) {
            return NextResponse.json({ instructor: { id: existing.rows[0].id, email, name }, created: false });
        }

        const ins = await query<{ id: string }>(`INSERT INTO instructors (email, name) VALUES ($1,$2) RETURNING id`, [email, name]);
        const instructorId = ins.rows[0].id;
        return NextResponse.json({ instructor: { id: instructorId, email, name }, created: true }, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e?.message || 'failed' }, { status: 500 });
    }
}
