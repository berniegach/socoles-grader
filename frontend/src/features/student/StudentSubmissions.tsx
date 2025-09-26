'use client';
import {
    Card, CardHeader, CardContent, Typography, Table, TableHead, TableRow, TableCell, TableBody, Button, Chip,
    Collapse, Box, LinearProgress
} from '@mui/material';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
import PageCard from '@/components/PageCard';
import VisibilityIcon from '@mui/icons-material/Visibility';
import DeleteIcon from '@mui/icons-material/Delete';
import { useAuth } from '@/features/auth/AuthProvider';
import { useState, useEffect, Fragment } from 'react';

export default function StudentSubmissions({ rows = [] as any[], onRefresh }: { rows?: any[]; onRefresh?: () => void }) {
    const [expandedId, setExpandedId] = useState<string | null>(null);
    const [loadingQs, setLoadingQs] = useState<Record<string, boolean>>({});
    const [questionsBySubmission, setQuestionsBySubmission] = useState<Record<string, any[]>>({});
    const [localRows, setLocalRows] = useState<any[]>(rows);
    const { authFetch, user } = useAuth();

    const formatGrade = (g: number | null | undefined) => (typeof g === 'number' && isFinite(g) ? g.toFixed(2) : '—');

    useEffect(() => { setLocalRows(rows); }, [rows]);

    async function loadQuestions(submission: any) {
        if (questionsBySubmission[submission.id]) { return; }
        setLoadingQs(l => ({ ...l, [submission.id]: true }));
        try {
            const res = await authFetch(`/api/question-submissions?submissionId=${submission.id}`);
            const data = await res.json();
            const enriched = await Promise.all((data || []).map(async (qs: any) => {
                try {
                    const qRes = await authFetch(`/api/questions/${qs.questionId}`);
                    if (qRes.ok) { const qDetail = await qRes.json(); return { ...qs, prompt: qDetail.prompt }; }
                } catch { /* ignore */ }
                return qs;
            }));
            setQuestionsBySubmission(m => ({ ...m, [submission.id]: enriched }));
        } catch { /* ignore */ } finally { setLoadingQs(l => ({ ...l, [submission.id]: false })); }
    }

    async function handleDelete(id: string) {
        const target = localRows.find(r => r.id === id);
        if (!target) return;
        if (target.status === 'Submitted') return; // safeguard
        if (!confirm('Delete this in-progress submission?')) return;
        try {
            const res = await authFetch(`/api/submissions?id=${id}`, { method: 'DELETE' });
            if (res.ok) {
                setLocalRows(r => r.filter(s => s.id !== id));
                if (onRefresh) onRefresh();
                if (expandedId === id) setExpandedId(null); // close if deleted
            } else {
                const d = await res.json();
                alert(d?.error || 'Failed to delete');
            }
        } catch { /* ignore */ }
    }

    return (
        <PageCard>
            <Card>
                <CardHeader title={<Typography variant="subtitle1">Your Submissions</Typography>} />
                <CardContent>
                    <Table size="small">
                        <TableHead>
                            <TableRow>
                                <TableCell>Assignment</TableCell>
                                <TableCell>Date</TableCell>
                                <TableCell>Grade</TableCell>
                                <TableCell>Status</TableCell>
                                <TableCell align="right">Actions</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {localRows.map((s: any) => (
                                <Fragment key={s.id}>
                                    <TableRow key={s.id}>
                                        <TableCell>{s.assignment}</TableCell>
                                        <TableCell>{formatDateTimeDDMMYYYYHHmm(s.date)}</TableCell>
                                        <TableCell>{formatGrade(s.grade as any)}</TableCell>
                                        <TableCell>
                                            <Chip
                                                size="small"
                                                label={s.status}
                                                color={s.status === 'Auto-graded' ? 'primary' : 'default'}
                                                variant={s.status === 'Auto-graded' ? 'outlined' : 'filled'}
                                            />
                                        </TableCell>
                                        <TableCell align="right" style={{ whiteSpace: 'nowrap' }}>
                                            <Button
                                                size="small"
                                                variant="outlined"
                                                startIcon={<VisibilityIcon />}
                                                onClick={() => {
                                                    setExpandedId(curr => {
                                                        if (curr === s.id) return null; // toggle closed
                                                        // opening new one: load questions if not loaded
                                                        if (!questionsBySubmission[s.id]) loadQuestions(s);
                                                        return s.id;
                                                    });
                                                }}
                                                sx={{ mr: 1 }}
                                            >{expandedId === s.id ? 'Hide' : 'View'}</Button>
                                            {s.status === 'In Progress' && (
                                                <Button size="small" color="error" variant="outlined" startIcon={<DeleteIcon />} onClick={() => handleDelete(s.id)}>Delete</Button>
                                            )}
                                        </TableCell>
                                    </TableRow>
                                    <TableRow key={s.id + '-detail'}>
                                        <TableCell colSpan={5} sx={{ p: 0, border: 'none' }}>
                                            <Collapse in={expandedId === s.id} timeout="auto" unmountOnExit>
                                                <Box sx={{ p: 2, bgcolor: 'background.default', borderTop: '1px solid', borderColor: 'divider' }}>
                                                    {loadingQs[s.id] && <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}><LinearProgress sx={{ flex: 1 }} /><Typography variant='caption'>Loading answers…</Typography></Box>}
                                                    {!loadingQs[s.id] && !questionsBySubmission[s.id]?.length && (
                                                        <Typography variant='body2' color='text.secondary'>No per-question answers stored for this submission.</Typography>
                                                    )}
                                                    {!!questionsBySubmission[s.id]?.length && questionsBySubmission[s.id].map(qs => (
                                                        <Box key={qs.id} sx={{ mb: 2, p: 1.25, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                                                            <Typography variant='subtitle2' sx={{ mb: .5 }}>{qs.title || qs.questionId}</Typography>
                                                            {qs.prompt && <Typography variant='caption' sx={{ display: 'block', mb: 1, whiteSpace: 'pre-line' }}>Prompt:\n{qs.prompt}</Typography>}
                                                            <Typography variant='overline' sx={{ opacity: .7 }}>Your Answer</Typography>
                                                            <Box sx={{ mt: .5, mb: 1 }}>
                                                                <Typography variant='body2' sx={{ fontFamily: 'ui-monospace,monospace', whiteSpace: 'pre-wrap' }}>{qs.sql || '-- (no SQL captured)'}</Typography>
                                                            </Box>
                                                            {/* Feedback as structured list with preserved line breaks */}
                                                            <Box sx={{ mt: .5 }}>
                                                                <Typography variant='overline' sx={{ opacity: .7 }}>Feedback</Typography>
                                                                {Array.isArray(qs.feedback) && qs.feedback.length > 0 ? (
                                                                    <Box component='ul' sx={{ listStyle: 'none', pl: 2.25, my: .5 }}>
                                                                        {qs.feedback.map((f: string, i: number) => (
                                                                            <li key={i}>
                                                                                <Typography variant='body2' sx={{ whiteSpace: 'pre-line' }}>{f}</Typography>
                                                                            </li>
                                                                        ))}
                                                                    </Box>
                                                                ) : (
                                                                    <Typography variant='caption' color='text.secondary'>No feedback.</Typography>
                                                                )}
                                                            </Box>
                                                            <Typography variant='caption' color='text.secondary'>Status: {qs.status}{qs.grade != null && ` • Grade: ${formatGrade(qs.grade as any)}`}</Typography>
                                                        </Box>
                                                    ))}
                                                </Box>
                                            </Collapse>
                                        </TableCell>
                                    </TableRow>
                                </Fragment>
                            ))}
                            {!localRows.length && (
                                <TableRow><TableCell colSpan={5}><Typography color="text.secondary">No submissions yet.</Typography></TableCell></TableRow>
                            )}
                        </TableBody>
                    </Table>
                </CardContent>
            </Card>
        </PageCard>
    );
}
