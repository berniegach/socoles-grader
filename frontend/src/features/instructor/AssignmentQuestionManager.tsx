'use client';
import { useEffect, useState, useMemo } from 'react';
import {
    Box,
    Card,
    CardHeader,
    CardContent,
    CardActions,
    Typography,
    TextField,
    Button,
    IconButton,
    Chip,
    Divider,
    MenuItem,
    CircularProgress,
    Tooltip,
    Snackbar,
    Alert,
    Dialog,
    DialogTitle,
    DialogContent,
    DialogActions,
    LinearProgress,
    CardActionArea,
} from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import RefreshIcon from '@mui/icons-material/Refresh';
import ManageSearchIcon from '@mui/icons-material/ManageSearch';
import type { Assignment, AssignmentWithQuestions, NewAssignmentPayload } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';
import AssignmentQuestionPicker from './AssignmentQuestionPicker';
import { LocalizationProvider, DatePicker } from '@mui/x-date-pickers';
import { AdapterDayjs } from '@mui/x-date-pickers/AdapterDayjs';
import dayjs, { Dayjs } from 'dayjs';
import VisibilityIcon from '@mui/icons-material/Visibility';
import StudentAssignmentPlayer from '@/features/student/StudentAssignmentPlayer';
import { useTheme, alpha } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import { ListItemIcon, ListItemText } from '@mui/material';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import PreviewIcon from '@mui/icons-material/Visibility';
import EditIcon from '@mui/icons-material/Edit';
import DeleteForeverIcon from '@mui/icons-material/DeleteForever';
import Menu from '@mui/material/Menu';
import PageCard from '@/components/PageCard';

// Helper (kept for potential future use; currently not used)
async function fetchJson(url: string, init?: RequestInit) {
    const res = await fetch(url, init);
    const data = await res.json();
    if (!res.ok) throw new Error(data?.error || 'Request failed');
    return data;
}

export default function AssignmentQuestionManager() {
    const { authFetch, user } = useAuth();
    const [assignments, setAssignments] = useState<Assignment[]>([]);
    const [loading, setLoading] = useState(false);
    const [creating, setCreating] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [note, setNote] = useState<string | null>(null);
    const [selectedId, setSelectedId] = useState<string | null>(null); // card click selection
    const [actionTargetId, setActionTargetId] = useState<string | null>(null); // overflow menu target
    const [editingId, setEditingId] = useState<string | null>(null);
    const [actionAnchor, setActionAnchor] = useState<null | HTMLElement>(null);
    const [previewOpen, setPreviewOpen] = useState(false);
    const [previewLoading, setPreviewLoading] = useState(false);
    const [previewAssignment, setPreviewAssignment] = useState<Assignment | AssignmentWithQuestions | null>(null);
    // Inline editor view (mirror QuestionManager style)
    const [view, setView] = useState<'list' | 'editor'>('list');

    // New assignment form
    const [title, setTitle] = useState('');
    const [course, setCourse] = useState(() => {
        // Pick default course from Instructor Settings stored in localStorage
        try {
            const raw = typeof window !== 'undefined' ? localStorage.getItem('sqlgrader.settings') : null;
            if (raw) {
                const parsed = JSON.parse(raw);
                const name = (parsed?.courseName || '').toString().trim();
                if (name) return name;
            }
        } catch { /* ignore */ }
        return 'DB201 — Intermediate SQL';
    });
    const [difficulty, setDifficulty] = useState('Beginner');
    const [points, setPoints] = useState(0);
    const [due, setDue] = useState('');
    const [tags, setTags] = useState('');
    const [filter, setFilter] = useState('');
    const [attemptsAllowed, setAttemptsAllowed] = useState<number>(3);

    const selected = useMemo<Assignment | AssignmentWithQuestions | null>(() => assignments.find(a => a.id === selectedId) || null, [assignments, selectedId]);

    async function loadAssignments(includeQuestions = false) {
        if (!user?.token) return; // wait for auth
        setLoading(true); setError(null);
        try {
            const res = await authFetch(`/api/assignments${includeQuestions ? '?include=questions' : ''}`);
            if (!res.ok) {
                const err = await res.json().catch(() => ({}));
                throw new Error(err?.error || 'Failed to load assignments');
            }
            const data = await res.json();
            setAssignments(data);
        } catch (e: any) { setError(e.message); } finally { setLoading(false); }
    }

    useEffect(() => { loadAssignments(); }, [user?.token]); // reload when auth ready

    async function createAssignment() {
        setCreating(true); setError(null);
        const payload: NewAssignmentPayload = {
            title: title.trim(),
            course: course.trim() || 'Course',
            difficulty: difficulty || 'Beginner',
            points: Number(points) || 0,
            due: due || '',
            tags: tags.split(',').map(t => t.trim()).filter(Boolean),
            attemptsAllowed: Math.max(1, Math.floor(Number(attemptsAllowed) || 1)),
        };
        if (!payload.title) { setError('Title required'); setCreating(false); return; }
        try {
            if (!user?.token) throw new Error('Not authenticated');
            const res = await authFetch('/api/assignments', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) });
            if (!res.ok) {
                const err = await res.json().catch(() => ({}));
                throw new Error(err?.error || 'Create failed');
            }
            const a = await res.json();
            setAssignments(prev => [a, ...prev]);
            setSelectedId(a.id);
            setTitle(''); setTags(''); setPoints(0); setDue(''); setAttemptsAllowed(3);
            setNote('Assignment created');
            setView('list');
        } catch (e: any) { setError(e.message); } finally { setCreating(false); }
    }
    function startCreate() {
        setTitle('');
        // Pull the course from saved Instructor Settings each time we create
        try {
            const raw = typeof window !== 'undefined' ? localStorage.getItem('sqlgrader.settings') : null;
            if (raw) {
                const parsed = JSON.parse(raw);
                const name = (parsed?.courseName || '').toString().trim();
                setCourse(name || 'DB201 — Intermediate SQL');
            } else {
                setCourse('DB201 — Intermediate SQL');
            }
        } catch { setCourse('DB201 — Intermediate SQL'); }
        setDifficulty('Beginner');
        setPoints(0);
        setDue('');
        setTags('');
        setAttemptsAllowed(3);
        setError(null);
        setSelectedId(null);
        setView('editor');
    }
    async function startEdit(a: Assignment) {
        setEditingId(a.id);
        setTitle(a.title || '');
        setCourse(a.course || '');
        setDifficulty(a.difficulty || 'Beginner');
        setPoints(a.points || 0);
        setDue(a.due || '');
        setTags((a.tags || []).join(', '));
        setAttemptsAllowed((a as any).attemptsAllowed ?? 3);
        setError(null);
        setSelectedId(null);
        setView('editor');
    }
    async function saveEdit() {
        if (!editingId) return;
        try {
            if (!user?.token) throw new Error('Not authenticated');
            const res = await authFetch('/api/assignments', {
                method: 'PATCH',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    id: editingId,
                    title: title.trim() || 'Untitled',
                    points: Number(points) || 0,
                    due: due || '',
                    difficulty: difficulty || 'Beginner',
                    tags: tags.split(',').map(t => t.trim()).filter(Boolean),
                    course: course || 'Course',
                    attemptsAllowed: Math.max(1, Math.floor(Number(attemptsAllowed) || 1))
                })
            });
            if (!res.ok) {
                const err = await res.json().catch(() => ({}));
                throw new Error(err?.error || 'Update failed');
            }
            const updated = await res.json();
            setAssignments(list => list.map(a => a.id === updated.id ? updated : a));
            setNote('Updated');
            setEditingId(null);
            setView('list');
        } catch (e: any) { setError(e.message); }
    }
    async function deleteAssignment(id: string) {
        if (!confirm('Delete assignment? This will unlink its questions.')) return;
        try {
            if (!user?.token) throw new Error('Not authenticated');
            const res = await authFetch(`/api/assignments?id=${id}`, { method: 'DELETE' });
            if (!res.ok) {
                const err = await res.json().catch(() => ({}));
                throw new Error(err?.error || 'Delete failed');
            }
            setAssignments(list => list.filter(a => a.id !== id));
            if (selectedId === id) setSelectedId(null);
            setNote('Deleted');
        } catch (e: any) { setError(e.message); }
    }

    const dueDayjs: Dayjs | null = due ? dayjs(due, 'DD-MM-YYYY') : null;

    const filteredAssignments = useMemo(() => {
        if (!filter.trim()) return assignments;
        const q = filter.toLowerCase();
        return assignments.filter(a => (
            a.title.toLowerCase().includes(q) ||
            (a.course || '').toLowerCase().includes(q) ||
            (a.tags || []).some(t => t.toLowerCase().includes(q))
        ));
    }, [assignments, filter]);

    const theme = useTheme();

    const openActions = (e: React.MouseEvent<HTMLButtonElement>, assignment: Assignment) => {
        e.stopPropagation(); // prevent card navigation
        setActionTargetId(assignment.id);
        setActionAnchor(e.currentTarget);
    };
    const closeActions = () => { setActionAnchor(null); setActionTargetId(null); };

    async function openAssignmentPreview(a: Assignment) {
        // Do NOT change selectedId; preview should not navigate away from list
        setPreviewOpen(true);
        setPreviewLoading(true);
        try {
            const res = await authFetch(`/api/assignments/${a.id}`);
            if (res.ok) {
                setPreviewAssignment(await res.json());
            } else setPreviewAssignment(a);
        } catch { setPreviewAssignment(a); } finally { setPreviewLoading(false); }
    }

    return (
        <LocalizationProvider dateAdapter={AdapterDayjs} adapterLocale='en-gb'>
            {!selected ? (
                view === 'editor' ? (
                    <PageCard>
                        <Card>
                            <CardHeader
                                sx={{ pb: 1 }}
                                title={<Typography variant='subtitle1' fontWeight={600}>{editingId ? 'Edit Assignment' : 'Create Assignment'}</Typography>}
                                action={<Box sx={{ display: 'flex', gap: 1 }}>
                                    <Button variant='outlined' size='small' onClick={() => setView('list')}>Back to list</Button>
                                    <Button startIcon={<AddIcon />} variant='contained' size='small' onClick={startCreate}>New</Button>
                                </Box>}
                            />
                            <CardContent sx={{ pt: 0 }}>
                                <Box sx={{ display: 'grid', gap: 1.5, gridTemplateColumns: { sm: 'repeat(2,1fr)', xs: '1fr' } }}>
                                    <TextField label='Title' value={title} onChange={e => setTitle(e.target.value)} variant='outlined' size='small' fullWidth required />
                                    <TextField label='Course' value={course} onChange={e => setCourse(e.target.value)} variant='outlined' size='small' fullWidth />
                                    <TextField label='Difficulty' value={difficulty} onChange={e => setDifficulty(e.target.value)} select size='small'>
                                        {['Beginner', 'Intermediate', 'Advanced'].map(d => <MenuItem key={d} value={d}>{d}</MenuItem>)}
                                    </TextField>
                                    <TextField label='Points' type='number' value={points} onChange={e => setPoints(Number(e.target.value || 0))} variant='outlined' size='small' />
                                    <DatePicker label='Due' value={dueDayjs} onChange={(v: Dayjs | null) => setDue(v ? v.format('DD-MM-YYYY') : '')} format='DD-MM-YYYY' slotProps={{ textField: { size: 'small', variant: 'outlined', placeholder: '31-12-2025' } }} />
                                    <TextField label='Tags (comma)' value={tags} onChange={e => setTags(e.target.value)} variant='outlined' size='small' />
                                    <TextField label='Attempts allowed' type='number' value={attemptsAllowed} onChange={e => setAttemptsAllowed(Math.max(1, Math.floor(Number(e.target.value) || 1)))} variant='outlined' size='small' inputProps={{ min: 1, step: 1 }} />
                                </Box>
                                {error && <Typography variant='caption' color='error' sx={{ mt: 1, display: 'block' }}>{error}</Typography>}
                            </CardContent>
                            <CardActions sx={{ justifyContent: 'space-between' }}>
                                <Box />
                                <Box sx={{ display: 'flex', gap: 1 }}>
                                    <Button variant='outlined' size='small' onClick={() => { setEditingId(null); setView('list'); }}>Cancel</Button>
                                    {editingId ? (
                                        <Button variant='contained' size='small' onClick={saveEdit} disabled={!title.trim()}>Update</Button>
                                    ) : (
                                        <Button variant='contained' size='small' onClick={createAssignment} disabled={creating || !title.trim()}>{creating ? 'Creating…' : 'Create'}</Button>
                                    )}
                                </Box>
                            </CardActions>
                        </Card>
                    </PageCard>
                ) : (
                    <PageCard>
                        <CardHeader title={<Typography variant='subtitle1'>Assignments</Typography>} />
                        <CardContent>
                            <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', mb: 2, flexWrap: 'wrap' }}>
                                <TextField
                                    label='Search assignments'
                                    placeholder='Title, course, tag'
                                    value={filter}
                                    onChange={e => setFilter(e.target.value)}
                                    variant='outlined'
                                    size='small'
                                />
                                <Button startIcon={<AddIcon />} variant='contained' size='small' onClick={startCreate}>Create assignment</Button>
                                <Tooltip title="Reload"><span><IconButton onClick={() => loadAssignments()} disabled={loading}><RefreshIcon /></IconButton></span></Tooltip>
                            </Box>
                            {loading && <Box sx={{ py: 3, textAlign: 'center' }}><CircularProgress size={26} /></Box>}
                            {!loading && (
                                <Box sx={{
                                    display: 'grid', gap: 1.5,
                                    gridTemplateColumns: { xs: '1fr', sm: 'repeat(2, 1fr)', md: 'repeat(3, 1fr)', lg: 'repeat(4, 1fr)' }
                                }}>
                                    {filteredAssignments.map(a => (
                                        <Card key={a.id} variant='outlined' sx={{ display: 'flex', flexDirection: 'column', position: 'relative' }}>
                                            <CardActionArea onClick={() => setSelectedId(a.id)} sx={{ alignSelf: 'stretch' }}>
                                                <CardHeader
                                                    title={<Typography variant='subtitle1' sx={{ fontWeight: 600, lineHeight: 1.2 }}>{a.title}</Typography>}
                                                    subheader={<Typography variant='caption' color='text.secondary'>{a.course}</Typography>}
                                                />
                                                <CardContent sx={{ pt: 0, display: 'grid', gap: 1 }}>
                                                    <Box sx={{ display: 'flex', gap: .75, flexWrap: 'wrap' }}>
                                                        <Chip size='small' label={a.difficulty} />
                                                        <Chip size='small' label={`${a.points} pts`} />
                                                        <Chip size='small' variant='outlined' label={`Attempts: ${(a as any).attemptsAllowed ?? 3}`} />
                                                    </Box>
                                                    <Typography variant='caption' color='text.secondary'>Due: {a.due || '—'}</Typography>
                                                </CardContent>
                                            </CardActionArea>
                                            <Box sx={{ position: 'absolute', top: 4, right: 4 }}>
                                                <Tooltip title='Actions'>
                                                    <IconButton size='small' onClick={(e) => openActions(e, a)} aria-label={`Actions for ${a.title}`}>
                                                        <MoreVertIcon fontSize='small' />
                                                    </IconButton>
                                                </Tooltip>
                                            </Box>
                                        </Card>
                                    ))}
                                </Box>
                            )}
                            {!loading && !filteredAssignments.length && (
                                <Typography variant='body2' color='text.secondary' sx={{ mt: 2 }}>No assignments found.</Typography>
                            )}
                            {error && <Typography variant='caption' color='error' sx={{ mt: 1 }}>{error}</Typography>}
                        </CardContent>
                        {/* Overflow actions menu (stays outside grid mapping) */}
                        <Menu
                            anchorEl={actionAnchor}
                            open={!!actionAnchor}
                            onClose={closeActions}
                            keepMounted
                        >
                            <MenuItem
                                onClick={() => {
                                    const target = assignments.find(a => a.id === actionTargetId);
                                    closeActions();
                                    if (target) openAssignmentPreview(target);
                                }}
                            >
                                <ListItemIcon><PreviewIcon fontSize='small' /></ListItemIcon>
                                <ListItemText>Preview as student</ListItemText>
                            </MenuItem>
                            <MenuItem
                                onClick={() => {
                                    const target = assignments.find(a => a.id === actionTargetId);
                                    closeActions();
                                    if (target) startEdit(target);
                                }}
                            >
                                <ListItemIcon><EditIcon fontSize='small' /></ListItemIcon>
                                <ListItemText>Edit details</ListItemText>
                            </MenuItem>
                            <MenuItem
                                onClick={() => {
                                    const id = actionTargetId;
                                    closeActions();
                                    if (id) deleteAssignment(id);
                                }}
                            >
                                <ListItemIcon><DeleteForeverIcon fontSize='small' color='error' /></ListItemIcon>
                                <ListItemText primaryTypographyProps={{ color: 'error.main' }}>Delete</ListItemText>
                            </MenuItem>
                        </Menu>
                    </PageCard>
                )
            ) : (
                <>
                    <AssignmentQuestionPicker assignment={selected} onClose={() => setSelectedId(null)} />
                </>
            )}

            <Snackbar open={!!note} autoHideDuration={2800} onClose={() => setNote(null)} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                {!!note ? <Alert severity='success' variant='filled' onClose={() => setNote(null)}>{note}</Alert> : undefined}
            </Snackbar>


            {/* Preview dialog */}
            <Dialog open={previewOpen} onClose={() => setPreviewOpen(false)} fullScreen={useMediaQuery(theme.breakpoints.down('md'))} fullWidth maxWidth='lg'>
                <DialogTitle>Preview Assignment as Student{previewAssignment?.title ? `: ${previewAssignment.title}` : ''}</DialogTitle>
                <DialogContent dividers sx={{ p: 0 }}>
                    {previewLoading && <LinearProgress />}
                    {!previewLoading && (
                        <Box sx={{ height: 'calc(100vh - 200px)', overflow: 'auto', p: 2 }}>
                            <StudentAssignmentPlayer assignmentId={previewAssignment?.id || selectedId} onClose={() => setPreviewOpen(false)} previewMode />
                        </Box>
                    )}
                </DialogContent>
                <DialogActions>
                    <Button onClick={() => setPreviewOpen(false)}>Close</Button>
                </DialogActions>
            </Dialog>

        </LocalizationProvider>
    );
}
