import { NextRequest, NextResponse } from 'next/server';
import { query, ensureInstructorTables } from '@/lib/db';
import { signJwt, verifyJwt, parseAuthHeader } from '@/lib/auth';
import bcrypt from 'bcryptjs';

// POST /api/auth { email, name, password } -> simplistic local auth (no hashing for demo) returns JWT
// GET /api/auth (with Authorization) -> validate token / refresh

export async function POST(req: NextRequest) {
    try {
        const body = await req.json().catch(() => ({}));
        const { email, name, password } = body || {};
        if (!email || !password) return NextResponse.json({ error: 'email & password required' }, { status: 400 });
        const normEmail = String(email).toLowerCase().trim();

        // Ensure minimal instructor schema exists
        await ensureInstructorTables();

        const existing = await query<{ id: string; name: string }>(`SELECT id, name FROM instructors WHERE email=$1 LIMIT 1`, [normEmail]);
        let instructorId: string;
        let displayName = name;
        if (!existing.rows.length) {
            return NextResponse.json({ error: 'account not found' }, { status: 404 });
        }
        instructorId = existing.rows[0].id;
        displayName = existing.rows[0].name;
        const pw = await query<{ password: string }>(`SELECT password FROM instructor_secrets WHERE instructor_id=$1`, [instructorId]);
        if (!pw.rows.length) {
            return NextResponse.json({ error: 'credentials not provisioned' }, { status: 400 });
        }
        const stored = pw.rows[0].password;
        let ok = false;
        if (stored.startsWith('$2a$') || stored.startsWith('$2b$')) {
            ok = await bcrypt.compare(password, stored);
        } else {
            // Legacy plain password: upgrade transparently if matches
            if (stored === password) {
                const hash = await bcrypt.hash(password, 10);
                await query(`UPDATE instructor_secrets SET password=$2 WHERE instructor_id=$1`, [instructorId, hash]);
                ok = true;
            }
        }
        if (!ok) return NextResponse.json({ error: 'invalid credentials' }, { status: 401 });

        const token = signJwt({ sub: instructorId, email: normEmail, name: displayName, role: 'instructor' });
        return NextResponse.json({ token, instructor: { id: instructorId, email: normEmail, name: displayName } });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

export async function GET(req: NextRequest) {
    try {
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'missing token' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        return NextResponse.json({ instructor: { id: payload.sub, email: payload.email, name: payload.name }, token });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
