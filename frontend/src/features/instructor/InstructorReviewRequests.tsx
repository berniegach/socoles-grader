'use client';
import { useEffect, useState, useMemo, useRef } from 'react';
import { Box, Card, CardContent, Typography, Chip, Button, TextField, MenuItem, Select, FormControl, InputLabel, Tooltip, Divider } from '@mui/material';
import PageCard from '@/components/PageCard';
import { useAuth } from '@/features/auth/AuthProvider';
import HeaderActions from '@/components/HeaderActions';
import RefreshIcon from '@mui/icons-material/Refresh';
import DoneIcon from '@mui/icons-material/Done';
import CheckCircleIcon from '@mui/icons-material/CheckCircleOutline';
import CloseIcon from '@mui/icons-material/Close';
import VisibilityIcon from '@mui/icons-material/Visibility';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import InstructorSubmissionPlayer from '@/features/instructor/InstructorSubmissionPlayer';

interface ReviewReq { id: string; assignmentId: string; questionId: string; submissionId: string; student: string; comment: string; status: string; createdAt: string; updatedAt: string; instructorReply?: string | null; replyAt?: string | null; }
interface AssignmentLite { id: string; title: string; }
interface QuestionLite { id: string; title: string; assignmentId?: string; }

export default function InstructorReviewRequests() {
    const { authFetch } = useAuth();
    const [requests, setRequests] = useState<ReviewReq[]>([]);
    const [loading, setLoading] = useState(false);
    const [assignments, setAssignments] = useState<AssignmentLite[]>([]);
    const [questions, setQuestions] = useState<QuestionLite[]>([]);
    const [filterAssignment, setFilterAssignment] = useState<string>('');
    const [filterStatus, setFilterStatus] = useState<string>('Pending');
    const [search, setSearch] = useState('');
    // Inline viewer state
    const [activeRequest, setActiveRequest] = useState<ReviewReq | null>(null);
    const [activeSubmission, setActiveSubmission] = useState<any | null>(null);
    const [fetchingSubmission, setFetchingSubmission] = useState(false);
    // Thread messages
    const [messages, setMessages] = useState<any[]>([]);
    const [msgDraft, setMsgDraft] = useState('');
    const [sendingMsg, setSendingMsg] = useState(false);
    const [manualGrade, setManualGrade] = useState<string>('');
    const [manualFeedback, setManualFeedback] = useState<string>('');
    const [savingManual, setSavingManual] = useState(false);
    const [saveSuccess, setSaveSuccess] = useState(false);
    // polling refs
    const msgPollRef = useRef<any>(null);
    const listPollRef = useRef<any>(null);

    async function load() {
        setLoading(true);
        try {
            const [aRes, rRes] = await Promise.all([
                authFetch('/api/assignments?include=questions'),
                authFetch('/api/review-requests')
            ]);
            let aData: any[] = []; if (aRes.ok) aData = await aRes.json();
            let rData: any[] = []; if (rRes.ok) rData = await rRes.json();
            const as: AssignmentLite[] = aData.map(a => ({ id: a.id, title: a.title }));
            const qs: QuestionLite[] = aData.flatMap(a => (a.questions || []).map((q: any) => ({ id: q.id, title: q.title, assignmentId: a.id })));
            setAssignments(as); setQuestions(qs);
            setRequests(rData || []);
        } finally { setLoading(false); }
    }
    useEffect(() => { load(); }, []);
    // Poll list while there are pending requests or an activeRequest is open
    useEffect(() => {
        const shouldPoll = !activeRequest && (requests.some(r => r.status === 'Pending') || filterStatus === 'Pending');
        if (!shouldPoll) { if (listPollRef.current) { clearInterval(listPollRef.current); listPollRef.current = null; } return; }
        if (listPollRef.current) return;
        listPollRef.current = setInterval(() => { load(); }, 10000); // every 10s
        return () => { if (listPollRef.current) { clearInterval(listPollRef.current); listPollRef.current = null; } };
    }, [requests, filterStatus, activeRequest]);

    async function updateStatus(id: string, status: string) {
        const res = await authFetch('/api/review-requests', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id, status }) });
        if (res.ok) {
            const upd = await res.json();
            setRequests(r => r.map(rr => rr.id === id ? upd : rr));
            if (activeRequest && activeRequest.id === id) setActiveRequest(upd); // reflect change inline
        }
    }

    async function loadMessages(requestId: string) {
        try {
            const res = await authFetch(`/api/review-request-messages?requestId=${requestId}`);
            if (res.ok) {
                const arr = await res.json();
                setMessages(Array.isArray(arr) ? arr : []);
            }
        } catch { /* ignore */ }
    }

    async function sendMessage() {
        if (!activeRequest || !msgDraft.trim()) return;
        setSendingMsg(true);
        try {
            const res = await authFetch('/api/review-request-messages', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ requestId: activeRequest.id, message: msgDraft.trim() }) });
            if (res.ok) {
                const created = await res.json();
                setMessages(m => [...m, created]);
                setMsgDraft('');
            }
        } finally { setSendingMsg(false); }
    }

    async function saveManualRegrade() {
        if (!activeRequest || !activeSubmission) return;
        const q = (activeSubmission.questions || []).find((q: any) => q.questionId === activeRequest.questionId);
        if (!q?.id) return;
        const gradeNum = manualGrade.trim() === '' ? null : Number(manualGrade);
        setSavingManual(true);
        try {
            const res = await authFetch('/api/question-submissions', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: q.id, grade: gradeNum, feedback: manualFeedback.split(/\n+/).map(f => f.trim()).filter(Boolean), status: 'Manual', incrementAttempt: true }) });
            if (res.ok) {
                // refresh question submissions for submission so player sees latest
                const qsRes = await authFetch(`/api/question-submissions?submissionId=${activeRequest.submissionId}`);
                if (qsRes.ok) {
                    const qs = await qsRes.json();
                    setActiveSubmission((s: any) => s ? { ...s, questions: qs } : s);
                }
                // Clear inputs
                setManualFeedback('');
                // keep grade to maybe apply again
                // Show success feedback and temporarily disable button
                setSaveSuccess(true);
                setTimeout(() => setSaveSuccess(false), 2500);
            }
        } finally { setSavingManual(false); }
    }

    async function openRequest(r: ReviewReq) {
        setActiveRequest(r); setActiveSubmission(null); setFetchingSubmission(true);
        setMessages([]); setMsgDraft('');
        try {
            // fetch submission
            const subRes = await authFetch(`/api/submissions?id=${encodeURIComponent(r.submissionId)}`);
            let submission: any = null;
            if (subRes.ok) {
                const data = await subRes.json();
                submission = Array.isArray(data) ? data.find((s: any) => s.id === r.submissionId) : data;
            }
            // fetch question submissions for that submission
            if (submission) {
                const qsRes = await authFetch(`/api/question-submissions?submissionId=${r.submissionId}`);
                if (qsRes.ok) {
                    const qs = await qsRes.json();
                    submission = { ...submission, questions: qs };
                }
            }
            setActiveSubmission(submission);
        } finally { setFetchingSubmission(false); }
        // load messages after base data
        void loadMessages(r.id);
    }
    // Poll messages of active pending request
    useEffect(() => {
        if (!activeRequest || activeRequest.status !== 'Pending') { if (msgPollRef.current) { clearInterval(msgPollRef.current); msgPollRef.current = null; } return; }
        if (msgPollRef.current) return;
        msgPollRef.current = setInterval(() => { loadMessages(activeRequest.id); }, 5000);
        return () => { if (msgPollRef.current) { clearInterval(msgPollRef.current); msgPollRef.current = null; } };
    }, [activeRequest?.id, activeRequest?.status]);

    const filtered = useMemo(() => {
        return requests.filter(r => (
            (!filterAssignment || r.assignmentId === filterAssignment) &&
            (!filterStatus || r.status === filterStatus) &&
            (!search || r.comment.toLowerCase().includes(search.toLowerCase()) || r.student.toLowerCase().includes(search.toLowerCase()))
        ));
    }, [requests, filterAssignment, filterStatus, search]);

    const assignmentTitle = (id: string) => assignments.find(a => a.id === id)?.title || '—';
    const questionTitle = (id: string) => questions.find(q => q.id === id)?.title || '—';

    const headerActions = activeRequest ? (
        <HeaderActions actions={[
            { key: 'back', label: 'Back', ariaLabel: 'Back to requests', onClick: () => { setActiveRequest(null); setActiveSubmission(null); }, icon: <ArrowBackIcon fontSize='small' /> }
        ]} />
    ) : (
        <HeaderActions actions={[
            { key: 'refresh', label: 'Refresh', ariaLabel: 'Refresh', onClick: () => load(), icon: <RefreshIcon fontSize='small' /> }
        ]} />
    );

    return (
        <PageCard headerTitle={activeRequest ? 'Review Request Viewer' : `Review Requests (${filtered.length}${filterStatus ? ' • ' + filterStatus : ''})`} headerProps={{ height: 56 }} headerActions={headerActions} headerActionsVariant='plain'>
            <CardContent sx={{ display: 'grid', gap: 2 }}>
                {!activeRequest && (
                    <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap', alignItems: 'center' }}>
                        <FormControl size='small' sx={{ minWidth: 160 }}>
                            <InputLabel>Assignment</InputLabel>
                            <Select label='Assignment' value={filterAssignment} onChange={e => setFilterAssignment(e.target.value)}>
                                <MenuItem value=''><em>All</em></MenuItem>
                                {assignments.map(a => <MenuItem key={a.id} value={a.id}>{a.title}</MenuItem>)}
                            </Select>
                        </FormControl>
                        <FormControl size='small' sx={{ minWidth: 140 }}>
                            <InputLabel>Status</InputLabel>
                            <Select label='Status' value={filterStatus} onChange={e => setFilterStatus(e.target.value)}>
                                <MenuItem value=''><em>All</em></MenuItem>
                                <MenuItem value='Pending'>Pending</MenuItem>
                                <MenuItem value='Resolved'>Resolved</MenuItem>
                            </Select>
                        </FormControl>
                        <TextField size='small' placeholder='Search comment or student' value={search} onChange={e => setSearch(e.target.value)} />
                    </Box>
                )}
                {!activeRequest && (
                    <Box sx={{ display: 'grid', gap: 1 }}>
                        {filtered.map(r => (
                            <Card key={r.id} variant='outlined' sx={{ p: 1.25, display: 'grid', gap: .5 }}>
                                <Box sx={{ display: 'flex', justifyContent: 'space-between', gap: 1, flexWrap: 'wrap' }}>
                                    <Typography variant='subtitle2' sx={{ fontWeight: 600 }}>{assignmentTitle(r.assignmentId)}</Typography>
                                    <Chip size='small' color={r.status === 'Pending' ? 'warning' : r.status === 'Resolved' ? 'success' : 'default'} label={r.status} />
                                </Box>
                                <Typography variant='caption' color='text.secondary'>{questionTitle(r.questionId)}</Typography>
                                <Typography variant='body2' sx={{ whiteSpace: 'pre-wrap' }}>{r.comment}</Typography>
                                <Typography variant='caption' color='text.secondary'>Student: {r.student} • Submitted: {new Date(r.createdAt).toLocaleString()}</Typography>
                                <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                                    {r.status === 'Pending' && <Button size='small' variant='contained' color='success' startIcon={<CheckCircleIcon fontSize='inherit' />} onClick={() => updateStatus(r.id, 'Resolved')}>Resolve</Button>}
                                    {r.status === 'Resolved' && <Button size='small' variant='outlined' color='error' startIcon={<CloseIcon fontSize='inherit' />} onClick={() => updateStatus(r.id, 'Pending')}>Reopen</Button>}
                                    <Tooltip title='Open submission viewer'>
                                        <Button size='small' variant='text' startIcon={<VisibilityIcon fontSize='inherit' />} onClick={() => openRequest(r)}>View</Button>
                                    </Tooltip>
                                </Box>
                            </Card>
                        ))}
                        {!filtered.length && !loading && (
                            <Typography variant='body2' color='text.secondary'>No requests.</Typography>
                        )}
                        {loading && (
                            <Typography variant='body2' color='text.secondary'>Loading…</Typography>
                        )}
                    </Box>
                )}
                {activeRequest && (
                    <Box sx={{ display: 'grid', gap: 2 }}>
                        <Card variant='outlined' sx={{ p: 1.25, display: 'grid', gap: .75 }}>
                            <Typography variant='subtitle2'>{assignmentTitle(activeRequest.assignmentId)}</Typography>
                            <Typography variant='caption' color='text.secondary'>{questionTitle(activeRequest.questionId)}</Typography>
                            <Typography variant='body2' sx={{ whiteSpace: 'pre-wrap' }}>{activeRequest.comment}</Typography>
                            <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                                {activeRequest.status === 'Pending' && <Button size='small' variant='contained' color='success' onClick={() => updateStatus(activeRequest.id, 'Resolved')}>Resolve</Button>}
                                {activeRequest.status === 'Resolved' && <Button size='small' variant='outlined' color='error' onClick={() => updateStatus(activeRequest.id, 'Pending')}>Reopen</Button>}
                            </Box>
                            <Divider sx={{ my: 1 }} />
                            <Typography variant='caption' color='text.secondary' sx={{ fontWeight: 600 }}>Thread</Typography>
                            <Box sx={{ maxHeight: 240, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: .75, mt: .5, pr: .5 }}>
                                {messages.map(m => (
                                    <Box key={m.id} sx={{ alignSelf: m.senderRole === 'instructor' ? 'flex-end' : 'flex-start', maxWidth: '80%', p: .75, borderRadius: 1, bgcolor: m.senderRole === 'instructor' ? 'secondary.light' : 'grey.100', border: '1px solid', borderColor: 'divider' }}>
                                        <Typography variant='caption' sx={{ display: 'block', fontWeight: 600 }}>
                                            {m.senderRole === 'instructor' ? 'Instructor' : 'Student'}{m.sender ? ` • ${m.sender}` : ''}
                                        </Typography>
                                        <Typography variant='caption' sx={{ display: 'block', whiteSpace: 'pre-wrap' }}>{m.message}</Typography>
                                    </Box>
                                ))}
                                {!messages.length && <Typography variant='caption' color='text.secondary'>No messages yet.</Typography>}
                            </Box>
                            {activeRequest.status === 'Pending' && (
                                <Box sx={{ mt: 1, display: 'grid', gap: .75 }}>
                                    <TextField multiline minRows={2} size='small' placeholder='Type a reply...' value={msgDraft} onChange={e => setMsgDraft(e.target.value)} />
                                    <Button size='small' variant='outlined' disabled={sendingMsg || !msgDraft.trim()} onClick={sendMessage}>{sendingMsg ? 'Sending…' : 'Send Message'}</Button>
                                </Box>
                            )}
                        </Card>
                        {fetchingSubmission && <Typography variant='body2' color='text.secondary'>Loading submission…</Typography>}
                        {activeSubmission && (
                            <InstructorSubmissionPlayer
                                assignmentId={activeRequest.assignmentId}
                                submission={activeSubmission}
                                initialQuestionId={activeRequest.questionId}
                                onClose={() => { setActiveRequest(null); setActiveSubmission(null); }}
                            />
                        )}
                        {activeSubmission && (
                            <Card variant='outlined' sx={{ p: 1.25, display: 'grid', gap: .75 }}>
                                <Typography variant='subtitle2'>Manual Regrade</Typography>
                                <Typography variant='caption' color='text.secondary'>Enter a new grade and optional feedback lines (one per line). This will create a manual attempt.</Typography>
                                <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                                    <TextField label='Grade' size='small' value={manualGrade} onChange={e => setManualGrade(e.target.value)} sx={{ width: 120 }} />
                                </Box>
                                <TextField multiline minRows={3} size='small' label='Feedback (one line per item)' value={manualFeedback} onChange={e => setManualFeedback(e.target.value)} />
                                <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', flexWrap: 'wrap' }}>
                                    <Button size='small' variant='contained' disabled={savingManual || saveSuccess} onClick={saveManualRegrade} startIcon={saveSuccess ? <DoneIcon fontSize='inherit' /> : undefined}>
                                        {savingManual ? 'Saving...' : saveSuccess ? 'Saved' : 'Save Manual Regrade'}
                                    </Button>
                                    {saveSuccess && (
                                        <Typography variant='caption' color='success.main'>Manual regrade saved.</Typography>
                                    )}
                                </Box>
                            </Card>
                        )}
                        {!fetchingSubmission && !activeSubmission && <Typography variant='caption' color='text.secondary'>Submission not found.</Typography>}
                    </Box>
                )}
            </CardContent>
        </PageCard>
    );
}
