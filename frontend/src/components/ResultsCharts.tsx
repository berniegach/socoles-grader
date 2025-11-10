'use client';
import React from 'react';
import { Box, Card, CardContent, CardHeader, Typography, Table, TableBody, TableCell, TableHead, TableRow, Button, Tooltip, Divider, Switch, FormControlLabel, Select, MenuItem, FormControl, InputLabel } from '@mui/material';
import { BarChart } from '@mui/x-charts/BarChart';
import { PieChart } from '@mui/x-charts/PieChart';
import { LineChart } from '@mui/x-charts/LineChart';
import { ScatterChart } from '@mui/x-charts/ScatterChart';
import { RadarChart } from '@mui/x-charts/RadarChart';
import PeopleIcon from '@mui/icons-material/People';
import AssignmentIcon from '@mui/icons-material/Assignment';
import GradeIcon from '@mui/icons-material/Grade';
import ArrowUpwardIcon from '@mui/icons-material/ArrowUpward';
import ArrowDownwardIcon from '@mui/icons-material/ArrowDownward';
import { useTheme } from '@mui/material/styles';
import { useAuth } from '@/features/auth/AuthProvider';

interface ResultsChartsProps {
    results?: any[];
    loadAttemptsForCohort?: { assignmentId: string; token?: string };
    onAdvancedChange?: (open: boolean) => void;
}
export default function ResultsCharts({ results = [] as any[], loadAttemptsForCohort, onAdvancedChange }: ResultsChartsProps) {
    const theme = useTheme();
    const { user } = useAuth();
    const [showAdvanced, setShowAdvanced] = React.useState(false);
    const toggleAdvanced = (val: boolean) => { setShowAdvanced(val); try { onAdvancedChange?.(val); } catch { /* ignore */ } };
    const [attemptAugmented, setAttemptAugmented] = React.useState<any[] | null>(null);
    const [loadingAttempts, setLoadingAttempts] = React.useState(false);
    const [selectedQuestionId, setSelectedQuestionId] = React.useState<string>('all');

    // Auto-fetch attempts if loadAttemptsFor provided; this runs client-side with token via fetch API.
    React.useEffect(() => {
        async function load() {
            if (!loadAttemptsForCohort) { setAttemptAugmented(null); return; }
            const { assignmentId, token } = loadAttemptsForCohort;
            if (!assignmentId) return;
            setLoadingAttempts(true);
            try {
                // 1. Get current question submissions for this assignment (all students)
                const baseHeaders: Record<string, string> = { 'Content-Type': 'application/json' };
                if (token) baseHeaders['Authorization'] = `Bearer ${token}`;
                const qsUrl = `/api/question-submissions?assignmentId=${encodeURIComponent(assignmentId)}&allStudents=1`;
                const qsRes = await fetch(qsUrl, { headers: baseHeaders });
                if (!qsRes.ok) throw new Error('failed question submissions');
                const qs = await qsRes.json();
                const enriched: any[] = [];
                // 2. For each question submission, fetch history attempts (sequential to limit load)
                for (const q of qs) {
                    if (!q.id) continue;
                    try {
                        const histRes = await fetch(`/api/question-submissions?historyOf=${encodeURIComponent(q.id)}`, { headers: baseHeaders });
                        if (!histRes.ok) continue;
                        const attempts = await histRes.json();
                        const normAttempts = attempts.map((a: any) => ({
                            attempt: a.attempt,
                            grade: typeof a.grade === 'number' ? (a.grade <= 1 ? a.grade : a.grade) : 0,
                            ts: a.createdAt,
                            rubric: a.rubric || null,
                            feedback: Array.isArray(a.feedback) ? a.feedback : []
                        }));
                        enriched.push({ ...q, attempts: normAttempts });
                    } catch { /* ignore single failure */ }
                }
                setAttemptAugmented(enriched);
            } catch { setAttemptAugmented(null); } finally { setLoadingAttempts(false); }
        }
        void load();
    }, [loadAttemptsForCohort?.assignmentId, loadAttemptsForCohort?.token]);
    // Reset question filter when dataset changes
    React.useEffect(() => { setSelectedQuestionId('all'); }, [attemptAugmented?.length]);
    // Optionally exclude non-final submissions (e.g., In Progress, Draft, Pending) from top-level aggregates
    const resultsFiltered = Array.isArray(results) ? results.filter((r: any) => {
        const sRaw = (r.Status ?? r.status ?? '').toString().trim().toLowerCase();
        if (!sRaw) return true; // keep if status unknown (e.g., CSV import)
        if (sRaw.includes('in progress')) return false;
        if (sRaw === 'draft' || sRaw === 'pending') return false;
        return true; // keep final-like statuses (Auto-graded, Manual, Needs review, etc.)
    }) : [];

    // Normalize grades 0..1 and compute aggregates
    const dist = Array.from({ length: 11 }, (_, i) => ({ bin: (i / 10).toFixed(1), n: 0 })); // bins labeled 0.0..1.0
    const normalizedGrades: number[] = [];
    const answers: string[] = [];

    for (const r of resultsFiltered) {
        const q = String((r as any)['Q #'] ?? (r as any).q ?? (r as any).question ?? '?');
        const rawScore = Number((r as any).Score ?? (r as any).score ?? (r as any).Grade ?? (r as any).grade ?? 0);
        const outOf = Number((r as any)['Out Of'] ?? (r as any).outOf ?? (r as any)['OutOf'] ?? 1) || 1;
        const ans = String((r as any).Answer ?? (r as any).Query ?? (r as any).query ?? '').trim();

        // Normalize to 0..1
        let normalized = outOf > 1 ? rawScore / outOf : rawScore;
        if (!isFinite(normalized) || isNaN(normalized)) normalized = 0;
        normalized = Math.max(0, Math.min(1, normalized));
        normalizedGrades.push(normalized);
        if (ans) answers.push(ans);

        // Distribution
        let bin = Math.round(normalized * 10);
        bin = Math.max(0, Math.min(10, bin));
        dist[bin].n += 1;
    }

    // Summary stats
    const totalAnswers = resultsFiltered.length;
    const uniqueAnswers = new Set(answers).size;
    const avgGrade = normalizedGrades.length ? +(normalizedGrades.reduce((a, b) => a + b, 0) / normalizedGrades.length).toFixed(2) : 0;
    const maxGrade = normalizedGrades.length ? +Math.max(...normalizedGrades).toFixed(2) : 0;
    const minGrade = normalizedGrades.length ? +Math.min(...normalizedGrades).toFixed(2) : 0;

    // Pass/Fail pie (threshold 0.6)
    const pass = normalizedGrades.filter((g) => g >= 0.6).length;
    const fail = normalizedGrades.filter((g) => g < 0.6).length;
    const passFailData = [
        { id: 0, value: pass, label: 'Pass' },
        { id: 1, value: fail, label: 'Fail' },
    ];

    // Colors for bars (11 bins)
    const COLORS = [
        theme.palette.primary.light,
        theme.palette.secondary.light,
        theme.palette.success.light,
        theme.palette.warning.light,
        theme.palette.info.light,
        theme.palette.error.light,
        theme.palette.primary.main,
        theme.palette.secondary.main,
        theme.palette.success.main,
        theme.palette.warning.main,
        theme.palette.info.main,
    ];

    // Single series with per-category colors to keep bars thick
    const distDataset = dist.map((d) => ({ bin: d.bin, n: d.n }));

    // Common incorrect answers (grade < 1.0)
    const incorrectCounts: Record<string, number> = {};
    resultsFiltered.forEach((r) => {
        const rawScore = Number((r as any).Score ?? (r as any).score ?? (r as any).Grade ?? 0);
        const outOf = Number((r as any)['Out Of'] ?? (r as any).outOf ?? 1) || 1;
        let normalized = outOf > 1 ? rawScore / outOf : rawScore;
        if (!isFinite(normalized) || isNaN(normalized)) normalized = 0;
        const ans = String((r as any).Answer ?? (r as any).Query ?? '').trim();
        if (normalized < 1 && ans) incorrectCounts[ans] = (incorrectCounts[ans] || 0) + 1;
    });
    const commonIncorrect = Object.entries(incorrectCounts)
        .map(([answer, count]) => ({ answer, count }))
        .sort((a, b) => b.count - a.count)
        .slice(0, 5);

    // -------- Advanced Charts Data Preparation --------
    // Expect each result to have attempts: [{attempt, grade, ts, rubric:{syntax,semantics,results}, feedback:[] }]
    type Attempt = { attempt: number; grade: number; ts?: string; rubric?: { syntax?: number; semantics?: number; results?: number }; feedback?: string[] };
    interface ResultWithAttempts { attempts?: Attempt[] }
    const allAttempts: Attempt[] = [];
    // Build question options when attempt data is available
    const questionOptions = React.useMemo(() => {
        if (!Array.isArray(attemptAugmented)) return [] as Array<{ id: string; label: string }>;
        const byQ = new Map<string, string>();
        attemptAugmented.forEach((q: any, idx: number) => {
            const qid = String(q.questionId ?? q.question_id ?? '');
            if (!qid) return;
            const label = String(q.title ?? q.question ?? q.name ?? q['Q #'] ?? `Question ${idx + 1}`);
            if (!byQ.has(qid)) byQ.set(qid, label);
        });
        return Array.from(byQ.entries()).map(([id, label]) => ({ id, label }));
    }, [attemptAugmented]);
    // Apply filter: when a specific question is selected, narrow down datasets
    const baseResults = React.useMemo(() => {
        if (!attemptAugmented) return resultsFiltered;
        if (!selectedQuestionId || selectedQuestionId === 'all') return attemptAugmented;
        return attemptAugmented.filter((q: any) => String(q.questionId ?? q.question_id) === String(selectedQuestionId));
    }, [attemptAugmented, resultsFiltered, selectedQuestionId]);
    (baseResults as (ResultWithAttempts & any)[]).forEach(r => {
        if (Array.isArray(r.attempts) && r.attempts.length) {
            // If backend didn't provide attempt numbers (or they are all identical), synthesize per question
            const haveValid = r.attempts.some((a: any) => typeof a.attempt === 'number' && a.attempt > 0);
            r.attempts.forEach((a: any, idx: number) => {
                const attNum = haveValid ? a.attempt : (idx + 1); // fallback sequence
                allAttempts.push({
                    attempt: attNum,
                    grade: typeof a.grade === 'number' ? Math.max(0, Math.min(1, a.grade <= 1 ? a.grade : a.grade)) : 0,
                    ts: a.createdAt || a.ts,
                    rubric: a.rubric || undefined,
                    feedback: a.feedback || []
                });
            });
        } else if (!attemptAugmented) {
            const rawScore = Number(r.Score ?? r.score ?? r.Grade ?? r.grade ?? 0);
            const outOf = Number(r['Out Of'] ?? r.outOf ?? 1) || 1;
            const grade = outOf > 1 ? (rawScore / outOf) : rawScore;
            allAttempts.push({ attempt: 1, grade: Math.max(0, Math.min(1, grade)) });
        }
    });
    // Sort attempts by attempt number
    allAttempts.sort((a, b) => a.attempt - b.attempt);
    // Aggregate averages per attempt index
    const byAttempt: Record<number, { count: number; gradeSum: number; syntaxSum: number; semanticsSum: number; resultsSum: number; syntaxN: number; semanticsN: number; resultsN: number }> = {};
    allAttempts.forEach(a => {
        const key = a.attempt;
        if (!byAttempt[key]) byAttempt[key] = { count: 0, gradeSum: 0, syntaxSum: 0, semanticsSum: 0, resultsSum: 0, syntaxN: 0, semanticsN: 0, resultsN: 0 };
        const bucket = byAttempt[key];
        bucket.count += 1;
        if (typeof a.grade === 'number' && isFinite(a.grade)) bucket.gradeSum += a.grade;
        if (a.rubric) {
            if (typeof a.rubric.syntax === 'number') { bucket.syntaxSum += a.rubric.syntax; bucket.syntaxN++; }
            if (typeof a.rubric.semantics === 'number') { bucket.semanticsSum += a.rubric.semantics; bucket.semanticsN++; }
            if (typeof a.rubric.results === 'number') { bucket.resultsSum += a.rubric.results; bucket.resultsN++; }
        }
    });
    const progressDataset = Object.keys(byAttempt).map(k => {
        const att = Number(k);
        const b = byAttempt[att];
        return {
            attempt: att,
            grade: b.gradeSum / (b.count || 1),
            syntax: b.syntaxN ? b.syntaxSum / b.syntaxN : null,
            semantics: b.semanticsN ? b.semanticsSum / b.semanticsN : null,
            results: b.resultsN ? b.resultsSum / b.resultsN : null,
        };
    }).sort((a, b) => a.attempt - b.attempt);
    // Deltas per attempt (grade + rubric components if present)
    const deltaDataset = progressDataset.slice(1).map((row, i) => {
        const prev = progressDataset[i];
        return {
            attempt: row.attempt,
            gradeDelta: row.grade - prev.grade,
            syntaxDelta: (row.syntax ?? prev.syntax ?? 0) - (prev.syntax ?? row.syntax ?? 0),
            semanticsDelta: (row.semantics ?? prev.semantics ?? 0) - (prev.semantics ?? row.semantics ?? 0),
            resultsDelta: (row.results ?? prev.results ?? 0) - (prev.results ?? row.results ?? 0),
        };
    });
    // Time gap vs improvement (requires timestamps). If absent, dataset empty.
    const attemptsWithTime = allAttempts.filter(a => a.ts && typeof a.grade === 'number').sort((a, b) => new Date(a.ts || '').getTime() - new Date(b.ts || '').getTime());
    const timeGapDataset: Array<{ gapMinutes: number; improvement: number }> = [];
    for (let i = 1; i < attemptsWithTime.length; i++) {
        const prev = attemptsWithTime[i - 1];
        const cur = attemptsWithTime[i];
        const gap = (new Date(cur.ts!).getTime() - new Date(prev.ts!).getTime()) / (1000 * 60);
        timeGapDataset.push({ gapMinutes: gap, improvement: (cur.grade - prev.grade) });
    }
    // Radar first vs best
    const first = progressDataset[0];
    const best = progressDataset.reduce((acc, r) => r.grade > (acc?.grade ?? -1) ? r : acc, first || null);
    const radarData = (first && best) ? [
        { axis: 'Syntax', first: first.syntax ?? 0, best: best.syntax ?? first.syntax ?? 0 },
        { axis: 'Semantics', first: first.semantics ?? 0, best: best.semantics ?? first.semantics ?? 0 },
        { axis: 'Results', first: first.results ?? 0, best: best.results ?? first.results ?? 0 },
    ] : [];
    // Cumulative mastery (step) – max grade reached after each attempt.
    const cumulDataset = progressDataset.map((r, i) => ({ attempt: r.attempt, bestSoFar: Math.max(...progressDataset.slice(0, i + 1).map(x => x.grade)) }));

    const attemptsSummary = React.useMemo(() => {
        if (!Array.isArray(baseResults)) return null;
        const perQuestion = baseResults.map((q: any) => ({ id: q.id, attempts: Array.isArray((q as any).attempts) ? (q as any).attempts.length : 0 }));
        const totalHist = perQuestion.reduce((a, b) => a + b.attempts, 0);
        const attemptKeys = progressDataset.map(p => p.attempt);
        return { perQuestion, totalHist, attemptKeys };
    }, [baseResults, progressDataset]);

    // Attempts vs Students distribution: total attempt records per student (sum across their questions)
    const attemptsVsStudents = React.useMemo(() => {
        // For attempt index i, count how many entities have reached at least i attempts.
        // If a specific question is selected: entities = students (unique by student).
        // If All questions: entities = student-question pairs.
        if (!Array.isArray(baseResults) || !baseResults.length) return null;
        const perEntityTotals: Record<string, number> = {};
        baseResults.forEach((q: any) => {
            const studentKey = String(q.student ?? q.studentId ?? 'unknown');
            const qid = String(q.questionId ?? q.question_id ?? '');
            const key = (selectedQuestionId && selectedQuestionId !== 'all') ? studentKey : `${studentKey}|${qid}`;
            const attCount = Array.isArray(q.attempts) ? q.attempts.length : 0;
            perEntityTotals[key] = (perEntityTotals[key] || 0) + attCount;
        });
        const totals = Object.values(perEntityTotals).filter(t => t > 0);
        if (!totals.length) return { dataset: [], keys: [] as number[] };
        const maxAttempts = Math.max(...totals);
        const dataset: { attemptIndex: number; students: number }[] = [];
        for (let i = 1; i <= maxAttempts; i++) {
            const count = totals.filter(t => t >= i).length; // entities who have "used up" attempt i
            dataset.push({ attemptIndex: i, students: count });
        }
        return { dataset, keys: dataset.map(d => d.attemptIndex) };
    }, [baseResults, selectedQuestionId]);

    if (showAdvanced) {
        return (
            <Box sx={{ display: 'grid', gap: 2 }}>
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: 2, flexWrap: 'wrap' }}>
                    <Typography variant='h6'>Attempt Progress Analytics</Typography>
                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        {Array.isArray(attemptAugmented) && attemptAugmented.length > 1 && (
                            <FormControl size='small' sx={{ minWidth: 220 }}>
                                <InputLabel id='attempt-q-filter-label'>Question</InputLabel>
                                <Select
                                    labelId='attempt-q-filter-label'
                                    label='Question'
                                    value={selectedQuestionId}
                                    onChange={(e) => setSelectedQuestionId(e.target.value)}
                                >
                                    <MenuItem value='all'><em>All questions</em></MenuItem>
                                    {questionOptions.map(q => (
                                        <MenuItem key={q.id} value={q.id}>{q.label}</MenuItem>
                                    ))}
                                </Select>
                            </FormControl>
                        )}
                        <Button size='small' onClick={() => toggleAdvanced(false)}>Back</Button>
                    </Box>
                </Box>
                {!loadingAttempts && Array.isArray(baseResults) && (
                    <Typography variant='caption' color='text.secondary'>
                        {`Questions: ${attemptsSummary?.perQuestion?.length || 0} • Students: ${new Set((baseResults as any[]).map(a => a.student)).size} • Attempt records: ${attemptsSummary?.totalHist || 0}`}
                    </Typography>
                )}
                {/* Per-student toggle removed per requirement; cohort only */}
                <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr', lg: '1fr 1fr' } }}>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Attempt Progress</Typography>} subheader={<Typography variant='caption'>Overall + rubric lines</Typography>} />
                        <CardContent>
                            {progressDataset.length ? (
                                <LineChart height={260} margin={{ top: 7, right: 7, bottom: 2, left: 0 }} dataset={progressDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} yAxis={[{ label: 'Average Grade / Rubric' }]} series={[
                                    { dataKey: 'grade', label: 'Grade', color: theme.palette.primary.main },
                                    ...(progressDataset.some(r => r.syntax != null) ? [{ dataKey: 'syntax', label: 'Syntax', color: theme.palette.success.main }] : []),
                                    ...(progressDataset.some(r => r.semantics != null) ? [{ dataKey: 'semantics', label: 'Semantics', color: theme.palette.warning.main }] : []),
                                    ...(progressDataset.some(r => r.results != null) ? [{ dataKey: 'results', label: 'Results', color: theme.palette.info.main }] : []),
                                ]} />
                            ) : <Typography variant='caption' color='text.secondary'>No attempts found.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Improvement Deltas</Typography>} subheader={<Typography variant='caption'>Change vs previous attempt</Typography>} />
                        <CardContent>
                            {deltaDataset.length ? (
                                <BarChart height={260} margin={{ top: 7, right: 7, bottom: 2, left: 0 }} dataset={deltaDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} yAxis={[{ label: 'Δ Grade / Rubric' }]} series={[
                                    { dataKey: 'gradeDelta', label: 'Grade Δ', color: theme.palette.primary.main },
                                    ...(deltaDataset.some(d => d.syntaxDelta !== 0) ? [{ dataKey: 'syntaxDelta', label: 'Syntax Δ', color: theme.palette.success.main }] : []),
                                    ...(deltaDataset.some(d => d.semanticsDelta !== 0) ? [{ dataKey: 'semanticsDelta', label: 'Semantics Δ', color: theme.palette.warning.main }] : []),
                                    ...(deltaDataset.some(d => d.resultsDelta !== 0) ? [{ dataKey: 'resultsDelta', label: 'Results Δ', color: theme.palette.info.main }] : []),
                                ]} />
                            ) : <Typography variant='caption' color='text.secondary'>Need at least 2 attempts.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Time Gap vs Improvement</Typography>} subheader={<Typography variant='caption'>Minutes between attempts</Typography>} />
                        <CardContent>
                            {timeGapDataset.length ? (
                                <ScatterChart height={260} margin={{ top: 7, right: 7, bottom: 2, left: 0 }} series={[{ data: timeGapDataset.map(p => ({ x: p.gapMinutes, y: p.improvement })) }]} xAxis={[{ label: 'Gap (min)' }]} yAxis={[{ label: 'Grade Δ' }]} />
                            ) : <Typography variant='caption' color='text.secondary'>Timestamp data unavailable.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Attempts Used per Student</Typography>} subheader={<Typography variant='caption'>How many students reached each attempt index</Typography>} />
                        <CardContent>
                            {attemptsVsStudents && attemptsVsStudents.dataset.length ? (
                                <BarChart
                                    height={260}
                                    margin={{ top: 7, right: 7, bottom: 2, left: 0 }}
                                    dataset={attemptsVsStudents.dataset}
                                    xAxis={[{
                                        dataKey: 'attemptIndex', label: 'Attempt #', colorMap: {
                                            type: 'ordinal', colors: [
                                                theme.palette.primary.light,
                                                theme.palette.secondary.light,
                                                theme.palette.success.light,
                                                theme.palette.warning.light,
                                                theme.palette.info.light,
                                                theme.palette.error.light,
                                                theme.palette.primary.main,
                                                theme.palette.secondary.main,
                                                theme.palette.success.main,
                                                theme.palette.warning.main,
                                                theme.palette.info.main,
                                            ], values: attemptsVsStudents.dataset.map(d => d.attemptIndex)
                                        }
                                    }]}
                                    yAxis={[{ label: 'Students' }]}
                                    series={[{ dataKey: 'students', label: 'Students' }]}
                                    hideLegend
                                />
                            ) : <Typography variant='caption' color='text.secondary'>Need attempt data to compute distribution.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Cumulative Mastery</Typography>} subheader={<Typography variant='caption'>Best grade so far</Typography>} />
                        <CardContent>
                            {cumulDataset.length ? (
                                <LineChart height={260} margin={{ top: 7, right: 7, bottom: 2, left: 0 }} dataset={cumulDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} yAxis={[{ label: 'Best Grade So Far' }]} series={[{ dataKey: 'bestSoFar', label: 'Best so far', color: theme.palette.primary.main }]} />
                            ) : <Typography variant='caption' color='text.secondary'>No attempts.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Question Difficulty</Typography>} subheader={<Typography variant='caption'>Avg attempts vs best grade</Typography>} />
                        <CardContent>
                            {Array.isArray(baseResults) && baseResults.length ? (() => {
                                const qMetrics = baseResults.map((q: any) => {
                                    const attempts = Array.isArray(q.attempts) ? q.attempts : [];
                                    const first = attempts[0];
                                    const best = attempts.reduce((acc: any, a: any) => (a.grade ?? 0) > (acc?.grade ?? -1) ? a : acc, first || null);
                                    return { attemptsCount: attempts.length, best: best?.grade ?? 0 };
                                });
                                const scatter = qMetrics.map(m => ({ x: m.attemptsCount, y: m.best }));
                                return <ScatterChart height={260} margin={{ top: 7, right: 7, bottom: 2, left: 0 }} series={[{ data: scatter.map(p => ({ x: p.x, y: p.y })) }]} xAxis={[{ label: 'Attempts' }]} yAxis={[{ label: 'Best Grade' }]} />;
                            })() : <Typography variant='caption' color='text.secondary'>Need attempt data.</Typography>}
                        </CardContent>
                    </Card>
                </Box>
                <InstructorSurveyAnalytics assignmentId={loadAttemptsForCohort?.assignmentId} />
            </Box>
        );
    }

    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            {/* Summary stats */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', sm: 'repeat(2, 1fr)', md: 'repeat(3, 1fr)' }, gap: 2 }}>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <PeopleIcon color="primary" />
                        <Box>
                            <Typography variant="h5">{totalAnswers}</Typography>
                            <Typography color="text.secondary">Total Answers</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <AssignmentIcon color="secondary" />
                        <Box>
                            <Typography variant="h5">{uniqueAnswers}</Typography>
                            <Typography color="text.secondary">Unique Answers</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <GradeIcon color="success" />
                        <Box>
                            <Typography variant="h5">{avgGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Average Grade (0–1)</Typography>
                        </Box>
                    </CardContent>
                </Card>
            </Box>

            {/* High/Low */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', sm: '1fr 1fr' }, gap: 2 }}>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <ArrowUpwardIcon color="info" />
                        <Box>
                            <Typography variant="h5">{maxGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Highest Grade</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <ArrowDownwardIcon color="error" />
                        <Box>
                            <Typography variant="h5">{minGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Lowest Grade</Typography>
                        </Box>
                    </CardContent>
                </Card>
            </Box>

            {/* Visualizations */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', md: '1fr 1fr' }, gap: 2 }}>
                <Card>
                    <CardHeader title={<Typography variant="subtitle1">Grade Distribution (0–1)</Typography>} subheader={<Typography variant='caption' color='text.secondary'>N = {totalAnswers}</Typography>} />
                    <CardContent>
                        <BarChart
                            margin={{ top: 4, right: 8, bottom: 26, left: 34 }}
                            height={260}
                            dataset={distDataset}
                            xAxis={[{
                                dataKey: 'bin',
                                label: 'Grade (0–1)',
                                categoryGapRatio: 0.15,
                                barGapRatio: 0.0,
                                colorMap: { type: 'ordinal', colors: COLORS, values: distDataset.map((d) => d.bin) },
                            }]}
                            series={[{ dataKey: 'n' }]}
                            hideLegend
                            borderRadius={2}
                            grid={{ horizontal: true }}
                        />
                    </CardContent>
                </Card>
                <Card>
                    <CardHeader title={<Typography variant="subtitle1">Pass/Fail Rate</Typography>} subheader={<Typography variant='caption' color='text.secondary'>N = {normalizedGrades.length}</Typography>} />
                    <CardContent>
                        <PieChart series={[{ data: passFailData }]} height={260} />
                    </CardContent>
                </Card>
            </Box>

            {user?.role === 'instructor' && (
                <Box>
                    <Tooltip title='Explore detailed progress & rubric evolution'>
                        <span>
                            <Button variant='outlined' onClick={() => toggleAdvanced(true)}>More Charts</Button>
                        </span>
                    </Tooltip>
                </Box>
            )}

            {/* Common Incorrect Answers */}
            <Card>
                <CardHeader title="Common Incorrect Answers" />
                <CardContent>
                    {commonIncorrect.length ? (
                        <Table size="small">
                            <TableHead>
                                <TableRow>
                                    <TableCell sx={{ fontWeight: 600 }}>Count</TableCell>
                                    <TableCell sx={{ fontWeight: 600 }}>Answer</TableCell>
                                </TableRow>
                            </TableHead>
                            <TableBody>
                                {commonIncorrect.map((item, idx) => (
                                    <TableRow key={idx}>
                                        <TableCell>{item.count}</TableCell>
                                        <TableCell sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>{item.answer}</TableCell>
                                    </TableRow>
                                ))}
                            </TableBody>
                        </Table>
                    ) : (
                        <Typography color="text.secondary">No common incorrect answers available.</Typography>
                    )}
                </CardContent>
            </Card>


        </Box>
    );
}

// Lightweight component to load & present instructor feedback survey analytics inside advanced section
interface InstructorSurveyAnalyticsProps { assignmentId?: string }
function InstructorSurveyAnalytics({ assignmentId }: InstructorSurveyAnalyticsProps) {
    const [data, setData] = React.useState<any | null>(null);
    const [loading, setLoading] = React.useState(false);
    const [error, setError] = React.useState<string | null>(null);
    const [showRaw, setShowRaw] = React.useState(false);
    // Lazy import to avoid circular if AuthProvider changes
    const auth = (typeof window !== 'undefined') ? (window as any).__AUTH : null;
    const token: string | undefined = auth?.user?.token;
    const [showAll, setShowAll] = React.useState(false);
    React.useEffect(() => {
        let cancelled = false;
        async function load() {
            setLoading(true); setError(null);
            try {
                const headers: Record<string, string> = {};
                if (token) headers['Authorization'] = `Bearer ${token}`;
                const url = showAll ? '/api/feedback?aggregate=1' : (assignmentId ? `/api/feedback?aggregate=1&assignmentId=${encodeURIComponent(assignmentId)}` : '/api/feedback?aggregate=1');
                const res = await fetch(url, { headers });
                if (res.status === 401) { setError('Unauthorized'); return; }
                if (!res.ok) throw new Error('fetch failed');
                const json = await res.json();
                if (!cancelled) setData(json);
            } catch (e: any) {
                if (!cancelled) setError(e.message || 'error');
            } finally { if (!cancelled) setLoading(false); }
        }
        load();
        return () => { cancelled = true; };
    }, [token, assignmentId, showAll]);
    if (loading) return <Typography variant='caption' color='text.secondary'>Loading survey analytics…</Typography>;
    if (error) return <Typography variant='caption' color='error'>Survey analytics unavailable: {error}</Typography>;
    if (!data || !data.total_responses) return <Typography variant='caption' color='text.secondary'>No survey responses yet.</Typography>;
    // Prepare distributions for charts
    const helpedRaw = (data.helped_fix_distribution || []).map((d: any) => ({ rating: d.helped_fix as number, n: d.n as number }));
    const understandingRaw = (data.improved_understanding_distribution || []).map((d: any) => ({ rating: d.improved_understanding as number, n: d.n as number }));
    // High-contrast distinct colors (blue, orange, green, purple, red) chosen for differentiation & color-blind friendliness
    const surveyColors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#9467bd', '#d62728'];
    // Convert into single-row dataset with columns value1..value5 so each bar becomes its own series (allowing distinct colors)
    function toSeriesDataset(rows: { rating: number; n: number }[]) {
        const base: any = { bucket: 'ratings' };
        rows.forEach(r => { base['v' + r.rating] = r.n; });
        return [base];
    }
    const helpedDataset = toSeriesDataset(helpedRaw);
    const understandingDataset = toSeriesDataset(understandingRaw);
    const helpedSeries = helpedRaw.map((r: { rating: number; n: number }) => ({ dataKey: 'v' + r.rating, label: String(r.rating), color: surveyColors[(r.rating - 1) % surveyColors.length] }));
    const understandingSeries = understandingRaw.map((r: { rating: number; n: number }) => ({ dataKey: 'v' + r.rating, label: String(r.rating), color: surveyColors[(r.rating - 1) % surveyColors.length] }));
    // Raw responses extraction: expect backend to optionally include `responses` array if aggregate=1
    const rawResponses: any[] = Array.isArray(data.responses) ? data.responses : [];
    const fmt2 = (v: any) => {
        const n = Number(v);
        return isFinite(n) ? n.toFixed(2) : '–';
    };
    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
                <Typography variant='h6'>Survey Analytics</Typography>
                <FormControlLabel
                    control={<Switch size='small' checked={showAll} onChange={(_, v) => setShowAll(v)} />}
                    label={<Typography variant='caption' color='text.secondary'>{showAll ? 'All assignments' : 'Current assignment'}</Typography>}
                />
            </Box>
            <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr', sm: 'repeat(3, 1fr)', md: 'repeat(6, 1fr)' } }}>
                <Card><CardContent><Typography variant='h6'>{data.total_responses}</Typography><Typography variant='caption' color='text.secondary'>Responses</Typography></CardContent></Card>
                <Card><CardContent><Typography variant='h6'>{data.avg_helped_fix ?? '–'}</Typography><Typography variant='caption' color='text.secondary'>Avg Helped Fix</Typography></CardContent></Card>
                <Card><CardContent><Typography variant='h6'>{data.avg_improved_understanding ?? '–'}</Typography><Typography variant='caption' color='text.secondary'>Avg Understanding</Typography></CardContent></Card>
                <Card><CardContent><Typography variant='h6'>{data.avg_improvement != null ? fmt2(data.avg_improvement) : '–'}</Typography><Typography variant='caption' color='text.secondary'>Avg Improvement</Typography></CardContent></Card>
                <Card><CardContent><Typography variant='h6'>{data.percent_improved != null ? data.percent_improved + '%' : '–'}</Typography><Typography variant='caption' color='text.secondary'>% Improved</Typography></CardContent></Card>
                <Card><CardContent><Typography variant='h6'>{data.median_improvement != null ? fmt2(data.median_improvement) : '–'}</Typography><Typography variant='caption' color='text.secondary'>Median Δ</Typography></CardContent></Card>
            </Box>
            <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr', md: '1fr 1fr' } }}>
                <Card>
                    <CardHeader title={<Typography variant='subtitle2'>Helped Fix Distribution</Typography>} />
                    <CardContent>
                        {helpedDataset[0] && helpedSeries.length ? <BarChart height={200} dataset={helpedDataset} xAxis={[{ dataKey: 'bucket', scaleType: 'band', label: 'Helped Fix (1-5)', categoryGapRatio: 0.4 }]} series={helpedSeries} /> : <Typography variant='caption' color='text.secondary'>No data</Typography>}
                    </CardContent>
                </Card>
                <Card>
                    <CardHeader title={<Typography variant='subtitle2'>Understanding Distribution</Typography>} />
                    <CardContent>
                        {understandingDataset[0] && understandingSeries.length ? <BarChart height={200} dataset={understandingDataset} xAxis={[{ dataKey: 'bucket', scaleType: 'band', label: 'Understanding (1-5)', categoryGapRatio: 0.4 }]} series={understandingSeries} /> : <Typography variant='caption' color='text.secondary'>No data</Typography>}
                    </CardContent>
                </Card>
            </Box>
            <Card>
                <CardHeader title={<Typography variant='subtitle2'>Correlations</Typography>} subheader={<Typography variant='caption'>Improvement vs Ratings (Pearson)</Typography>} />
                <CardContent>
                    <Table size='small'>
                        <TableHead><TableRow><TableCell>Metric</TableCell><TableCell>r</TableCell></TableRow></TableHead>
                        <TableBody>
                            <TableRow><TableCell>Improvement ~ Helped Fix</TableCell><TableCell>{data.correlation?.improvement_helped_fix ?? 'n/a'}</TableCell></TableRow>
                            <TableRow><TableCell>Improvement ~ Understanding</TableCell><TableCell>{data.correlation?.improvement_understanding ?? 'n/a'}</TableCell></TableRow>
                        </TableBody>
                    </Table>
                </CardContent>
            </Card>
            <Card>
                <CardHeader title={<Typography variant='subtitle2'>Raw Feedback Responses</Typography>} subheader={<Typography variant='caption'>Individual entries (toggle visibility)</Typography>} action={<Button size='small' onClick={() => setShowRaw(r => !r)}>{showRaw ? 'Hide' : 'Show'}</Button>} />
                <CardContent>
                    {showRaw ? (
                        rawResponses.length ? (
                            <Box sx={{ maxHeight: 300, overflow: 'auto', border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                                <Table size='small' stickyHeader>
                                    <TableHead>
                                        <TableRow>
                                            <TableCell sx={{ fontWeight: 600 }}>#</TableCell>
                                            <TableCell sx={{ fontWeight: 600 }}>Helped Fix</TableCell>
                                            <TableCell sx={{ fontWeight: 600 }}>Understanding</TableCell>
                                            <TableCell sx={{ fontWeight: 600 }}>Improvement</TableCell>
                                            <TableCell sx={{ fontWeight: 600 }}>Comment</TableCell>
                                        </TableRow>
                                    </TableHead>
                                    <TableBody>
                                        {rawResponses.map((r, idx) => (
                                            <TableRow key={idx}>
                                                <TableCell>{idx + 1}</TableCell>
                                                <TableCell>{r.helped_fix ?? '–'}</TableCell>
                                                <TableCell>{r.improved_understanding ?? '–'}</TableCell>
                                                <TableCell>{isFinite(Number(r.improvement)) ? Number(r.improvement).toFixed(2) : '–'}</TableCell>
                                                <TableCell sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word', maxWidth: 260 }}>{(r.comment ?? r.comments ?? '').trim() || <Typography component='span' variant='caption' color='text.secondary'>—</Typography>}</TableCell>
                                            </TableRow>
                                        ))}
                                    </TableBody>
                                </Table>
                            </Box>
                        ) : <Typography variant='caption' color='text.secondary'>Backend did not include raw responses.</Typography>
                    ) : <Typography variant='caption' color='text.secondary'>Hidden (click Show to expand).</Typography>}
                </CardContent>
            </Card>
        </Box>
    );
}
