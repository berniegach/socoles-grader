import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/datasets  (optional ?q= filter)
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const q = (searchParams.get('q') || '').trim().toLowerCase();
        if (q) {
            const { rows } = await withInstructorContext(payload.sub, () => query(`SELECT id, name, sql FROM datasets WHERE owner_id = current_setting('app.current_instructor')::uuid AND LOWER(name) LIKE '%' || $1 || '%' ORDER BY name ASC`, [q]));
            return NextResponse.json(rows);
        }
        const { rows } = await withInstructorContext(payload.sub, () => query(`SELECT id, name, sql FROM datasets WHERE owner_id = current_setting('app.current_instructor')::uuid ORDER BY name ASC`));
        return NextResponse.json(rows);
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// POST /api/datasets  body: { name, sql }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { name, sql } = body || {};
        if (!name || !String(name).trim()) return NextResponse.json({ error: 'name required' }, { status: 400 });
        const nm = String(name).trim();
        const s = String(sql || '');
        const { rows } = await withInstructorContext(payload.sub, () => query(`INSERT INTO datasets (name, sql, owner_id) VALUES ($1,$2,current_setting('app.current_instructor')::uuid) RETURNING id, name, sql`, [nm, s]));
        return NextResponse.json(rows[0]);
    } catch (e: any) {
        if ((e?.message || '').toLowerCase().includes('unique')) {
            return NextResponse.json({ error: 'Dataset name must be unique' }, { status: 409 });
        }
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// PATCH /api/datasets  body: { id, name?, sql? }
export async function PATCH(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body = await req.json();
        const { id, name, sql } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        const { rows } = await withInstructorContext(payload.sub, () => query(`UPDATE datasets SET name=COALESCE($2, name), sql=COALESCE($3, sql), updated_at=now() WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid RETURNING id, name, sql`, [id, name ?? null, sql ?? null]));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}

// DELETE /api/datasets?id=...
export async function DELETE(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const { searchParams } = new URL(req.url);
        const id = searchParams.get('id');
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        await withInstructorContext(payload.sub, () => query(`DELETE FROM datasets WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [id]));
        return NextResponse.json({ ok: true });
    } catch (e: any) {
        return NextResponse.json({ error: e.message || 'failed' }, { status: 500 });
    }
}
