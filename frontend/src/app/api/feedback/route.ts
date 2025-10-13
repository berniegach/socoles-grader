import { NextResponse } from 'next/server';
import { initSchema, withInstructorContext, query } from '@/lib/db';
import { computeAssignmentImprovement, computeSurveyEligibility } from '@/lib/feedbackStats';
import { parseAuthHeader, verifyJwt } from '@/lib/auth';

let initialized = false;
async function ensureInit() { if (!initialized) { await initSchema(); initialized = true; } }

// GET /api/feedback?assignmentId=...  (student fetch own) OR list all for instructor
export async function GET(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        const url = new URL(req.url);
        const assignmentId = url.searchParams.get('assignmentId');
        const aggregate = url.searchParams.get('aggregate');
        const instructorId = payload.role === 'student' ? (payload.instructorId as string) : payload.sub;
        const isStudent = payload.role === 'student';
        // Aggregate analytics (instructors only) over all (or specific assignment if provided)
        if (!isStudent && aggregate) {
            // Build optional assignment filter
            const params: any[] = [];
            let where = 'owner_id = current_setting(\'app.current_instructor\')::uuid';
            if (assignmentId) { where += ' AND assignment_id = $1'; params.push(assignmentId); }
            const { rows: basic } = await withInstructorContext(instructorId, () => query<any>(
                `SELECT count(*)::int AS total_responses,
                        AVG(helped_fix)::numeric(10,2) AS avg_helped_fix,
                        AVG(improved_understanding)::numeric(10,2) AS avg_improved_understanding,
                        AVG(improvement)::numeric(10,4) AS avg_improvement
                 FROM assignment_feedback_survey WHERE ${where}`,
                params
            ));
            const { rows: dist } = await withInstructorContext(instructorId, () => query<any>(
                `SELECT helped_fix, count(*)::int AS n FROM assignment_feedback_survey WHERE ${where} GROUP BY helped_fix ORDER BY helped_fix`, params));
            const { rows: dist2 } = await withInstructorContext(instructorId, () => query<any>(
                `SELECT improved_understanding, count(*)::int AS n FROM assignment_feedback_survey WHERE ${where} GROUP BY improved_understanding ORDER BY improved_understanding`, params));
            const { rows: improvements } = await withInstructorContext(instructorId, () => query<any>(
                `SELECT improvement, helped_fix, improved_understanding FROM assignment_feedback_survey WHERE ${where} AND improvement IS NOT NULL`, params));
            // Correlation (Pearson) computed in JS for simplicity
            function pearson(a: number[], b: number[]): number | null {
                if (a.length !== b.length || a.length < 2) return null;
                const n = a.length;
                const mean = (arr: number[]) => arr.reduce((x, y) => x + y, 0) / arr.length;
                const ma = mean(a), mb = mean(b);
                let num = 0, da = 0, db = 0;
                for (let i = 0; i < n; i++) { const xa = a[i] - ma; const xb = b[i] - mb; num += xa * xb; da += xa * xa; db += xb * xb; }
                const denom = Math.sqrt(da * db);
                if (!denom) return null;
                return +(num / denom).toFixed(4);
            }
            const impVals = improvements.map(r => Number(r.improvement)).filter(v => isFinite(v));
            const helpedVals = improvements.map(r => Number(r.helped_fix)).filter(v => isFinite(v));
            const understandVals = improvements.map(r => Number(r.improved_understanding)).filter(v => isFinite(v));
            const corrHelped = pearson(impVals, helpedVals);
            const corrUnderstanding = pearson(impVals, understandVals);
            const improvedCount = impVals.filter(v => v > 0).length;
            const medianImprovement = (() => {
                if (!impVals.length) return null;
                const sorted = [...impVals].sort((a, b) => a - b); const mid = Math.floor(sorted.length / 2);
                return sorted.length % 2 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
            })();
            return NextResponse.json({
                ...basic[0],
                helped_fix_distribution: dist,
                improved_understanding_distribution: dist2,
                correlation: { improvement_helped_fix: corrHelped, improvement_understanding: corrUnderstanding },
                percent_improved: basic[0]?.total_responses ? +(improvedCount / basic[0].total_responses * 100).toFixed(1) : null,
                median_improvement: medianImprovement
            });
        }
        if (assignmentId) {
            const { rows } = await withInstructorContext(instructorId, () => query<any>(`SELECT assignment_id as "assignmentId", student, helped_fix as "helpedFix", improved_understanding as "improvedUnderstanding", comment, first_score as "firstScore", final_score as "finalScore", attempt_count as "attemptCount", improvement FROM assignment_feedback_survey WHERE assignment_id=$1 AND owner_id = current_setting('app.current_instructor')::uuid${isStudent ? ' AND student=$2' : ''} LIMIT 1`, isStudent ? [assignmentId, payload.name] : [assignmentId]));
            if (rows[0]) return NextResponse.json(rows[0]);
            // Preview stats for student before submitting the survey
            if (isStudent) {
                const eligible = await computeSurveyEligibility({ instructorId, assignmentId, studentName: payload.name, studentEmail: payload.email });
                if (eligible.perfectOnFirstTry) return NextResponse.json({ notEligible: true });
                const stats = await computeAssignmentImprovement({ instructorId, assignmentId, studentName: payload.name, studentEmail: payload.email });
                return NextResponse.json(stats);
            }
            return NextResponse.json(null);
        }
        if (isStudent) {
            const { rows } = await withInstructorContext(instructorId, () => query<any>(`SELECT assignment_id as "assignmentId" FROM assignment_feedback_survey WHERE student=$1 AND owner_id = current_setting('app.current_instructor')::uuid`, [payload.name]));
            return NextResponse.json(rows);
        }
        // Instructor: list recent
        const { rows } = await withInstructorContext(instructorId, () => query<any>(`SELECT assignment_id as "assignmentId", student, helped_fix as "helpedFix", improved_understanding as "improvedUnderstanding", improvement FROM assignment_feedback_survey WHERE owner_id = current_setting('app.current_instructor')::uuid ORDER BY created_at DESC LIMIT 500`));
        return NextResponse.json(rows);
    } catch (e) {
        console.error('[feedback.GET] error', e);
        return NextResponse.json({ error: (e as Error).message }, { status: 500 });
    }
}

// POST /api/feedback  { assignmentId, helpedFix, improvedUnderstanding, comment }
export async function POST(req: Request) {
    try {
        await ensureInit();
        const token = parseAuthHeader(req.headers.get('authorization') || undefined);
        if (!token) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
        const payload = verifyJwt(token);
        if (!payload) return NextResponse.json({ error: 'invalid token' }, { status: 401 });
        if (payload.role !== 'student') return NextResponse.json({ error: 'students only' }, { status: 403 });
        const body = await req.json();
        const { assignmentId, helpedFix, improvedUnderstanding, comment } = body || {};
        if (!assignmentId || !helpedFix || !improvedUnderstanding) return NextResponse.json({ error: 'missing fields' }, { status: 400 });

        // Compute improvement metrics using shared utility
        const instructorId = payload.instructorId as string;
        const stats = await computeAssignmentImprovement({ instructorId, assignmentId, studentName: payload.name, studentEmail: payload.email });
        const { firstScore, finalScore, attemptCount, improvement } = stats;

        const { rows } = await withInstructorContext(instructorId, () => query<any>(`INSERT INTO assignment_feedback_survey (assignment_id, student, owner_id, helped_fix, improved_understanding, comment, first_score, final_score, attempt_count, improvement)
            VALUES ($1,$2,current_setting('app.current_instructor')::uuid,$3,$4,$5,$6,$7,$8,$9)
            ON CONFLICT (assignment_id, student, owner_id)
            DO UPDATE SET helped_fix=EXCLUDED.helped_fix, improved_understanding=EXCLUDED.improved_understanding, comment=EXCLUDED.comment, first_score=EXCLUDED.first_score, final_score=EXCLUDED.final_score, attempt_count=EXCLUDED.attempt_count, improvement=EXCLUDED.improvement
            RETURNING assignment_id as "assignmentId", student, helped_fix as "helpedFix", improved_understanding as "improvedUnderstanding", comment, first_score as "firstScore", final_score as "finalScore", attempt_count as "attemptCount", improvement`, [assignmentId, (payload.name || payload.email), helpedFix, improvedUnderstanding, comment || null, firstScore, finalScore, attemptCount, improvement]));
        return NextResponse.json(rows[0]);
    } catch (e) {
        console.error('[feedback.POST] error', e);
        return NextResponse.json({ error: (e as Error).message }, { status: 500 });
    }
}
