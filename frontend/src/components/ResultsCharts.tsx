'use client';
import React from 'react';
import { Box, Card, CardContent, CardHeader, Typography, Table, TableBody, TableCell, TableHead, TableRow, Button, Tooltip, Divider } from '@mui/material';
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

interface ResultsChartsProps { results?: any[]; loadAttemptsFor?: { assignmentId: string; student: string; token?: string }; }
export default function ResultsCharts({ results = [] as any[], loadAttemptsFor }: ResultsChartsProps) {
    const theme = useTheme();
    const [showAdvanced, setShowAdvanced] = React.useState(false);
    const [attemptAugmented, setAttemptAugmented] = React.useState<any[] | null>(null);
    const [loadingAttempts, setLoadingAttempts] = React.useState(false);

    // Auto-fetch attempts if loadAttemptsFor provided; this runs client-side with token via fetch API.
    React.useEffect(() => {
        async function load() {
            if (!loadAttemptsFor) { setAttemptAugmented(null); return; }
            const { assignmentId, student, token } = loadAttemptsFor;
            if (!assignmentId || !student) return;
            setLoadingAttempts(true);
            try {
                // 1. Get current question submissions for this assignment+student
                const baseHeaders: Record<string, string> = { 'Content-Type': 'application/json' };
                if (token) baseHeaders['Authorization'] = `Bearer ${token}`;
                const qsRes = await fetch(`/api/question-submissions?assignmentId=${encodeURIComponent(assignmentId)}&student=${encodeURIComponent(student)}`, { headers: baseHeaders });
                if (!qsRes.ok) throw new Error('failed question submissions');
                const qs = await qsRes.json();
                const enriched: any[] = [];
                // 2. For each question submission, fetch history attempts
                for (const q of qs) {
                    if (!q.id) continue;
                    try {
                        const histRes = await fetch(`/api/question-submissions?historyOf=${encodeURIComponent(q.id)}`, { headers: baseHeaders });
                        if (!histRes.ok) continue;
                        const attempts = await histRes.json();
                        // normalize attempts objects to expected shape
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
    }, [loadAttemptsFor?.assignmentId, loadAttemptsFor?.student, loadAttemptsFor?.token]);
    // Normalize grades 0..1 and compute aggregates
    const dist = Array.from({ length: 11 }, (_, i) => ({ bin: (i / 10).toFixed(1), n: 0 })); // bins labeled 0.0..1.0
    const normalizedGrades: number[] = [];
    const answers: string[] = [];

    for (const r of results) {
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
    const totalAnswers = results.length;
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
    results.forEach((r) => {
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
    // Expect (optionally) each result to have attempts: [{attempt, grade, ts, rubric:{syntax,semantics,results}, feedback:[] }]
    type Attempt = { attempt: number; grade: number; ts?: string; rubric?: { syntax?: number; semantics?: number; results?: number }; feedback?: string[] };
    interface ResultWithAttempts { attempts?: Attempt[] }
    const allAttempts: Attempt[] = [];
    const baseResults = attemptAugmented || results;
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
        } else {
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
        if (!attemptAugmented) return null;
        const perQuestion = attemptAugmented.map(q => ({ id: q.id, attempts: Array.isArray(q.attempts) ? q.attempts.length : 0 }));
        const totalHist = perQuestion.reduce((a, b) => a + b.attempts, 0);
        const attemptKeys = progressDataset.map(p => p.attempt);
        return { perQuestion, totalHist, attemptKeys };
    }, [attemptAugmented, progressDataset]);

    if (showAdvanced) {
        return (
            <Box sx={{ display: 'grid', gap: 2 }}>
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                    <Typography variant='h6'>Advanced Progress Charts</Typography>
                    <Button size='small' onClick={() => setShowAdvanced(false)}>Back</Button>
                </Box>
                {loadingAttempts && <Typography variant='caption' color='text.secondary'>Loading attempts…</Typography>}
                <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr', lg: '1fr 1fr' } }}>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Attempt Progress</Typography>} subheader={<Typography variant='caption'>Overall + rubric lines</Typography>} />
                        <CardContent>
                            {progressDataset.length ? (
                                <LineChart height={260} dataset={progressDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} series={[
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
                                <BarChart height={260} dataset={deltaDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} series={[
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
                                <ScatterChart height={260} series={[{ data: timeGapDataset.map(p => ({ x: p.gapMinutes, y: p.improvement })) }]} xAxis={[{ label: 'Gap (min)' }]} yAxis={[{ label: 'Grade Δ' }]} />
                            ) : <Typography variant='caption' color='text.secondary'>Timestamp data unavailable.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>First vs Best (Comparison)</Typography>} />
                        <CardContent>
                            {radarData.length ? (
                                <BarChart height={260}
                                    dataset={radarData}
                                    xAxis={[{ dataKey: 'axis', scaleType: 'band' }]}
                                    series={[
                                        { dataKey: 'first', label: 'First', color: theme.palette.error.light },
                                        { dataKey: 'best', label: 'Best', color: theme.palette.success.main },
                                    ]}
                                />
                            ) : <Typography variant='caption' color='text.secondary'>Rubric data insufficient.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Cumulative Mastery</Typography>} subheader={<Typography variant='caption'>Best grade so far</Typography>} />
                        <CardContent>
                            {cumulDataset.length ? (
                                <LineChart height={260} dataset={cumulDataset} xAxis={[{ dataKey: 'attempt', label: 'Attempt' }]} series={[{ dataKey: 'bestSoFar', label: 'Best so far', color: theme.palette.primary.main }]} />
                            ) : <Typography variant='caption' color='text.secondary'>No attempts.</Typography>}
                        </CardContent>
                    </Card>
                    <Card>
                        <CardHeader title={<Typography variant='subtitle1'>Question Difficulty</Typography>} subheader={<Typography variant='caption'>Avg attempts vs best grade</Typography>} />
                        <CardContent>
                            {Array.isArray(attemptAugmented) && attemptAugmented.length ? (() => {
                                const qMetrics = attemptAugmented.map(q => {
                                    const attempts = Array.isArray(q.attempts) ? q.attempts : [];
                                    const first = attempts[0];
                                    const best = attempts.reduce((acc: any, a: any) => (a.grade ?? 0) > (acc?.grade ?? -1) ? a : acc, first || null);
                                    return { attemptsCount: attempts.length, best: best?.grade ?? 0 };
                                });
                                const scatter = qMetrics.map(m => ({ x: m.attemptsCount, y: m.best }));
                                return <ScatterChart height={200} series={[{ data: scatter.map(p => ({ x: p.x, y: p.y })) }]} xAxis={[{ label: 'Attempts' }]} yAxis={[{ label: 'Best Grade' }]} />;
                            })() : <Typography variant='caption' color='text.secondary'>Need attempt data.</Typography>}
                        </CardContent>
                    </Card>
                </Box>
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
                    <CardHeader title={<Typography variant="subtitle1">Grade Distribution (0–1)</Typography>} />
                    <CardContent>
                        <BarChart
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
                    <CardHeader title={<Typography variant="subtitle1">Pass/Fail Rate</Typography>} />
                    <CardContent>
                        <PieChart series={[{ data: passFailData }]} height={260} />
                    </CardContent>
                </Card>
            </Box>

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

            <Box>
                <Divider sx={{ my: 1 }} />
                <Tooltip title='Explore detailed progress & rubric evolution'>
                    <Button variant='outlined' onClick={() => setShowAdvanced(true)}>More Charts</Button>
                </Tooltip>
            </Box>
        </Box>
    );
}
