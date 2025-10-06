'use client';
import { Card, CardContent, CardHeader, Table, TableBody, TableCell, TableHead, TableRow, Button, Typography, Collapse, Box, CircularProgress, Chip, Divider, TextField, CardActionArea, Tooltip } from '@mui/material';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import { useEffect, useMemo, useState } from 'react';
import type { SubmissionWithQuestions, QuestionSubmission, QuestionSubmissionAttempt, Assignment } from '@/lib/types';
import { DEFAULT_GRADING_OPTIONS } from '@/lib/api';


import { useAuth } from '@/features/auth/AuthProvider';
import { useRosterMap } from '@/lib/useRosterMap';
import PageCard from '@/components/PageCard';
import TilesGrid from '@/components/TilesGrid';
import DifficultyChip from '@/components/DifficultyChip';
import RefreshIcon from '@mui/icons-material/Refresh';
import CloseFullscreenIcon from '@mui/icons-material/CloseFullscreen';
import HeaderActionButton from '@/components/HeaderActionButton';
import HeaderActions from '@/components/HeaderActions';
import ResultsCharts from '@/components/ResultsCharts';
import InstructorSubmissionPlayer from '@/features/instructor/InstructorSubmissionPlayer';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
export default function SubmissionReview() {
    // Master: assignments (quizzes)
    const [assignments, setAssignments] = useState<Assignment[]>([]);
    const [aLoading, setALoading] = useState(false);
    const [aError, setAError] = useState<string | null>(null);
    const [selectedAssignment, setSelectedAssignment] = useState<Assignment | null>(null);
    // Submissions count per assignment (for the cards view)
    const [submissionCounts, setSubmissionCounts] = useState<Record<string, number>>({});

    // Detail: submissions for selected assignment
    const [rows, setRows] = useState<SubmissionWithQuestions[]>([]);
    const [loading, setLoading] = useState(false);
    const { user, authFetch } = useAuth();
    const rosterMap = useRosterMap();
    // Selected submission (detail view)
    const [selectedSubmissionId, setSelectedSubmissionId] = useState<string | null>(null);
    const [grading, setGrading] = useState<Record<string, { status: 'idle' | 'loading' | 'success' | 'error'; message?: string }>>({});
    const [gradingSub, setGradingSub] = useState<Record<string, { status: 'idle' | 'loading' | 'success' | 'error'; message?: string }>>({});
    // per-question attempt history state
    const [historyByQId, setHistoryByQId] = useState<Record<string, { open: boolean; loading: boolean; attempts: QuestionSubmissionAttempt[]; error?: string }>>({});
    const [search, setSearch] = useState('');
    const [chartsAdvancedOpen, setChartsAdvancedOpen] = useState(false);

    // Helper to format grades consistently
    const formatGrade = (g: number | null | undefined) => (typeof g === 'number' && isFinite(g) ? g.toFixed(2) : '—');

    // Load assignments initially
    useEffect(() => {
        async function loadAssignments() {
            setALoading(true); setAError(null);
            if (!user?.token) { setALoading(false); return; }
            try {
                const res = await authFetch('/api/assignments');
                if (!res.ok) throw new Error(`Failed (${res.status})`);
                const data = await res.json();
                setAssignments(Array.isArray(data) ? data : []);
            } catch (e: any) { setAError(e?.message || 'Failed to load quizzes'); } finally { setALoading(false); }
        }
        void loadAssignments();
    }, [user?.token, authFetch]);

    // Load all submissions once to compute counts for each assignment card
    useEffect(() => {
        async function loadSubmissionCounts() {
            if (!user?.token) return;
            try {
                const res = await authFetch('/api/submissions');
                const data = await res.json();
                const arr: any[] = Array.isArray(data) ? data : [];
                const counts: Record<string, number> = {};
                for (const s of arr) {
                    const key = s.assignment;
                    if (!key) continue;
                    counts[key] = (counts[key] || 0) + 1;
                }
                setSubmissionCounts(counts);
            } catch {
                setSubmissionCounts({});
            }
        }
        // Only needed on the assignments list view
        if (!selectedAssignment) void loadSubmissionCounts();
    }, [user?.token, authFetch, selectedAssignment]);

    // Load submissions for selected assignment
    useEffect(() => {
        async function loadForAssignment(title: string) {
            setLoading(true);
            if (!user?.token) { setLoading(false); return; }
            try {
                const res = await authFetch('/api/submissions');
                const data = await res.json();
                const arr: SubmissionWithQuestions[] = Array.isArray(data) ? data : [];
                setRows(arr.filter((s: any) => s.assignment === title));
            } catch { setRows([] as any); } finally { setLoading(false); }
        }
        if (selectedAssignment) { setSelectedSubmissionId(null); void loadForAssignment(selectedAssignment.title); }
        else { setRows([] as any); setSelectedSubmissionId(null); }
    }, [selectedAssignment?.title, user?.token, authFetch]);
    async function loadQuestions(submissionId: string) {
        try {
            const res = await authFetch(`/api/question-submissions?submissionId=${submissionId}`);
            const data = await res.json();
            setRows(r => r.map(s => s.id === submissionId ? { ...s, questions: data } : s));
            // After loading questions, recompute parent submission status/grade
            await updateParentSubmission(submissionId);
        } catch { /* ignore */ }
    }

    // When entering a submission detail view, ensure its questions are loaded
    useEffect(() => {
        if (!selectedSubmissionId) return;
        const s = rows.find(r => r.id === selectedSubmissionId) as any;
        if (s && !Array.isArray(s.questions)) {
            void loadQuestions(selectedSubmissionId);
        }
    }, [selectedSubmissionId, rows]);

    // Helper to recompute and update parent submission status/grade after grading
    async function updateParentSubmission(submissionId: string) {
        try {
            const qsRes = await authFetch(`/api/question-submissions?submissionId=${submissionId}`);
            if (!qsRes.ok) return;
            const qs = await qsRes.json();
            const graded = (qs || []).filter((q: any) => typeof q.grade === 'number');
            if (!graded.length) return; // don't switch status unless some grading exists
            // Compute SUM of numeric grades (not average)
            const total = graded.reduce((sum: number, q: any) => sum + (q.grade || 0), 0);
            const patch = await authFetch('/api/submissions', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: submissionId, grade: total, status: 'Auto-graded' }) });
            if (!patch.ok) return;
            const updated = await patch.json();
            setRows(r => r.map((s: any) => s.id === submissionId ? { ...s, grade: updated.grade, status: updated.status } : s));
        } catch { /* ignore */ }
    }

    // NEW: load attempt history for a given question submission id
    async function loadAttemptHistory(qsId: string) {
        setHistoryByQId(m => ({ ...m, [qsId]: { ...(m[qsId] || { open: true, attempts: [] }), loading: true, error: undefined } }));
        try {
            const res = await authFetch(`/api/question-submissions?historyOf=${qsId}`);
            if (!res.ok) throw new Error('Failed to load history');
            const attempts: QuestionSubmissionAttempt[] = await res.json();
            setHistoryByQId(m => ({ ...m, [qsId]: { open: true, loading: false, attempts } }));
        } catch (e: any) {
            setHistoryByQId(m => ({ ...m, [qsId]: { open: true, loading: false, attempts: [], error: e?.message || 'Failed to load history' } }));
        }
    }

    // Filtered list of assignments by search
    const filteredAssignments = useMemo(() => {
        const q = search.trim().toLowerCase();
        if (!q) return assignments;
        return assignments.filter(a => a.title.toLowerCase().includes(q) || a.course.toLowerCase().includes(q));
    }, [assignments, search]);

    return (
        <PageCard
            headerTitle={<Typography variant='subtitle1'>{selectedAssignment ? 'Submissions' : 'Submissions'}{selectedSubmissionId ? '' : ''}</Typography>}
            headerProps={{ height: 56 }}
            headerActionsVariant='plain'
            headerActions={
                <HeaderActions
                    actions={[
                        selectedSubmissionId ? {
                            key: 'close-viewer',
                            label: 'Close viewer',
                            ariaLabel: 'Close viewer',
                            onClick: () => setSelectedSubmissionId(null),
                            icon: <CloseFullscreenIcon fontSize='small' />,
                        } : null,
                        selectedAssignment && !selectedSubmissionId ? {
                            key: 'back',
                            label: 'Back to quizzes',
                            ariaLabel: 'Back to quizzes',
                            onClick: () => setSelectedAssignment(null),
                            icon: <ArrowBackIcon fontSize='small' />,
                        } : null,
                        {
                            key: 'refresh',
                            label: 'Refresh',
                            ariaLabel: 'Refresh',
                            onClick: () => { if (!selectedAssignment) { setSelectedAssignment(null); } else { setSelectedSubmissionId(null); } },
                            icon: <RefreshIcon fontSize='small' />,
                        }
                    ].filter(Boolean) as any}
                />
            }
        >
            {!selectedAssignment ? (
                <CardContent>
                    <Box sx={{ display: 'flex', gap: 1, mb: 2, flexWrap: 'wrap', alignItems: 'center' }}>
                        <TextField variant='outlined' size='small' placeholder='Search by title or course' value={search} onChange={e => setSearch(e.target.value)} />
                        {aError && <Typography variant='caption' color='error'>{aError}</Typography>}
                    </Box>
                    <TilesGrid>
                        {filteredAssignments.map(a => (
                            <Card key={a.id} variant='outlined' sx={{ display: 'flex', flexDirection: 'column', position: 'relative' }}>
                                <CardActionArea onClick={() => setSelectedAssignment(a)} sx={{ alignSelf: 'stretch' }}>
                                    <CardHeader title={<Typography variant='subtitle1' sx={{ fontWeight: 600, lineHeight: 1.2 }}>{a.title}</Typography>} subheader={<Typography variant='caption' color='text.secondary'>{a.course}</Typography>} sx={{ pb: 0 }} />
                                    <CardContent sx={{ pt: 0, display: 'grid', gap: 1 }}>
                                        <Box sx={{ display: 'flex', gap: .75, flexWrap: 'wrap' }}>
                                            <DifficultyChip value={a.difficulty} />
                                            <Chip size='small' variant='outlined' label={`${a.points} pts`} />
                                            <Chip size='small' variant='outlined' label={`Subs: ${submissionCounts[a.title] || 0}`} />
                                        </Box>
                                        <Typography variant='caption' color='text.secondary'>Due: {a.due || '—'}</Typography>
                                    </CardContent>
                                </CardActionArea>
                            </Card>
                        ))}
                    </TilesGrid>
                    {!filteredAssignments.length && !aLoading && (
                        <Typography variant='body2' color='text.secondary' sx={{ mt: 2 }}>No quizzes found.</Typography>
                    )}
                    {aLoading && (
                        <Typography variant='body2' color='text.secondary' sx={{ mt: 2 }}>Loading…</Typography>
                    )}
                </CardContent>
            ) : (
                <CardContent>
                    <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mb: 1 }}>{selectedAssignment.course} • Due {selectedAssignment.due} • Attempts {selectedAssignment.attemptsAllowed}</Typography>
                    {!selectedSubmissionId && (
                        <>
                            <Box sx={{ mb: 2 }}>
                                <Typography variant='subtitle1' sx={{ mb: 1 }}>Assignment Overview</Typography>
                                <ResultsCharts
                                    results={rows.map(r => ({ Grade: (r as any).grade ?? 0 }))}
                                    loadAttemptsFor={selectedAssignment ? { assignmentId: selectedAssignment.id, student: (rows[0]?.student || ''), token: user?.token } : undefined}
                                    onAdvancedChange={(o) => setChartsAdvancedOpen(o)}
                                />
                            </Box>
                            {!chartsAdvancedOpen && <Divider sx={{ my: 2 }} />}
                            {!chartsAdvancedOpen && (
                                <Table size="small">
                                    <TableHead>
                                        <TableRow>
                                            <TableCell>Student</TableCell>
                                            <TableCell>Date</TableCell>
                                            <TableCell>Grade</TableCell>
                                            <TableCell>Status</TableCell>
                                            <TableCell>Actions</TableCell>
                                        </TableRow>
                                    </TableHead>
                                    <TableBody>
                                        {rows.map((s: SubmissionWithQuestions) => (
                                            <TableRow key={s.id} hover sx={{ cursor: 'pointer' }} onClick={() => {
                                                setSelectedSubmissionId(s.id);
                                                if (!(s as any).questions) void loadQuestions(s.id);
                                            }}>
                                                <TableCell>{rosterMap[s.student] || s.student}</TableCell>
                                                <TableCell>{formatDateTimeDDMMYYYYHHmm(s.date)}</TableCell>
                                                <TableCell>{formatGrade((s as any).grade)}</TableCell>
                                                <TableCell>{s.status}</TableCell>
                                                <TableCell>
                                                    <Button size='small' sx={{ ml: 0 }} variant='contained' disabled={gradingSub[s.id]?.status === 'loading'} onClick={async (e) => {
                                                        e.stopPropagation();
                                                        setGradingSub(m => ({ ...m, [s.id]: { status: 'loading', message: 'Starting grader…' } }));
                                                        try {
                                                            const start = await authFetch('/api/grade-submission', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ submissionId: s.id, async: true }) });
                                                            if (!start.ok) throw new Error('Failed to start grading');
                                                            const started = await start.json();
                                                            const jobId: string | undefined = started?.jobId;
                                                            if (!jobId) throw new Error('No job id');
                                                            const deadline = Date.now() + 120_000;
                                                            let final: any = null;
                                                            while (Date.now() < deadline) {
                                                                await new Promise(r => setTimeout(r, 1000));
                                                                const poll = await authFetch(`/api/grade-submission?job=${encodeURIComponent(jobId)}`);
                                                                if (!poll.ok) continue;
                                                                const body = await poll.json();
                                                                if (body.status === 'succeeded') { final = body.result; break; }
                                                                if (body.status === 'failed') { throw new Error(body.error || 'Grading failed'); }
                                                                setGradingSub(m => ({ ...m, [s.id]: { status: 'loading', message: 'Grading in progress…' } }));
                                                            }
                                                            if (!final) {
                                                                setGradingSub(m => ({ ...m, [s.id]: { status: 'success', message: 'Still processing in background…' } }));
                                                                return;
                                                            }
                                                            setRows(r => r.map(row => row.id === s.id ? { ...row, grade: (final as any)?.submission?.grade, status: (final as any)?.status || (row as any).status } as any : row));
                                                            await loadQuestions(s.id);
                                                            setGradingSub(m => ({ ...m, [s.id]: { status: 'success', message: (final as any)?.status === 'Needs review' ? 'Completed with issues.' : 'Auto-grading complete.' } }));
                                                        } catch (e: any) {
                                                            setGradingSub(m => ({ ...m, [s.id]: { status: 'error', message: e?.message || 'Failed' } }));
                                                        }
                                                    }}>Auto-Grade</Button>
                                                    {gradingSub[s.id]?.status !== 'idle' && <Typography variant='caption' color={gradingSub[s.id]?.status === 'error' ? 'error' : 'text.secondary'} sx={{ ml: 1 }}>{gradingSub[s.id]?.message}</Typography>}
                                                </TableCell>
                                            </TableRow>
                                        ))}
                                        {!rows.length && !loading && (
                                            <TableRow><TableCell colSpan={5}>No submissions for this quiz.</TableCell></TableRow>
                                        )}
                                        {loading && (
                                            <TableRow><TableCell colSpan={5}>Loading…</TableCell></TableRow>
                                        )}
                                    </TableBody>
                                </Table>)}
                        </>
                    )}
                    {selectedSubmissionId && (() => {
                        const s = rows.find(r => r.id === selectedSubmissionId) as SubmissionWithQuestions | undefined;
                        if (!s) return <Typography variant='body2'>Submission not found.</Typography>;
                        return (
                            <InstructorSubmissionPlayer
                                assignmentId={selectedAssignment?.id || null}
                                submission={s as any}
                                onClose={() => setSelectedSubmissionId(null)}
                            />
                        );
                    })()}
                </CardContent>
            )}
        </PageCard>
    );
}