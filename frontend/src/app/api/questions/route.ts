import { NextRequest, NextResponse } from 'next/server';
import { query, initSchema, withInstructorContext } from '@/lib/db';
import type { Question, QuestionDetail, NewQuestionPayload } from '@/lib/types';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

// Ensure schema exists on first access
let initialized = false;
async function ensureInit() {
    if (!initialized) {
        await initSchema();
        initialized = true;
    }
}

export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        // Attempts derived metric restricted to this instructor's data.
        const { rows } = await withInstructorContext(payload.sub, () => query<Question>(`
            SELECT q.id,
                         q.title,
                         q.difficulty,
                         q.status,
                         COALESCE(stats.solved, 0)::int AS attempts,
                         q.max_points as "maxPoints",
                         q.model_queries as "modelQueries",
                         q.init_sql as "initSql",
                         q.use_default_grading as "useDefaultGrading",
                         q.grading_options as "gradingOptions"
                FROM questions q
                LEFT JOIN LATERAL (
                        SELECT COUNT(DISTINCT qs.student) AS solved
                        FROM question_submissions qs
                        WHERE qs.question_id = q.id
                            AND qs.owner_id = current_setting('app.current_instructor')::uuid
                            AND qs.grade IS NOT NULL
                            AND ((q.max_points > 0 AND qs.grade >= q.max_points * 0.999) OR qs.status = 'Auto-graded')
                ) stats ON true
             WHERE q.owner_id = current_setting('app.current_instructor')::uuid
             ORDER BY q.created_at DESC
             LIMIT 500`));
        return NextResponse.json(rows);
    } catch (e) {
        console.error('[questions.GET] error', e);
        return NextResponse.json({ error: (e as Error).message || 'failed', stack: process.env.NODE_ENV !== 'production' ? String((e as Error).stack || '') : undefined }, { status: 500 });
    }
}

export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body: NewQuestionPayload & { publish?: boolean; modelQueries?: string[]; id?: string; initSql?: string; useDefaultGrading?: boolean; gradingOptions?: unknown } = await req.json();
        const { title, difficulty, maxPoints, dataset, prompt, modelSql, hints, publish, modelQueries, initSql, useDefaultGrading, gradingOptions } = body;
        if (!title) return NextResponse.json({ error: 'Title required' }, { status: 400 });
        const status = publish ? 'Published' : 'Draft';
        const { rows } = await withInstructorContext(payload.sub, () => query<QuestionDetail>(
            `INSERT INTO questions (title, difficulty, status, max_points, dataset, prompt, model_sql, hints, model_queries, init_sql, use_default_grading, grading_options, owner_id)
             VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,current_setting('app.current_instructor')::uuid)
             RETURNING id, title, difficulty, status, 0 as attempts, max_points as "maxPoints", init_sql as "initSql"`,
            [title, difficulty || 'Beginner', status, maxPoints || 0, dataset || 'Default', prompt || '', modelSql || '', hints || '', Array.isArray(modelQueries) ? modelQueries : [], initSql || '', useDefaultGrading !== false, gradingOptions ? JSON.stringify(gradingOptions) : null]
        ));
        return NextResponse.json(rows[0]);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

// PATCH update an existing question
export async function PATCH(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body: Partial<NewQuestionPayload> & { id?: string; publish?: boolean; modelQueries?: string[]; initSql?: string; useDefaultGrading?: boolean; gradingOptions?: unknown } = await req.json();
        const { id, title, difficulty, maxPoints, dataset, prompt, modelSql, hints, publish, modelQueries, initSql, useDefaultGrading, gradingOptions } = body || {};
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        if (!title) return NextResponse.json({ error: 'title required' }, { status: 400 });
        const status = publish ? 'Published' : 'Draft';
        const { rows } = await withInstructorContext(payload.sub, () => query<QuestionDetail>(`UPDATE questions SET title=$2, difficulty=$3, status=$4, max_points=$5, dataset=$6, prompt=$7, model_sql=$8, hints=$9, model_queries=$10, init_sql=$11, use_default_grading=$12, grading_options=$13, updated_at=now()
                                        WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid RETURNING id, title, difficulty, status,
                                                (SELECT COUNT(DISTINCT qs.student)::int FROM question_submissions qs WHERE qs.question_id = $1 AND qs.owner_id = current_setting('app.current_instructor')::uuid AND qs.grade IS NOT NULL AND ((questions.max_points > 0 AND qs.grade >= questions.max_points * 0.999) OR qs.status='Auto-graded')) as attempts,
                                                max_points as "maxPoints", init_sql as "initSql"`, [id, title, difficulty || 'Beginner', status, maxPoints || 0, dataset || 'Default', prompt || '', modelSql || '', hints || '', Array.isArray(modelQueries) ? modelQueries : [], initSql || '', useDefaultGrading !== false, gradingOptions ? JSON.stringify(gradingOptions) : null]));
        if (!rows.length) return NextResponse.json({ error: 'Not found' }, { status: 404 });
        return NextResponse.json(rows[0]);
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}

// DELETE a question by id (querystring or body), scoped to current instructor
export async function DELETE(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const url = new URL(req.url);
        let id = url.searchParams.get('id');
        if (!id) {
            try { const body = await req.json(); id = (body as any)?.id; } catch { /* ignore */ }
        }
        if (!id) return NextResponse.json({ error: 'id required' }, { status: 400 });
        await withInstructorContext(payload.sub, () => query(`DELETE FROM questions WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [id]));
        return NextResponse.json({ ok: true });
    } catch (e) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
