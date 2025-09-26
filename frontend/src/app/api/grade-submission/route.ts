import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';
import { GradeSubmissionRequest, GradeSubmissionResponse, PerQuestionGradeResult, QuestionRubric } from '@/lib/types';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// In-memory job store for async grading
type JobStatus = 'queued' | 'running' | 'succeeded' | 'failed';
type JobRecord = { id: string; status: JobStatus; result?: GradeSubmissionResponse; error?: string };
const jobs: Record<string, JobRecord> = {};
function newJobId() { return Math.random().toString(36).slice(2) + Math.random().toString(36).slice(2); }

// Small helper: fetch with timeout (Node 18+)
async function fetchWithTimeout(url: string, init: RequestInit, timeoutMs: number) {
    const controller = new AbortController();
    const t = setTimeout(() => controller.abort(), timeoutMs);
    try {
        return await fetch(url, { ...init, signal: controller.signal });
    } finally {
        clearTimeout(t);
    }
}

// Core grading function extracted so we can run inline or in a background job
async function gradeSubmissionCore(instructorId: string, submissionId: string): Promise<GradeSubmissionResponse> {
    return await withInstructorContext(instructorId, async () => {
        const subRes = await query<{ id: string; student: string; assignment: string; date: string; grade: number | null; status: string; assignmentId: string }>(
            `SELECT s.id, s.student, s.assignment, s.date, s.grade::float8 as grade, s.status,
                            (SELECT a.id FROM assignments a WHERE a.title = s.assignment AND a.owner_id = current_setting('app.current_instructor')::uuid LIMIT 1) as "assignmentId"
             FROM submissions s WHERE s.id=$1 AND s.owner_id = current_setting('app.current_instructor')::uuid LIMIT 1`, [submissionId]
        );
        if (!subRes.rows.length) throw new Error('Submission not found');
        const submission = subRes.rows[0];
        if (!submission.assignmentId) throw new Error('Assignment not found for submission');
        interface RawQuestionRow { id: string; title: string; model_sql: string; model_queries: string[] | null; init_sql: string | null; use_default_grading: boolean | null; grading_options: Record<string, unknown> | null }
        const { rows: questions } = await query<RawQuestionRow>(
            `SELECT q.id, q.title, q.model_sql, q.model_queries, q.init_sql, q.use_default_grading, q.grading_options
             FROM assignment_questions aq JOIN questions q ON q.id = aq.question_id
             WHERE aq.assignment_id=$1 AND aq.owner_id = current_setting('app.current_instructor')::uuid
             ORDER BY aq.position ASC`, [submission.assignmentId]
        );
        interface RawQuestionSubmissionRow { id: string; question_id: string; sql: string; grade: number | null; status: string; rubric: QuestionRubric | null; feedback: string[]; attempt: number }
        const { rows: qsRows } = await query<RawQuestionSubmissionRow>(`SELECT id, question_id, sql, grade::float8 as grade, status, rubric, feedback, attempt
             FROM question_submissions WHERE submission_id=$1 AND owner_id = current_setting('app.current_instructor')::uuid ORDER BY created_at ASC`, [submissionId]);
        const qsByQ: Record<string, typeof qsRows[number]> = Object.fromEntries(qsRows.map(r => [r.question_id, r]));
        // Prefer internal container network endpoint for server-side calls, fall back to public URL
        const API_BASE = process.env.SOCOLES_INTERNAL_API_URL || process.env.NEXT_PUBLIC_SOCOLES_API_URL || 'http://localhost:5000';
        const GRADE_PATH = process.env.NEXT_PUBLIC_SOCOLES_GRADE_PATH || '/grade-queries';
        const perQuestion: PerQuestionGradeResult[] = [];
        let anyErrors = false;

        // Helper: robust number parsing from various API formats
        function parseNum(v: unknown): number {
            if (typeof v === 'number') return v;
            if (typeof v === 'string') {
                const m = v.match(/-?\d+(?:\.\d+)?/);
                return m ? Number(m[0]) : NaN;
            }
            return NaN;
        }

        for (const q of questions) {
            const qs = qsByQ[q.id];
            const title = q.title || q.id;
            // If no answer captured or empty SQL, flag as needs review
            if (!qs || !qs.sql || !String(qs.sql).trim()) {
                anyErrors = true;
                if (qs) {
                    await query(`UPDATE question_submissions SET status='Needs review' WHERE id=$1`, [qs.id]);
                }
                perQuestion.push({ questionId: q.id, id: qs?.id, title, grade: null, status: 'Needs review', error: 'No answer provided' });
                continue;
            }

            // Prepare grader payload
            const modelQueries: string[] = (Array.isArray(q.model_queries) && q.model_queries.length > 0)
                ? q.model_queries as string[]
                : ((q.model_sql && String(q.model_sql).trim()) ? [q.model_sql] : []);
            const initSql: string = q.init_sql || '';

            let gradeVal: number | null = null;
            let feedback: string[] = [];
            let status: string = 'Auto-graded';
            let error: string | undefined;

            try {
                // Determine grading parameters per question
                const useDefault = q.use_default_grading !== false; // default true
                const base: Record<string, unknown> = {
                    syntaxSensitivity: '3',
                    semanticsSensitivity: '8',
                    resultsSensitivity: '3',
                    evaluationPriority: '5',
                    textEditDistance: '4',
                    treeEditDistance: '4',
                    checkOrder: false,
                    autoDB: false,
                    numberOfDBs: '0',
                    dbName: '',
                    use_postgresql: true,
                };
                const rawOpts = (!useDefault && q.grading_options && typeof q.grading_options === 'object') ? q.grading_options : {};
                const merged: Record<string, unknown> = { ...base, ...rawOpts };
                const toInt = (v: unknown, d: number) => { const n = parseInt(String(v ?? '').trim(), 10); return Number.isFinite(n) ? n : d; };
                const payload = {
                    sql_data: initSql,
                    sql_create_data: initSql,
                    queries: [[encodeURIComponent(submission.student), (qs.attempt || 0) + 1, '1', qs.sql]],
                    model_queries: modelQueries,
                    syntax: toInt(merged.syntaxSensitivity, 3),
                    semantics: toInt(merged.semanticsSensitivity, 8),
                    results: toInt(merged.resultsSensitivity, 3),
                    prop_order: toInt(merged.evaluationPriority, 5),
                    edit_dist: toInt(merged.textEditDistance, 4),
                    tree_dist: toInt(merged.treeEditDistance, 4),
                    check_order: (merged as any).checkOrder ? 1 : 0,
                    auto_db: (merged as any).autoDB ? 1 : 0,
                    num_db: toInt(merged.numberOfDBs, 0),
                    dbname: (merged as any).dbName?.toString() || '',
                    use_postgresql: !!(merged as any).use_postgresql,
                };
                // Apply a server-side timeout of 25s per question
                const resp = await fetchWithTimeout(`${API_BASE}${GRADE_PATH}`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) }, 25000);
                if (!resp.ok) throw new Error(`Grader error (${resp.status})`);
                let g: number | null = null; let fb: string[] = [];
                try {
                    const data: unknown = await resp.json();
                    if (Array.isArray(data) && data.length) {
                        const first = data[0] as Record<string, unknown>;
                        const candidates = [first.Score, first.score, first.Grade, first.grade, first.finalScore, first.final_score];
                        const gParsed = candidates.map(parseNum).find((n) => isFinite(n));
                        g = typeof gParsed === 'number' ? gParsed : null;
                        const raw = (first.Feedback ?? first.feedback ?? '') as unknown;
                        fb = Array.isArray(raw) ? raw as string[] : String(raw).split(/;|\n/).filter((s: string) => s.trim());
                    } else if (data && typeof data === 'object') {
                        const anyData = data as Record<string, unknown>;
                        const candidates = [anyData.Score, anyData.score, anyData.Grade, anyData.grade, anyData.finalScore, anyData.final_score];
                        const gParsed = candidates.map(parseNum).find((n) => isFinite(n));
                        g = typeof gParsed === 'number' ? gParsed : null;
                        const raw = (anyData.Feedback ?? anyData.feedback ?? '') as unknown;
                        fb = Array.isArray(raw) ? raw as string[] : String(raw).split(/;|\n/).filter((s: string) => s.trim());
                    }
                } catch { /* ignore parse errors */ }
                if (g === null) {
                    anyErrors = true;
                    status = 'Needs review';
                }
                gradeVal = g;
                feedback = fb;
            } catch (e: unknown) {
                anyErrors = true;
                status = 'Needs review';
                const msg = e instanceof Error ? e.message : 'Auto-grading failed';
                // propagate error message into per-question entry
                // but continue to persist status
            }

            // Persist result back into question_submissions and (optionally) history
            try {
                const rubric: QuestionRubric | null = gradeVal != null ? { correctness: (gradeVal || 0) * 10, style: 60, efficiency: 50 } : null;
                await query(`UPDATE question_submissions SET grade=$2, rubric=$3, feedback=$4, status=$5 WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [qs.id, gradeVal, rubric, feedback, status]);
                // Do not increment attempt or create history on automatic grading
            } catch (e: unknown) {
                anyErrors = true;
                status = 'Needs review';
            }

            perQuestion.push({ questionId: q.id, id: qs.id, title, grade: gradeVal, status });
        }

        // Compute overall grade as SUM of numeric per-question results
        const numericGrades = perQuestion.map(r => r.grade).filter((g): g is number => typeof g === 'number' && isFinite(g));
        const total = numericGrades.reduce((a, b) => a + (b || 0), 0);
        const finalStatus = anyErrors ? 'Needs review' : 'Auto-graded';
        const subUpd = await query<{ id: string; student: string; assignment: string; date: string; grade: number; status: string }>(
            `UPDATE submissions SET grade=$2, status=$3 WHERE id=$1 AND owner_id = current_setting('app.current_instructor')::uuid
             RETURNING id, student, assignment, date, grade::float8 as grade, status`, [submissionId, total, finalStatus]
        );
        const respPayload: GradeSubmissionResponse = { submission: subUpd.rows[0], results: perQuestion, status: finalStatus };
        return respPayload;
    });
}

// GET /api/grade-submission?job=<jobId> to poll async grading status
export async function GET(req: NextRequest) {
    const { searchParams } = new URL(req.url);
    const jobId = searchParams.get('job');
    if (!jobId) return NextResponse.json({ error: 'job required' }, { status: 400 });
    const job = jobs[jobId];
    if (!job) return NextResponse.json({ error: 'not found' }, { status: 404 });
    return NextResponse.json({ id: job.id, status: job.status, result: job.result, error: job.error });
}

// POST /api/grade-submission { submissionId: string, noAttemptIncrement?: boolean }
export async function POST(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const body: Partial<GradeSubmissionRequest & { async?: boolean }> = await req.json().catch(() => ({}));
        const { submissionId, async: asyncFlag } = body || {};
        if (!submissionId) return NextResponse.json({ error: 'submissionId required' }, { status: 400 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        if (asyncFlag) {
            const jobId = newJobId();
            jobs[jobId] = { id: jobId, status: 'queued' };
            // Fire and forget async grading
            (async () => {
                jobs[jobId].status = 'running';
                try {
                    const result = await gradeSubmissionCore(instructorId, submissionId);
                    jobs[jobId].result = result;
                    jobs[jobId].status = 'succeeded';
                } catch (e: unknown) {
                    jobs[jobId].status = 'failed';
                    jobs[jobId].error = e instanceof Error ? e.message : 'failed';
                }
            })();
            return NextResponse.json({ jobId }, { status: 202 });
        }
        // Synchronous grading (legacy)
        const result = await gradeSubmissionCore(instructorId, submissionId);
        return NextResponse.json(result);
    } catch (e: unknown) {
        return NextResponse.json({ error: e instanceof Error ? e.message : 'failed' }, { status: 500 });
    }
}
