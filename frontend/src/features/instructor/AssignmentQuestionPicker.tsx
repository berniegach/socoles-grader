"use client";
import { useEffect, useMemo, useState, forwardRef, useImperativeHandle } from 'react';
import { useAuth } from '@/features/auth/AuthProvider';
import {
    Box,
    Card,
    CardContent,
    CardActions,
    Typography,
    TextField,
    IconButton,
    Button,
    CircularProgress,
    Tooltip,
    Chip,
    LinearProgress,
    Alert,
    InputAdornment,
} from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import DeleteIcon from '@mui/icons-material/Delete';
import RefreshIcon from '@mui/icons-material/Refresh';
import ArrowUpwardIcon from '@mui/icons-material/ArrowUpward';
import ArrowDownwardIcon from '@mui/icons-material/ArrowDownward';
import type { Assignment, AssignmentWithQuestions, Question } from '@/lib/types';
import DifficultyChip from '@/components/DifficultyChip';

interface PickerProps {
    assignment: Assignment | AssignmentWithQuestions | null;
    onClose?: () => void;
}

export interface AssignmentQuestionPickerHandle {
    save: () => Promise<void> | void;
}

const AssignmentQuestionPicker = forwardRef<AssignmentQuestionPickerHandle, PickerProps>(function AssignmentQuestionPicker({ assignment, onClose }, ref) {
    const { authFetch, user } = useAuth();
    const [loadingBank, setLoadingBank] = useState(false);
    const [loadingLinks, setLoadingLinks] = useState(false);
    const [savingOrder, setSavingOrder] = useState(false);
    const [bank, setBank] = useState<Question[]>([]);
    const [linked, setLinked] = useState<any[]>([]); // each has question fields + position + pointsOverride
    const [filter, setFilter] = useState('');
    const [error, setError] = useState<string | null>(null);
    const [pointsEdit, setPointsEdit] = useState<Record<string, string>>({});

    const assignId = assignment?.id;

    const filteredBank = useMemo(() => {
        const f = filter.toLowerCase();
        const linkedIds = new Set((linked || []).map((l: any) => l.questionId));
        return bank.filter(q => !linkedIds.has(q.id) && (!f || q.title.toLowerCase().includes(f) || q.difficulty.toLowerCase().includes(f)));
    }, [bank, filter, linked]);
    const totalLinkedPoints = useMemo(() => {
        return linked.reduce((sum, l) => {
            const edit = pointsEdit[l.questionId];
            const val = edit === '' || edit === undefined ? (Number(l.pointsOverride ?? l.maxPoints) || 0) : Number(edit) || 0;
            return sum + val;
        }, 0);
    }, [linked, pointsEdit]);

    async function loadBank() {
        setLoadingBank(true); setError(null);
        try {
            if (!user?.token) return;
            const res = await authFetch('/api/questions');
            if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed');
            const data = await res.json();
            setBank(data);
        } catch (e: any) { setError(e.message); } finally { setLoadingBank(false); }
    }
    async function loadLinked() {
        if (!assignId) return;
        setLoadingLinks(true); setError(null);
        try {
            if (!user?.token) return;
            const res = await authFetch(`/api/assignment-questions?assignmentId=${assignId}`);
            if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed');
            const data = await res.json();
            setLinked(data);
            setPointsEdit(Object.fromEntries(data.map((r: any) => [r.questionId, r.pointsOverride != null ? String(r.pointsOverride) : ''])));
        } catch (e: any) { setError(e.message); } finally { setLoadingLinks(false); }
    }

    useEffect(() => { loadBank(); }, [user?.token]);
    useEffect(() => { loadLinked(); }, [assignId, user?.token]);

    async function addQuestion(q: Question) {
        if (!assignId) return;
        try {
            if (!user?.token) throw new Error('Not authenticated');
            const res = await authFetch('/api/assignment-questions', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ assignmentId: assignId, questionId: q.id, position: linked.length }) });
            if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed');
            await loadLinked();
        } catch (e: any) { setError(e.message); }
    }
    async function removeQuestion(questionId: string) {
        if (!assignId) return;
        try {
            if (!user?.token) throw new Error('Not authenticated');
            const res = await authFetch(`/api/assignment-questions?assignmentId=${assignId}&questionId=${questionId}`, { method: 'DELETE' });
            if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed');
            setLinked(l => l.filter(r => r.questionId !== questionId));
        } catch (e: any) { setError(e.message); }
    }
    function move(questionId: string, dir: -1 | 1) {
        setLinked(prev => {
            const arr = [...prev];
            const idx = arr.findIndex(r => r.questionId === questionId);
            if (idx < 0) return prev;
            const newIdx = idx + dir;
            if (newIdx < 0 || newIdx >= arr.length) return prev;
            const [item] = arr.splice(idx, 1);
            arr.splice(newIdx, 0, item);
            return arr.map((r, i) => ({ ...r, position: i }));
        });
    }
    async function saveOrder() {
        if (!assignId) return;
        setSavingOrder(true); setError(null);
        try {
            const order = linked.map(l => ({ questionId: l.questionId, position: l.position }));
            if (!user?.token) throw new Error('Not authenticated');
            let res = await authFetch('/api/assignment-questions', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ assignmentId: assignId, order }) });
            if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed to reorder');
            for (const l of linked) {
                const po = pointsEdit[l.questionId];
                if (po !== undefined) {
                    res = await authFetch('/api/assignment-questions', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ assignmentId: assignId, questionId: l.questionId, position: l.position, pointsOverride: po === '' ? null : Number(po) }) });
                    if (!res.ok) throw new Error((await res.json().catch(() => ({})))?.error || 'Failed to set points');
                }
            }
            await loadLinked();
        } catch (e: any) { setError(e.message); } finally { setSavingOrder(false); }
    }

    const bankIds = useMemo(() => new Set(linked.map(l => l.questionId)), [linked]);

    useImperativeHandle(ref, () => ({ save: saveOrder }), [linked, pointsEdit, assignId]);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: 520, maxHeight: '82vh' }}>
            <Box sx={{ flex: 1, display: 'grid', gap: 2, gridTemplateColumns: { md: '1fr 1fr', xs: '1fr' }, minHeight: 0, overflowX: 'auto', overflowY: 'hidden' }}>
                {/* BANK */}
                <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: 0, minWidth: { md: 420, xs: '100%' } }}>
                    <Box sx={{ position: 'sticky', top: 0, zIndex: 1, bgcolor: 'background.paper', borderBottom: '1px solid', borderColor: 'divider', mb: 1, py: 1, display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 1 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Typography variant='subtitle2'>Question bank</Typography>
                            <Chip size='small' label={`${filteredBank.length}/${bank.length}`} />
                        </Box>
                        <Tooltip title="Refresh"><span><IconButton onClick={loadBank} disabled={loadingBank}><RefreshIcon fontSize="small" /></IconButton></span></Tooltip>
                    </Box>
                    <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', mb: 1 }}>
                        <TextField size="small" variant="outlined" placeholder="Filter…" value={filter} onChange={e => setFilter(e.target.value)} fullWidth />
                    </Box>
                    <Box sx={{ flex: 1, minHeight: 0, overflow: 'auto', border: '1px solid', borderColor: 'divider', borderRadius: 1, p: 1, display: 'flex', flexDirection: 'column', gap: 1 }}>
                        {loadingBank && (
                            <Box sx={{ py: 2, display: 'flex', justifyContent: 'center' }}>
                                <CircularProgress size={20} />
                            </Box>
                        )}
                        {!loadingBank && filteredBank.map(q => {
                            const isLinked = bankIds.has(q.id);
                            return (
                                <Card key={q.id} variant="outlined" sx={{ borderColor: isLinked ? 'primary.main' : 'divider', opacity: isLinked ? 0.95 : 1 }}>
                                    <CardContent sx={{ py: 1.25, '&:last-child': { pb: 1.25 } }}>
                                        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 1 }}>
                                            <Typography variant='body2' fontWeight={600} noWrap title={q.title}>{q.title}</Typography>
                                            {isLinked && <Chip label='Linked' size='small' color='primary' variant='outlined' />}
                                        </Box>
                                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mt: .75, flexWrap: 'wrap' }}>
                                            <DifficultyChip value={q.difficulty} />
                                            <Chip size='small' label={q.status} color={q.status === 'Published' ? 'primary' : 'default'} variant={q.status === 'Published' ? 'outlined' : 'filled'} />
                                            <Chip size='small' label={`Max ${q.maxPoints} pts`} variant='outlined' />
                                        </Box>
                                    </CardContent>
                                    <CardActions sx={{ pt: 0, pb: 1, pr: 1, justifyContent: 'flex-end' }}>
                                        <Tooltip title={isLinked ? 'Already linked' : 'Add'}>
                                            <span>
                                                <IconButton size='small' disabled={isLinked || !assignId} onClick={() => addQuestion(q)} aria-label={`Add ${q.title}`}>
                                                    <AddIcon fontSize='inherit' />
                                                </IconButton>
                                            </span>
                                        </Tooltip>
                                    </CardActions>
                                </Card>
                            );
                        })}
                        {!loadingBank && !filteredBank.length && (
                            <Box sx={{ py: 2, textAlign: 'center' }}>
                                <Typography variant='body2' color='text.secondary'>No questions match your filter.</Typography>
                            </Box>
                        )}
                    </Box>
                </Box>

                {/* LINKED */}
                <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: 0, minWidth: { md: 420, xs: '100%' }, position: 'relative' }}>
                    <Box sx={{ position: 'sticky', top: 0, zIndex: 1, bgcolor: 'background.paper', borderBottom: '1px solid', borderColor: 'divider', mb: 1, py: 1, display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Typography variant='subtitle2'>Linked</Typography>
                            <Chip size='small' label={`${linked.length}`} />
                            <Chip size='small' color='primary' variant='outlined' label={`Total: ${totalLinkedPoints} pts`} />
                        </Box>
                    </Box>
                    <Box sx={{ flex: 1, minHeight: 0, overflow: 'auto', border: '1px solid', borderColor: 'divider', borderRadius: 1, p: 1, display: 'flex', flexDirection: 'column', gap: 1, position: 'relative' }}>
                        {loadingLinks && (
                            <Box sx={{ py: 2, display: 'flex', justifyContent: 'center' }}>
                                <CircularProgress size={20} />
                            </Box>
                        )}
                        {!loadingLinks && linked.sort((a, b) => a.position - b.position).map((l, idx) => (
                            <Card key={l.questionId} variant='outlined'>
                                <CardContent sx={{ py: 1.25, '&:last-child': { pb: 1.25 } }}>
                                    <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 1 }}>
                                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, minWidth: 0 }}>
                                            <Chip size='small' label={`#${idx + 1}`} sx={{ fontWeight: 600 }} />
                                            <Typography variant='body2' fontWeight={600} noWrap title={l.title}>{l.title}</Typography>
                                        </Box>
                                    </Box>
                                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mt: .75, flexWrap: 'wrap' }}>
                                        <DifficultyChip value={l.difficulty} />
                                        <Chip size='small' label={l.status} color={l.status === 'Published' ? 'primary' : 'default'} variant={l.status === 'Published' ? 'outlined' : 'filled'} />
                                        <Chip size='small' label={`Default ${l.maxPoints} pts`} variant='outlined' />
                                    </Box>
                                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mt: 1, justifyContent: 'space-between', flexWrap: 'wrap' }}>
                                        <TextField
                                            size='small'
                                            variant='outlined'
                                            type='number'
                                            value={pointsEdit[l.questionId] ?? ''}
                                            label='Points override'
                                            placeholder='—'
                                            aria-label={`Points override for ${l.title}`}
                                            onChange={e => {
                                                const v = e.target.value;
                                                if (v === '') { setPointsEdit(p => ({ ...p, [l.questionId]: '' })); return; }
                                                if (!/^\d+$/.test(v)) return; // numeric only
                                                const num = Number(v);
                                                if (num < 1) { setPointsEdit(p => ({ ...p, [l.questionId]: '1' })); return; }
                                                setPointsEdit(p => ({ ...p, [l.questionId]: String(num) }));
                                            }}
                                            onBlur={e => {
                                                const v = e.target.value;
                                                if (v === '') return;
                                                const num = Number(v);
                                                if (isNaN(num) || num < 1) setPointsEdit(p => ({ ...p, [l.questionId]: '1' }));
                                            }}
                                            InputProps={{ endAdornment: <InputAdornment position="end">pts</InputAdornment> }}
                                            inputProps={{ min: 1, step: 1, style: { paddingTop: 6, paddingBottom: 6 } }}
                                            sx={{ width: 180, maxWidth: '100%' }}
                                        />
                                        <Box sx={{ display: 'flex', gap: .5, ml: 'auto' }}>
                                            <Tooltip title='Move up'><span><IconButton size='small' disabled={idx === 0} onClick={() => move(l.questionId, -1)}><ArrowUpwardIcon fontSize='inherit' /></IconButton></span></Tooltip>
                                            <Tooltip title='Move down'><span><IconButton size='small' disabled={idx === linked.length - 1} onClick={() => move(l.questionId, 1)}><ArrowDownwardIcon fontSize='inherit' /></IconButton></span></Tooltip>
                                            <Tooltip title='Remove'><IconButton size='small' onClick={() => removeQuestion(l.questionId)}><DeleteIcon fontSize='inherit' /></IconButton></Tooltip>
                                        </Box>
                                    </Box>
                                </CardContent>
                            </Card>
                        ))}
                        {!loadingLinks && !linked.length && (
                            <Box sx={{ py: 2, textAlign: 'center' }}>
                                <Typography variant='body2' color='text.secondary'>No linked questions yet.</Typography>
                            </Box>
                        )}
                        {savingOrder && <LinearProgress sx={{ position: 'absolute', top: 0, left: 0, right: 0 }} />}
                    </Box>
                </Box>
            </Box>
            {!!error && <Alert severity='error' sx={{ mx: 2, mb: 1 }}>{error}</Alert>}
            <CardActions sx={{ justifyContent: 'space-between', flexWrap: 'wrap', gap: 1, px: 2, pb: 2 }}>
                <Typography variant='caption' color='text.secondary'>Changes are not saved until you click "Save changes".</Typography>
                <Box sx={{ display: 'flex', gap: 1 }}>
                    <Button size='small' variant='outlined' onClick={() => { loadBank(); loadLinked(); }}>Reload</Button>
                    {onClose && <Button size='small' onClick={onClose}>Done</Button>}
                </Box>
            </CardActions>
        </Box>
    );
});

export default AssignmentQuestionPicker;
