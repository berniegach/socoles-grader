import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withCourseContext, withInstructorContext } from '@/lib/db';
import bcrypt from 'bcryptjs';
import { signStudentJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// POST { token, password, name?, email? }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const body = await req.json().catch(() => ({}));
        const { token, password, name, email } = body || {};
        if (!token || !password) return NextResponse.json({ error: 'token & password required' }, { status: 400 });
        // find invite by token
        const { rows } = await query<any>(
            `SELECT i.id, i.owner_id as "ownerId", i.roster_id as "rosterId", i.course_id as "courseId", i.email, i.name, i.expires_at, i.used_at
       FROM invites i WHERE i.token=$1 LIMIT 1`,
            [String(token)]
        );
        if (!rows.length) return NextResponse.json({ error: 'invalid token' }, { status: 404 });
        const inv = rows[0];
        if (inv.used_at) return NextResponse.json({ error: 'invite already used' }, { status: 400 });
        if (inv.expires_at && new Date(inv.expires_at) < new Date()) return NextResponse.json({ error: 'invite expired' }, { status: 400 });
        // set password and mark roster active inside instructor context
        const hash = await bcrypt.hash(String(password), 10);
        const run = <T>(fn: () => Promise<T>) => inv.courseId
            ? withCourseContext(inv.ownerId, inv.courseId, fn)
            : withInstructorContext(inv.ownerId, fn);
        await run(async () => {
            // upsert student secret on roster id
            // if already exists, overwrite (re-accept scenario)
            await query(`INSERT INTO student_secrets (roster_id, password) VALUES ($1,$2)
                   ON CONFLICT (roster_id) DO UPDATE SET password=EXCLUDED.password`, [inv.rosterId, hash]);
            await query(`UPDATE roster SET status='Active', email=COALESCE($2,email), name=COALESCE($3,name) WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [inv.rosterId, email || null, name || null]);
            await query(`UPDATE invites SET used_at=now() WHERE id=$1`, [inv.id]);
        });
        const jwt = signStudentJwt({ rosterId: inv.rosterId, instructorId: inv.ownerId, courseId: inv.courseId || undefined, email: email || inv.email, name: name || inv.name }, { expiresInSec: 60 * 60 * 24 * 7 });
        return NextResponse.json({ token: jwt, student: { id: inv.rosterId, instructorId: inv.ownerId, email: email || inv.email, name: name || inv.name } }, { status: 201 });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
