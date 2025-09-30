import { emptyMetrics, DashboardMetrics } from './types';

// Placeholder API fetch; if endpoint missing, synthesize from existing submissions fetches.
export async function fetchDashboardMetrics(authFetch: (url: string, init?: RequestInit) => Promise<Response>): Promise<DashboardMetrics> {
    try {
        // 0. Try consolidated backend endpoint first
        try {
            const consolidated = await authFetch('/api/instructor/metrics');
            if (consolidated.ok) {
                const data = await consolidated.json();
                // Validate minimal shape
                if (data && typeof data === 'object' && 'avgGrade' in data) {
                    return {
                        pendingReviews: data.pendingReviews ?? 0,
                        avgGrade: data.avgGrade ?? 0,
                        passRate: data.passRate ?? 0,
                        avgAttempts: data.avgAttempts ?? 0,
                        gradeTrend: Array.isArray(data.gradeTrend) ? data.gradeTrend : [],
                        submissionsTrend: Array.isArray(data.submissionsTrend) ? data.submissionsTrend : [],
                        questionDifficulty: Array.isArray(data.questionDifficulty) ? data.questionDifficulty : [],
                        atRisk: Array.isArray(data.atRisk) ? data.atRisk : [],
                        recent: Array.isArray(data.recent) ? data.recent : [],
                    };
                }
            }
        } catch { /* fall back silently */ }

        const [subsRes, assignmentsRes] = await Promise.all([
            authFetch('/api/submissions'),
            authFetch('/api/assignments'),
        ]);
        if (!subsRes.ok) return emptyMetrics;
        const subs = await subsRes.json();
        const assignments = assignmentsRes.ok ? await assignmentsRes.json() : [];
        const now = new Date();

        const graded = subs.filter((s: any) => typeof s.grade === 'number');
        const avgGrade = graded.length ? graded.reduce((a: number, b: any) => a + (b.grade || 0), 0) / graded.length / 10 : 0; // assuming grade out of 10
        const passRate = graded.length ? graded.filter((s: any) => (s.grade || 0) >= 6).length / graded.length : 0;
        const pendingReviews = subs.filter((s: any) => (s.status || '').toLowerCase().includes('needs')).length;

        // Trends (last 7 days)
        const dayKey = (d: Date) => d.toISOString().slice(0, 10);
        const last7: string[] = Array.from({ length: 7 }, (_, i) => {
            const d = new Date(now); d.setDate(now.getDate() - (6 - i)); return dayKey(d);
        });
        const gradeMap: Record<string, number[]> = {};
        const countMap: Record<string, number> = {};
        subs.forEach((s: any) => {
            const d = new Date(s.date.replace(' ', 'T') + 'Z');
            const k = dayKey(d);
            if (!last7.includes(k)) return;
            if (!gradeMap[k]) gradeMap[k] = [];
            if (typeof s.grade === 'number') gradeMap[k].push((s.grade || 0) / 10);
            countMap[k] = (countMap[k] || 0) + 1;
        });
        const gradeTrend = last7.map(k => ({ date: k, avg: gradeMap[k]?.length ? gradeMap[k].reduce((a, b) => a + b, 0) / gradeMap[k].length : 0 }));
        const submissionsTrend = last7.map(k => ({ date: k, count: countMap[k] || 0 }));

        // Question difficulty placeholder: group by assignment name as stand-in
        const byAssign: Record<string, { attempts: number; best: number }> = {};
        subs.forEach((s: any) => {
            const key = s.assignment || 'Unknown';
            if (!byAssign[key]) byAssign[key] = { attempts: 0, best: 0 };
            byAssign[key].attempts += 1;
            const norm = typeof s.grade === 'number' ? (s.grade / 10) : 0;
            if (norm > byAssign[key].best) byAssign[key].best = norm;
        });
        const questionDifficulty = Object.entries(byAssign).map(([title, v]) => ({ id: title, title, attempts: v.attempts, best: v.best }));
        const atRisk = questionDifficulty.filter(q => q.attempts >= 3 && q.best < 0.7).slice(0, 5);

        // Current proxy: attempts == count of submissions per assignment (NOT per-question attempt history).
        // Fallback: if we have submissions but derived avg is <1, set to 1 to signal activity.
        let avgAttempts = questionDifficulty.length ? questionDifficulty.reduce((a, b) => a + b.attempts, 0) / questionDifficulty.length : 0;
        if (avgAttempts < 1 && subs.length) avgAttempts = 1; // placeholder until real attempt histories integrated

        const recent = subs.slice(0, 5).map((s: any) => ({ id: s.id, student: s.student, assignment: s.assignment, date: s.date, grade: typeof s.grade === 'number' ? s.grade / 10 : null, status: s.status }));

        return { pendingReviews, avgGrade, passRate, avgAttempts, gradeTrend, submissionsTrend, questionDifficulty, atRisk, recent };
    } catch {
        return emptyMetrics;
    }
}
