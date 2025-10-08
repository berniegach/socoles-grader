"use client";
import { useEffect, useMemo, useState, useRef } from 'react';
import { Box, Card, CardHeader, CardContent, CardActions, Typography, Button, Chip, Tabs, Tab, Divider, CircularProgress } from '@mui/material';
import SchemaPreview from '@/components/SchemaPreview';
import PromptWithHint from '@/components/PromptWithHint';
import SqlEditor from '@/components/SqlEditor';
import RubricBars from '@/components/RubricBars';
import FeedbackList from '@/components/FeedbackList';
import GradeDisplay from '@/components/GradeDisplay';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import DifficultyChip from '@/components/DifficultyChip';
import type { AssignmentWithQuestions, QuestionSubmission, QuestionSubmissionAttempt } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';
import { useRosterMap } from '@/lib/useRosterMap';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';

interface Props {
    assignmentId: string | null;
    submission: { id: string; student: string } & Record<string, any>;
    onClose: () => void; // parent already handles close; no extra PageCard wrapper here
    initialQuestionId?: string; // optional deep link
}

export default function InstructorSubmissionPlayer({ assignmentId, submission, onClose, initialQuestionId }: Props) {
    const { authFetch, user } = useAuth();
    const rosterMap = useRosterMap();
    const [assignment, setAssignment] = useState<AssignmentWithQuestions | null>(null);
    const [tab, setTab] = useState(1); // 0 schema, 1 editor (read-only)
    const [activeIdx, setActiveIdx] = useState(0);
    const [qDetail, setQDetail] = useState<Record<string, any>>({}); // questionId -> detail (prompt, hints, initSql)
    const [loadingQ, setLoadingQ] = useState<Record<string, boolean>>({});
    const [historyMap, setHistoryMap] = useState<Record<string, { loading: boolean; attempts: QuestionSubmissionAttempt[]; error?: string }>>({});
    const [attemptTab, setAttemptTab] = useState(0); // index into attempts array (sorted by attempt)

    const questions: QuestionSubmission[] = useMemo(() => Array.isArray((submission as any).questions) ? (submission as any).questions : [], [submission]);

    // Load assignment with questions for metadata and ordering
    useEffect(() => {
        let cancelled = false;
        async function load() {
            if (!assignmentId || !user?.token) return;
            try {
                const res = await authFetch('/api/assignments?include=questions');
                if (!res.ok) return;
                const data = await res.json();
                const found = (Array.isArray(data) ? data : []).find((a: any) => a.id === assignmentId) || null;
                if (!cancelled) setAssignment(found);
            } catch { /* ignore */ }
        }
        load();
        return () => { cancelled = true; };
    }, [assignmentId, user?.token, authFetch]);

    const orderedQuestionIds = useMemo(() => {
        // Order by assignment.links positions when available, else keep submission order
        const fromAssign = assignment?.questions?.slice().sort((a, b) => a.position - b.position).map(q => q.id) || [];
        if (fromAssign.length) return fromAssign;
        return questions.map(q => q.questionId);
    }, [assignment?.questions, questions]);

    // Apply initialQuestionId once
    const appliedInitial = useRef(false);
    useEffect(() => {
        if (appliedInitial.current) return;
        if (!initialQuestionId) return;
        const idx = orderedQuestionIds.indexOf(initialQuestionId);
        if (idx >= 0) { setActiveIdx(idx); appliedInitial.current = true; }
    }, [initialQuestionId, orderedQuestionIds]);


    const activeQuestion = useMemo(() => {
        const qid = orderedQuestionIds[activeIdx];
        return questions.find(q => q.questionId === qid) || questions[activeIdx] || null;
    }, [orderedQuestionIds, activeIdx, questions]);

    // Load question detail for schema/prompt/hints on demand
    useEffect(() => {
        const qid = activeQuestion?.questionId;
        if (!qid || qDetail[qid] || !user?.token) return;
        setLoadingQ(m => ({ ...m, [qid]: true }));
        (async () => {
            try {
                const res = await authFetch(`/api/questions/${qid}`);
                const d = res.ok ? await res.json() : null;
                setQDetail(m => ({ ...m, [qid]: d || {} }));
            } catch { /* ignore */ }
            finally { setLoadingQ(m => ({ ...m, [qid]: false })); }
        })();
    }, [activeQuestion?.questionId, user?.token, authFetch, qDetail]);

    // Load attempts automatically when active question changes
    useEffect(() => {
        const q = activeQuestion;
        if (!q?.id || !user?.token) return;
        const qid = q.questionId;
        if (historyMap[qid]?.attempts?.length) {
            // default to final attempt when switching questions
            const len = historyMap[qid].attempts.length;
            setAttemptTab(len ? len - 1 : 0);
            return;
        }
        setHistoryMap(m => ({ ...m, [qid]: { ...(m[qid] || { attempts: [] }), loading: true, error: undefined } }));
        (async () => {
            try {
                const res = await authFetch(`/api/question-submissions?historyOf=${q.id}`);
                if (!res.ok) throw new Error('Failed to load history');
                const attempts: QuestionSubmissionAttempt[] = await res.json();
                setHistoryMap(m => ({ ...m, [qid]: { loading: false, attempts } }));
                // default to final attempt on initial load
                setAttemptTab(attempts.length ? attempts.length - 1 : 0);
            } catch (e: any) {
                setHistoryMap(m => ({ ...m, [qid]: { loading: false, attempts: [], error: e?.message || 'Failed' } }));
                setAttemptTab(0);
            }
        })();
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [activeQuestion?.id, activeQuestion?.questionId, user?.token, authFetch]);

    // Derive current attempts for the active question and the selected tab
    const { attemptsSorted, currentAttempt, attemptsLoading } = useMemo(() => {
        const qid = activeQuestion?.questionId || '';
        const state = historyMap[qid];
        const attempts = state?.attempts ? [...state.attempts].sort((a, b) => a.attempt - b.attempt) : [];
        const idx = Math.min(Math.max(0, attemptTab), Math.max(0, attempts.length - 1));
        return { attemptsSorted: attempts, currentAttempt: attempts[idx] || null, attemptsLoading: !!state?.loading };
    }, [historyMap, activeQuestion?.questionId, attemptTab]);

    const total = orderedQuestionIds.length || questions.length || 0;
    const idx = Math.min(Math.max(activeIdx, 0), Math.max(0, total - 1));

    return (
        <Card sx={{ display: 'flex', flexDirection: 'column' }}>
            <CardHeader
                title={<Typography variant='subtitle1'>{assignment?.title || 'Assignment'}</Typography>}
                subheader={<Typography variant='caption' color='text.secondary'>Question {idx + 1} of {total} • {rosterMap[submission.student] || submission.student}</Typography>}
            />
            <CardContent sx={{ pt: 0, display: 'grid', gap: 2, gridTemplateColumns: { lg: '1fr 340px' } }}>
                {/* meta moved to CardHeader subheader */}
                <Box>
                    <Typography variant='subtitle2' sx={{ mb: .5 }}>{activeQuestion?.title || activeQuestion?.questionId}</Typography>
                    <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap', mb: 1 }}>
                        {(() => {
                            const meta = assignment?.questions?.find(q => q.id === (activeQuestion as any)?.questionId);
                            return (
                                <>
                                    {!!(meta?.difficulty || activeQuestion?.status) && <DifficultyChip value={meta?.difficulty || activeQuestion?.status} />}
                                    <Chip size='small' variant='outlined' label={`${meta?.pointsOverride != null ? meta.pointsOverride : meta?.maxPoints || '—'} pts`} />
                                </>
                            );
                        })()}
                        {typeof currentAttempt?.attempt === 'number' && <Chip size='small' label={`Attempt ${String(currentAttempt.attempt)}`} />}
                        {((currentAttempt as any)?.manual || currentAttempt?.status === 'Manual') && <Chip size='small' color='secondary' label='Manual override' />}
                    </Box>
                    {(() => {
                        const qid = activeQuestion?.questionId || '';
                        const detail = qDetail[qid] || {};
                        const loading = !!loadingQ[qid];
                        return (
                            <Box sx={{ mb: 1 }}>
                                {loading && <Typography variant='caption' color='text.secondary'>Loading question…</Typography>}
                                {!loading && (
                                    <PromptWithHint
                                        prompt={String(detail?.prompt || 'No prompt provided.')}
                                        hint={String(detail?.hints || '')}
                                        showHint={false}
                                        onToggle={() => { }}
                                        promptKey={`prompt-${qid}`}
                                        hintKeyPrefix={`hint-${qid}`}
                                    />
                                )}
                            </Box>
                        );
                    })()}
                    <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mb: 1 }}>
                        <Tab label='Schema' />
                        <Tab label='Editor' />
                    </Tabs>
                    {tab === 0 && (
                        <Box sx={{ color: 'text.secondary', fontSize: 14 }}>
                            <SchemaPreview sql={String((qDetail[activeQuestion?.questionId || ''] || {}).initSql || '')} />
                        </Box>
                    )}
                    {tab === 1 && (
                        <SqlEditor value={(currentAttempt?.sql ?? activeQuestion?.sql) || ''} onChange={() => { }} readOnly minRows={10} placeholder='-- no SQL' />
                    )}
                </Box>
                <Box sx={{ display: 'grid', gap: 2 }}>
                    <Card variant='outlined' sx={{ '&:first-of-type': { mt: { xs: 1, lg: 0 } } }}>
                        <CardContent sx={{ pb: 1.5 }}>
                            <GradeDisplay grade={typeof currentAttempt?.grade === 'number' ? currentAttempt.grade : null} denom={typeof currentAttempt?.grade === 'number' ? ((currentAttempt.grade <= 1) ? 1 : 10) : null} />
                            <Divider sx={{ my: 1.5 }} />
                            <RubricBars rubric={{
                                syntax: Number(currentAttempt?.rubric?.syntax ?? currentAttempt?.rubric?.correctness ?? 0),
                                semantics: Number(currentAttempt?.rubric?.semantics ?? currentAttempt?.rubric?.style ?? 0),
                                results: Number(currentAttempt?.rubric?.results ?? currentAttempt?.rubric?.efficiency ?? 0),
                                absent: currentAttempt?.rubric?.absent
                            }} />
                            <Box sx={{ mt: 2 }}>
                                <Typography variant='body2' color='text.secondary'>Feedback</Typography>
                                <FeedbackList feedback={Array.isArray(currentAttempt?.feedback) ? currentAttempt?.feedback : []} emptyText='No feedback.' />
                            </Box>
                            <Divider sx={{ my: 1.5 }} />
                            <Box>
                                <Typography variant='body2' color='text.secondary' sx={{ mb: 1 }}>Attempts</Typography>
                                {(() => {
                                    const qid = activeQuestion?.questionId || '';
                                    const state = historyMap[qid];
                                    if (!state || state.loading) {
                                        return <Typography variant='caption' color='text.secondary' sx={{ display: 'inline-flex', alignItems: 'center', gap: .75 }}><CircularProgress size={12} /> Loading…</Typography>;
                                    }
                                    const attempts = [...(state.attempts || [])].sort((a, b) => a.attempt - b.attempt);
                                    if (!attempts.length) return <Typography variant='caption' color='text.secondary'>No attempts yet.</Typography>;
                                    const current = attempts[Math.min(Math.max(0, attemptTab), attempts.length - 1)];
                                    return (
                                        <Box>
                                            <Tabs value={Math.min(attemptTab, attempts.length - 1)} onChange={(_, v) => setAttemptTab(v)} variant='scrollable' scrollButtons='auto' sx={{ mb: 1 }}>
                                                {attempts.map((a, i) => (
                                                    <Tab key={a.id} label={`Attempt ${a.attempt}`} value={i} />
                                                ))}
                                            </Tabs>
                                            <Box sx={{ p: 1, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                                                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: .5 }}>
                                                    <Typography variant='subtitle2'>Attempt {current.attempt}</Typography>
                                                    <Typography variant='caption' color='text.secondary'>{current.createdAt ? formatDateTimeDDMMYYYYHHmm(current.createdAt) : ''}</Typography>
                                                </Box>
                                                <Typography variant='caption' color='text.secondary'>Select a tab above to switch attempts.</Typography>
                                            </Box>
                                        </Box>
                                    );
                                })()}
                            </Box>
                        </CardContent>
                    </Card>
                    <Box sx={{ p: 1, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                        <Typography variant='caption' color='text.secondary'>Submission {String(submission.id).substring(0, 8)}</Typography>
                    </Box>
                    <Card variant='outlined'>
                        <CardContent>
                            <Box sx={{ display: 'flex', gap: 1, justifyContent: 'space-between' }}>
                                <Button size='small' startIcon={<ArrowBackIcon />} disabled={idx === 0} onClick={() => setActiveIdx(i => Math.max(i - 1, 0))}>Previous</Button>
                                <Button size='small' endIcon={<ArrowForwardIcon />} disabled={idx === (total - 1)} onClick={() => setActiveIdx(i => Math.min(i + 1, total - 1))}>Next</Button>
                            </Box>
                            <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mt: 1 }}>Navigate questions.</Typography>
                        </CardContent>
                    </Card>
                </Box>
            </CardContent>
            <CardActions sx={{ justifyContent: 'flex-start', flexWrap: 'wrap', gap: 1 }}>
                <Typography variant='caption' color='text.secondary'>Progress {idx + 1}/{total} • Read-only</Typography>
            </CardActions>
        </Card>
    );
}
