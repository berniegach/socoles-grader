import { NextRequest, NextResponse } from 'next/server';
import { query, ensureInstructorTables } from '@/lib/db';
import { signJwt } from '@/lib/auth';
import bcrypt from 'bcryptjs';

// POST /api/auth/signup { email, name?, password }
export async function POST(req: NextRequest) {
    try {
        const body = await req.json().catch(() => ({}));
        const { email, name, password } = body || {};
        if (!email || !password) return NextResponse.json({ error: 'email & password required' }, { status: 400 });
        const normEmail = String(email).toLowerCase().trim();
        const displayName = (name || normEmail).trim();

        // Ensure base instructor tables exist
        await ensureInstructorTables();

        const existing = await query<{ id: string }>(`SELECT id FROM instructors WHERE email=$1 LIMIT 1`, [normEmail]);
        if (existing.rows.length) {
            return NextResponse.json({ error: 'account already exists' }, { status: 409 });
        }

        const hash = await bcrypt.hash(password, 10);
        const ins = await query<{ id: string }>(`INSERT INTO instructors (email, name) VALUES ($1,$2) RETURNING id`, [normEmail, displayName]);
        const instructorId = ins.rows[0].id;
        await query(`INSERT INTO instructor_secrets (instructor_id, password) VALUES ($1,$2)`, [instructorId, hash]);

        const token = signJwt({ sub: instructorId, email: normEmail, name: displayName, role: 'instructor' });
        return NextResponse.json({ token, instructor: { id: instructorId, email: normEmail, name: displayName } }, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
