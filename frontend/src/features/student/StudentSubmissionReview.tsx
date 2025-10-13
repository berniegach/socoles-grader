"use client";
import { useEffect, useState, useMemo, useRef } from 'react';
import { Box, Card, CardHeader, CardContent, CardActions, Typography, Button, Chip, Tabs, Tab, Divider, Collapse, CircularProgress, TextField } from '@mui/material';
import DifficultyChip from '@/components/DifficultyChip';
import SchemaPreview from '@/components/SchemaPreview';
import PromptWithHint from '@/components/PromptWithHint';
import SqlEditor from '@/components/SqlEditor';
import RubricBars from '@/components/RubricBars';
import FeedbackList from '@/components/FeedbackList';
import GradeDisplay from '@/components/GradeDisplay';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
import type { AssignmentWithQuestions, QuestionSubmissionAttempt } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';

interface ReviewProps {
    assignmentId: string | null;
    onClose: () => void;
}

export default function StudentSubmissionReview({ assignmentId, onClose }: ReviewProps) {
    const { authFetch, user } = useAuth();
    const [assignment, setAssignment] = useState<AssignmentWithQuestions | null>(null);
    const [loading, setLoading] = useState(false);
    const [activeIdx, setActiveIdx] = useState(0);
    const [tab, setTab] = useState(1); // 0 schema, 1 editor
    const [questionDetail, setQuestionDetail] = useState<any>(null);
    const [qLoading, setQLoading] = useState(false);
    const [historyMap, setHistoryMap] = useState<Record<string, { loading: boolean; attempts: QuestionSubmissionAttempt[]; error?: string }>>({});
    const [attemptTab, setAttemptTab] = useState(0);
    const [historyOpen, setHistoryOpen] = useState(false);
    const [submissionId, setSubmissionId] = useState<string | null>(null);
    const [reviewRequests, setReviewRequests] = useState<Record<string, { id: string; comment: string; status: string; instructorReply?: string | null; replyAt?: string | null }>>({});
    const [messages, setMessages] = useState<Record<string, any[]>>({}); // questionId -> messages
    const [messageDraft, setMessageDraft] = useState('');
    const [sending, setSending] = useState(false);
    // useRef for poller to avoid state changes causing re-renders
    const pollerRef = useRef<any>(null);
    // state for creating a new review request (dialog)
    const [reviewDialogOpen, setReviewDialogOpen] = useState(false);
    const [reviewComment, setReviewComment] = useState('');

    useEffect(() => {
        if (!assignmentId || !user?.token) return;
        let cancelled = false;
        (async () => {
            setLoading(true);
            try {
                const res = await authFetch('/api/assignments?include=questions');
                if (res.ok) {
                    const data = await res.json();
                    const found = (Array.isArray(data) ? data : []).find((a: any) => a.id === assignmentId) || null;
                    if (!cancelled) setAssignment(found);
                }
            } finally { if (!cancelled) setLoading(false); }
        })();
        return () => { cancelled = true; };
    }, [assignmentId, user?.token, authFetch]);

    const questions = useMemo(() => assignment?.questions?.slice().sort((a, b) => a.position - b.position) || [], [assignment?.questions]);
    const currentQuestion = questions[activeIdx] || null;
    const requestForCurrent = reviewRequests[currentQuestion?.id || ''];
    const threadMessages = messages[currentQuestion?.id || ''] || [];

    useEffect(() => {
        if (!currentQuestion || !user?.token) { setQuestionDetail(null); return; }
        const qid = currentQuestion.id;
        setQLoading(true);
        (async () => {
            try {
                const res = await authFetch(`/api/questions/${qid}`);
                const d = res.ok ? await res.json() : null;
                setQuestionDetail(d);
            } finally { setQLoading(false); }
        })();
    }, [currentQuestion?.id, user?.token, authFetch]);

    // load attempt history (final attempts) for read-only viewing
    useEffect(() => {
        if (!currentQuestion || !user?.token) return;
        const qid = currentQuestion.id;
        if (historyMap[qid]?.attempts?.length) return;
        setHistoryMap(m => ({ ...m, [qid]: { loading: true, attempts: [] } }));
        (async () => {
            try {
                // fetch latest question_submission to derive id history (reuse API pattern from player)
                // First need submission list to identify submission for this assignment
                const subs = await authFetch('/api/submissions');
                let submissionId: string | null = null;
                if (subs.ok) {
                    const list = await subs.json();
                    const mine = (list || []).filter((s: any) => s.student === user?.name && s.assignmentId === assignment?.id);
                    const final = mine.find((s: any) => ['Submitted', 'Auto-graded', 'Needs review'].includes(s.status));
                    submissionId = final?.id || null;
                }
                if (!submissionId) throw new Error('No submission');
                // get question_submissions for this submission
                const qsubsRes = await authFetch(`/api/question-submissions?submissionId=${encodeURIComponent(submissionId)}`);
                if (!qsubsRes.ok) throw new Error('No question submissions');
                const qsubs = await qsubsRes.json();
                const qs = (qsubs || []).find((x: any) => x.questionId === currentQuestion.id);
                const qsId = qs?.id;
                if (!qsId) throw new Error('Missing question submission');
                const histRes = await authFetch(`/api/question-submissions?historyOf=${qsId}`);
                if (!histRes.ok) throw new Error('History error');
                const attempts: QuestionSubmissionAttempt[] = await histRes.json();
                setHistoryMap(m => ({ ...m, [qid]: { loading: false, attempts } }));
                setAttemptTab(attempts.length ? attempts.length - 1 : 0);
            } catch (e: any) {
                setHistoryMap(m => ({ ...m, [currentQuestion.id]: { loading: false, attempts: [], error: e?.message || 'Failed' } }));
            }
        })();
    }, [currentQuestion?.id, assignment?.id, user?.name, user?.token, authFetch]);

    // Identify final submission id for this assignment (once) and then load review requests
    useEffect(() => {
        if (!assignment?.id || !user?.token) return;
        (async () => {
            try {
                const subs = await authFetch('/api/submissions');
                if (subs.ok) {
                    const list = await subs.json();
                    const mine = (list || []).filter((s: any) => s.student === user?.name && s.assignmentId === assignment.id);
                    const final = mine.find((s: any) => ['Submitted', 'Auto-graded', 'Needs review'].includes(s.status));
                    if (final?.id) setSubmissionId(final.id);
                }
            } catch { /* ignore */ }
        })();
    }, [assignment?.id, user?.token, user?.name, authFetch]);

    useEffect(() => {
        if (!submissionId || !assignment?.id || !user?.token) return;
        (async () => {
            try {
                const res = await authFetch(`/api/review-requests?assignmentId=${assignment.id}&submissionId=${submissionId}&student=${encodeURIComponent(user?.name || '')}`);
                if (res.ok) {
                    const arr = await res.json();
                    const map: Record<string, { id: string; comment: string; status: string; instructorReply?: string | null; replyAt?: string | null }> = {};
                    (arr || []).forEach((r: any) => { if (r.questionId) map[r.questionId] = { id: r.id, comment: r.comment, status: r.status, instructorReply: r.instructorReply, replyAt: r.replyAt }; });
                    setReviewRequests(map);
                }
            } catch { /* ignore */ }
        })();
    }, [submissionId, assignment?.id, user?.token, user?.name, authFetch]);

    // Load messages when requestForCurrent changes
    useEffect(() => {
        if (!requestForCurrent?.id || !user?.token || !currentQuestion?.id) return;
        let cancelled = false;
        (async () => {
            try {
                const res = await authFetch(`/api/review-request-messages?requestId=${requestForCurrent.id}`);
                if (res.ok) {
                    const arr = await res.json();
                    if (!cancelled) setMessages(m => ({ ...m, [currentQuestion.id]: arr }));
                }
            } catch { /* ignore */ }
        })();
        return () => { cancelled = true; };
    }, [requestForCurrent?.id, currentQuestion?.id, user?.token, authFetch]);

    // Polling for real-time updates while pending
    useEffect(() => {
        // stop if no active pending request
        if (!requestForCurrent?.id || requestForCurrent.status !== 'Pending') {
            if (pollerRef.current) { clearInterval(pollerRef.current); pollerRef.current = null; }
            return;
        }
        // already polling
        if (pollerRef.current) return;
        pollerRef.current = setInterval(async () => {
            try {
                const res = await authFetch(`/api/review-request-messages?requestId=${requestForCurrent.id}`);
                if (res.ok) {
                    const arr = await res.json();
                    setMessages(m => ({ ...m, [currentQuestion?.id || '']: arr }));
                }
            } catch { /* ignore */ }
        }, 5000);
        return () => { if (pollerRef.current) { clearInterval(pollerRef.current); pollerRef.current = null; } };
    }, [requestForCurrent?.id, requestForCurrent?.status, currentQuestion?.id, authFetch]);

    async function sendMessage() {
        if (!requestForCurrent?.id || !messageDraft.trim()) return;
        setSending(true);
        try {
            const res = await authFetch('/api/review-request-messages', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ requestId: requestForCurrent.id, message: messageDraft.trim() }) });
            if (res.ok) {
                const created = await res.json();
                setMessages(m => ({ ...m, [currentQuestion?.id || '']: [...(m[currentQuestion?.id || ''] || []), created] }));
                setMessageDraft('');
            }
        } finally { setSending(false); }
    }

    const qid = currentQuestion?.id;
    const attempts = (qid && historyMap[qid]?.attempts) ? [...historyMap[qid].attempts].sort((a, b) => a.attempt - b.attempt) : [];
    const activeAttempt = attempts[Math.min(Math.max(0, attemptTab), Math.max(0, attempts.length - 1))] || null;

    if (!assignmentId) return null;
    if (loading) return <Card sx={{ p: 3 }}><Typography>Loading submission…</Typography></Card>;
    if (!assignment) return <Card sx={{ p: 3 }}><Typography>Assignment not found.</Typography><Button sx={{ mt: 1 }} onClick={onClose}>Close</Button></Card>;

    return (
        <Card sx={{ display: 'flex', flexDirection: 'column', position: 'relative' }}>
            <CardHeader
                title={<Typography variant='subtitle1'>{assignment.title}</Typography>}
                subheader={<Typography variant='caption' color='text.secondary'>Question {activeIdx + 1} of {questions.length} • Review</Typography>}
                action={<Button size='small' onClick={onClose}>Close</Button>}
            />
            <CardContent sx={{ display: 'grid', gap: 2, gridTemplateColumns: { lg: '1fr 340px' } }}>
                <Box>
                    <Typography variant='subtitle2' sx={{ mb: .5 }}>{currentQuestion?.title}</Typography>
                    <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap', mb: 1 }}>
                        {currentQuestion?.difficulty && <DifficultyChip value={currentQuestion.difficulty} size='small' />}
                        {currentQuestion?.pointsOverride != null ? <Chip size='small' label={`${currentQuestion.pointsOverride} pts`} color='primary' /> : <Chip size='small' label={`${currentQuestion?.maxPoints} pts`} />}
                        {typeof activeAttempt?.attempt === 'number' && <Chip size='small' label={`Attempt ${activeAttempt.attempt}`} />}
                        {requestForCurrent && <Chip size='small' color={requestForCurrent.status === 'Pending' ? 'warning' : 'success'} label={requestForCurrent.status === 'Pending' ? 'Review requested' : 'Review resolved'} />}
                    </Box>
                    {/* Full prompt content first */}
                    <PromptWithHint
                        prompt={String(questionDetail?.prompt || 'No prompt provided.')}
                        hint={String(questionDetail?.hints || '')}
                        showHint={false}
                        onToggle={() => { }}
                        promptKey={`prompt-${currentQuestion?.id || 'none'}`}
                        hintKeyPrefix={`hint-${currentQuestion?.id || 'none'}`}
                    />
                    {/* Schema / Editor tabs now below content */}
                    <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mt: 2, mb: 1 }}>
                        <Tab label='Schema' />
                        <Tab label='Editor' />
                    </Tabs>
                    {tab === 0 && (
                        <Box sx={{ color: 'text.secondary', fontSize: 14 }}>
                            <SchemaPreview sql={String(questionDetail?.initSql || '')} />
                        </Box>
                    )}
                    {tab === 1 && (
                        <SqlEditor value={(activeAttempt?.sql) || ''} onChange={() => { }} readOnly minRows={10} placeholder='-- no SQL' />
                    )}
                    {/* Review request action (only if finished submission) */}
                    {submissionId && !requestForCurrent && (
                        <Box sx={{ mt: 2 }}>
                            <Button size='small' variant='outlined' onClick={(e) => { e.preventDefault(); e.stopPropagation(); setReviewDialogOpen(true); }}>Request instructor review</Button>
                        </Box>
                    )}
                    {requestForCurrent && (
                        <Box sx={{ mt: 1, p: 1, border: '1px solid', borderColor: 'divider', borderRadius: 1, display: 'grid', gap: .75 }}>
                            <Typography variant='caption' color='text.secondary' sx={{ fontWeight: 600 }}>Review Thread</Typography>
                            <Box sx={{ maxHeight: 220, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: .75, pr: .5 }}>
                                {threadMessages.map(m => (
                                    <Box key={m.id} sx={{ alignSelf: m.senderRole === 'instructor' ? 'flex-end' : 'flex-start', maxWidth: '80%', p: .75, borderRadius: 1, bgcolor: m.senderRole === 'instructor' ? 'secondary.light' : 'grey.100', border: '1px solid', borderColor: 'divider' }}>
                                        <Typography variant='caption' sx={{ display: 'block', fontWeight: 600 }}>{m.senderRole === 'instructor' ? 'Instructor' : 'You'}</Typography>
                                        <Typography variant='caption' sx={{ display: 'block', whiteSpace: 'pre-wrap' }}>{m.message}</Typography>
                                    </Box>
                                ))}
                                {!threadMessages.length && <Typography variant='caption' color='text.secondary'>No messages yet.</Typography>}
                            </Box>
                            {requestForCurrent.status === 'Pending' && (
                                <Box sx={{ display: 'grid', gap: .5 }}>
                                    <TextField size='small' multiline minRows={2} placeholder='Type a reply...' value={messageDraft} onChange={e => setMessageDraft(e.target.value)} />
                                    <Button size='small' variant='outlined' disabled={sending || !messageDraft.trim()} onClick={sendMessage}>{sending ? 'Sending…' : 'Send'}</Button>
                                </Box>
                            )}
                        </Box>
                    )}
                </Box>
                <Box sx={{ display: 'grid', gap: 2 }}>
                    <Card variant='outlined'>
                        <CardContent sx={{ pb: 1.5 }}>
                            <GradeDisplay grade={typeof activeAttempt?.grade === 'number' ? activeAttempt.grade : null} denom={typeof activeAttempt?.grade === 'number' ? ((activeAttempt.grade <= 1) ? 1 : 10) : null} />
                            <Divider sx={{ my: 1.5 }} />
                            <RubricBars rubric={{
                                syntax: Number(activeAttempt?.rubric?.syntax ?? activeAttempt?.rubric?.correctness ?? 0),
                                semantics: Number(activeAttempt?.rubric?.semantics ?? activeAttempt?.rubric?.style ?? 0),
                                results: Number(activeAttempt?.rubric?.results ?? activeAttempt?.rubric?.efficiency ?? 0),
                                absent: activeAttempt?.rubric?.absent
                            }} />
                            <Box sx={{ mt: 2 }}>
                                <Typography variant='body2' color='text.secondary'>Feedback</Typography>
                                <FeedbackList feedback={Array.isArray(activeAttempt?.feedback) ? activeAttempt.feedback : []} emptyText='No feedback.' />
                            </Box>
                            <Divider sx={{ my: 1.5 }} />
                            <Box>
                                <Typography variant='body2' color='text.secondary' sx={{ mb: 1 }}>Attempts</Typography>
                                {(() => {
                                    if (historyMap[currentQuestion?.id || '']?.loading) return <Typography variant='caption' color='text.secondary' sx={{ display: 'inline-flex', alignItems: 'center', gap: .75 }}><CircularProgress size={12} /> Loading…</Typography>;
                                    if (!attempts.length) return <Typography variant='caption' color='text.secondary'>No attempts.</Typography>;
                                    return (
                                        <Box>
                                            <Tabs value={Math.min(attemptTab, attempts.length - 1)} onChange={(_, v) => setAttemptTab(v)} variant='scrollable' scrollButtons='auto' sx={{ mb: 1 }}>
                                                {attempts.map((a, i) => (<Tab key={a.id} label={`Attempt ${a.attempt}`} value={i} />))}
                                            </Tabs>
                                            <Box sx={{ p: 1, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                                                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: .5 }}>
                                                    <Typography variant='subtitle2'>Attempt {activeAttempt?.attempt}</Typography>
                                                    <Typography variant='caption' color='text.secondary'>{activeAttempt?.createdAt ? formatDateTimeDDMMYYYYHHmm(activeAttempt.createdAt) : ''}</Typography>
                                                </Box>
                                                <Typography variant='caption' color='text.secondary'>Select a tab above to switch attempts.</Typography>
                                            </Box>
                                        </Box>
                                    );
                                })()}
                            </Box>
                        </CardContent>
                    </Card>
                    <Card variant='outlined'>
                        <CardContent>
                            <Box sx={{ display: 'flex', gap: 1, justifyContent: 'space-between' }}>
                                <Button size='small' startIcon={<ArrowBackIcon />} disabled={activeIdx === 0} onClick={() => setActiveIdx(i => Math.max(i - 1, 0))}>Previous</Button>
                                <Button size='small' endIcon={<ArrowForwardIcon />} disabled={activeIdx === (questions.length - 1)} onClick={() => setActiveIdx(i => Math.min(i + 1, questions.length - 1))}>Next</Button>
                            </Box>
                            <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mt: 1 }}>Navigate questions.</Typography>
                        </CardContent>
                    </Card>
                </Box>
            </CardContent>
            <CardActions sx={{ justifyContent: 'space-between', flexWrap: 'wrap', gap: 1 }}>
                <Typography variant='caption' color='text.secondary'>Progress {activeIdx + 1}/{questions.length} • Review</Typography>
            </CardActions>
            {reviewDialogOpen && currentQuestion && (
                <Box sx={{ position: 'fixed', inset: 0, zIndex: 1300, display: 'flex', alignItems: 'center', justifyContent: 'center', p: 2, bgcolor: 'rgba(0,0,0,0.35)' }} onClick={() => setReviewDialogOpen(false)}>
                    <Card sx={{ width: 420, maxWidth: '100%', p: 2 }} onClick={(e) => e.stopPropagation()}>
                        <Typography variant='subtitle2' sx={{ mb: 1 }}>Request Review</Typography>
                        <Typography variant='caption' color='text.secondary'>Explain why this question should be reviewed. Be specific.</Typography>
                        <Box component='textarea' value={reviewComment} onChange={(e: any) => setReviewComment(e.target.value)}
                            rows={5}
                            style={{ width: '100%', marginTop: 12, resize: 'vertical', fontFamily: 'inherit', fontSize: 14, padding: 8, borderRadius: 6, border: '1px solid rgba(0,0,0,0.2)' }}
                        />
                        <Box sx={{ display: 'flex', justifyContent: 'flex-end', gap: 1, mt: 2 }}>
                            <Button size='small' onClick={() => setReviewDialogOpen(false)}>Cancel</Button>
                            <Button size='small' variant='contained' disabled={!reviewComment.trim()} onClick={async () => {
                                if (!assignment?.id || !currentQuestion?.id || !submissionId || !user?.name) return;
                                try {
                                    const res = await authFetch('/api/review-requests', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ assignmentId: assignment.id, questionId: currentQuestion.id, submissionId, student: user.name, comment: reviewComment.trim() }) });
                                    if (res.ok) {
                                        const created = await res.json();
                                        setReviewRequests(m => ({ ...m, [currentQuestion.id]: { id: created.id, comment: created.comment, status: created.status, instructorReply: created.instructorReply, replyAt: created.replyAt } }));
                                        setReviewDialogOpen(false);
                                        setReviewComment('');
                                    }
                                } catch { /* ignore */ }
                            }}>Submit</Button>
                        </Box>
                    </Card>
                </Box>
            )}
        </Card>
    );
}
