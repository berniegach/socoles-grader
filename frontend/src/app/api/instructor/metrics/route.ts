import { NextRequest, NextResponse } from 'next/server';
import { initSchema, query, withInstructorContext } from '@/lib/db';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false; async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/instructor/metrics
// Returns consolidated dashboard metrics used by InstructorDashboard.
export async function GET(req: NextRequest) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        return await withInstructorContext(instructorId, async () => {
            // 1. Submissions (recent scope) – limit for safety
            const { rows: submissions } = await query(`SELECT s.id, s.student, s.assignment_id as "assignmentId", a.title as assignment, s.date, s.grade::float8 as grade, s.status, s.created_at
            FROM submissions s
                        LEFT JOIN assignments a ON a.id = s.assignment_id
            WHERE s.owner_id = current_setting('app.current_instructor')::uuid
                            AND s.course_id = current_setting('app.current_course', true)::uuid
            ORDER BY s.created_at DESC
            LIMIT 2000`);

            // Helper: normalize grade assumed 0..10 or already 0..1
            const norm = (g: unknown) => {
                if (g == null) return 0; const n = Number(g); if (!isFinite(n) || n < 0) return 0; return n <= 1 ? n : n / 10;
            };

            const now = new Date();
            const dayKey = (d: Date) => d.toISOString().slice(0, 10);
            const last7: string[] = Array.from({ length: 7 }, (_, i) => { const d = new Date(now); d.setDate(now.getDate() - (6 - i)); return dayKey(d); });
            const gradeMap: Record<string, number[]> = {}; const countMap: Record<string, number> = {};
            submissions.forEach((s: unknown) => {
                try {
                    const row = s as { date?: unknown; grade?: unknown };
                    const d = new Date(String(row.date || '').replace(' ', 'T') + 'Z');
                    const k = dayKey(d);
                    if (!last7.includes(k)) return;
                    if (!gradeMap[k]) gradeMap[k] = [];
                    if (typeof row.grade === 'number') gradeMap[k].push(norm(row.grade));
                    countMap[k] = (countMap[k] || 0) + 1;
                } catch {
                    /* ignore */
                }
            });
            const gradeTrend = last7.map(k => ({ date: k, avg: gradeMap[k]?.length ? gradeMap[k].reduce((a, b) => a + b, 0) / gradeMap[k].length : 0 }));
            const submissionsTrend = last7.map(k => ({ date: k, count: countMap[k] || 0 }));

            const graded = submissions.filter((s: unknown) => typeof (s as { grade?: unknown }).grade === 'number');
            const avgGrade = graded.length ? graded.reduce((a: number, s: unknown) => a + norm((s as { grade?: unknown }).grade), 0) / graded.length : 0;
            const passRate = graded.length ? graded.filter((s: unknown) => norm((s as { grade?: unknown }).grade) >= 0.6).length / graded.length : 0;
            // Pending reviews should reflect open instructor review requests, not submission statuses.
            const { rows: prCountRows } = await query(
                `SELECT COUNT(*)::int AS count
                                 FROM question_review_requests
                                 WHERE owner_id = current_setting('app.current_instructor')::uuid
                                     AND course_id = current_setting('app.current_course', true)::uuid
                                     AND status ILIKE 'pending'`
            );
            const pendingReviews = Number(prCountRows?.[0]?.count || 0);

            // 2. Question difficulty / attempts from attempts history
            const { rows: difficultyRaw } = await query(`
       SELECT qs.id,
           q.title,
           qs.assignment_id as assignment_id,
           a2.title as assignment_title,
           GREATEST(COALESCE(MAX(a.attempt), 0), 1) AS attempts,
           COALESCE(MAX(a.grade), qs.grade)::float8 AS best_raw
       FROM question_submissions qs
       JOIN questions q ON q.id = qs.question_id
       LEFT JOIN question_submission_attempts a ON a.question_submission_id = qs.id
       LEFT JOIN assignments a2 ON a2.id = qs.assignment_id
       WHERE qs.owner_id = current_setting('app.current_instructor')::uuid
         AND qs.course_id = current_setting('app.current_course', true)::uuid
         AND q.course_id = current_setting('app.current_course', true)::uuid
       GROUP BY qs.id, q.title, qs.assignment_id, a2.title
       LIMIT 500
      `);
            const questionDifficulty = difficultyRaw.map((r: unknown) => {
                const row = r as { id?: unknown; title?: unknown; assignment_id?: unknown; assignment_title?: unknown; attempts?: unknown; best_raw?: unknown };
                return {
                    id: String(row.id),
                    title: String(row.title || ''),
                    assignmentId: row.assignment_id == null ? null : String(row.assignment_id),
                    assignmentTitle: String(row.assignment_title || ''),
                    attempts: Number(row.attempts) || 1,
                    best: norm(row.best_raw),
                };
            });
            const atRisk = questionDifficulty.filter(q => q.attempts >= 3 && q.best < 0.7).slice(0, 5);
            const avgAttempts = questionDifficulty.length ? questionDifficulty.reduce((a, b) => a + b.attempts, 0) / questionDifficulty.length : 0;

            const recent = submissions.slice(0, 5).map((s: unknown) => {
                const row = s as { id?: unknown; student?: unknown; assignment?: unknown; assignmentId?: unknown; date?: unknown; grade?: unknown; status?: unknown };
                return {
                    id: String(row.id),
                    student: String(row.student || ''),
                    assignment: String(row.assignment || row.assignmentId || ''),
                    date: String(row.date || ''),
                    grade: norm(row.grade),
                    status: String(row.status || ''),
                };
            });

            return NextResponse.json({ pendingReviews, avgGrade, passRate, avgAttempts, gradeTrend, submissionsTrend, questionDifficulty, atRisk, recent });
        });
    } catch (e: unknown) {
        return NextResponse.json({ error: (e as Error).message || 'failed' }, { status: 500 });
    }
}
