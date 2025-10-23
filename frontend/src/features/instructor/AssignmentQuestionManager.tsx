'use client';
import { useEffect, useState, useMemo } from 'react';
import {
    Box,
    Typography,
    TextField,
    Button,
    IconButton,
    Chip,
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
    FormControlLabel,
    Switch,
} from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import RefreshIcon from '@mui/icons-material/Refresh';
import type { Assignment, AssignmentWithQuestions, NewAssignmentPayload } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';
import AssignmentQuestionPicker, { AssignmentQuestionPickerHandle } from './AssignmentQuestionPicker';
import { LocalizationProvider, DatePicker } from '@mui/x-date-pickers';
import { AdapterDayjs } from '@mui/x-date-pickers/AdapterDayjs';
import dayjs, { Dayjs } from 'dayjs';
import StudentAssignmentPlayer from '@/features/student/StudentAssignmentPlayer';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import { ListItemIcon, ListItemText } from '@mui/material';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import PreviewIcon from '@mui/icons-material/Visibility';
import EditIcon from '@mui/icons-material/Edit';
import DeleteForeverIcon from '@mui/icons-material/DeleteForever';
import PublishIcon from '@mui/icons-material/Publish';
import UnpublishedIcon from '@mui/icons-material/Unpublished';
import Menu from '@mui/material/Menu';
import PageCard from '@/components/PageCard';
import TileCard from '@/components/TileCard';
import TilesGrid from '@/components/TilesGrid';
import HeaderActionButton from '@/components/HeaderActionButton';
import HeaderActions from '@/components/HeaderActions';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import SaveIcon from '@mui/icons-material/Save';
import DifficultyChip from '@/components/DifficultyChip';


export default function AssignmentQuestionManager() {
    const { authFetch, user } = useAuth();
    const [assignments, setAssignments] = useState<Assignment[]>([]);
    const [loading, setLoading] = useState(false);
    const [creating, setCreating] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [note, setNote] = useState<string | null>(null);
    const [selectedId, setSelectedId] = useState<string | null>(null); // selected assignment for linking questions
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
    const [course, setCourse] = useState('DB201 — Intermediate SQL');
    useEffect(() => {
        (async () => {
            try {
                const res = await authFetch('/api/instructor/settings', { method: 'GET' });
                if (res.ok) {
                    const data = await res.json();
                    if (data.course_name) setCourse(data.course_name);
                }
            } catch { /* ignore */ }
        })();
    }, [authFetch]);
    const [difficulty, setDifficulty] = useState('Beginner');
    const [points, setPoints] = useState(0);
    const [due, setDue] = useState('');
    const [tags, setTags] = useState('');
    const [filter, setFilter] = useState('');
    const [attemptsAllowed, setAttemptsAllowed] = useState<number>(3);
    const [published, setPublished] = useState(false);

    const selected = useMemo<Assignment | AssignmentWithQuestions | null>(() => assignments.find(a => a.id === selectedId) || null, [assignments, selectedId]);
    const [pickerMode, setPickerMode] = useState(false);
    const pickerRef = useState<AssignmentQuestionPickerHandle | null>(null)[0] as any;

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
            published,
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
            setTitle(''); setTags(''); setPoints(0); setDue(''); setAttemptsAllowed(3); setPublished(false);
            setNote('Assignment created');
            setView('list');
        } catch (e: any) { setError(e.message); } finally { setCreating(false); }
    }
    function startCreate() {
        setTitle('');
        // Always fetch latest course name from backend
        (async () => {
            try {
                const res = await authFetch('/api/instructor/settings', { method: 'GET' });
                if (res.ok) {
                    const data = await res.json();
                    setCourse((data.course_name || 'DB201 — Intermediate SQL').toString().trim());
                } else {
                    setCourse('DB201 — Intermediate SQL');
                }
            } catch { setCourse('DB201 — Intermediate SQL'); }
        })();
        setDifficulty('Beginner');
        setPoints(0);
        setDue('');
        setTags('');
        setAttemptsAllowed(3);
        setPublished(false);
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
        setPublished(!!(a as any).published);
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
                    attemptsAllowed: Math.max(1, Math.floor(Number(attemptsAllowed) || 1)),
                    published
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

    async function togglePublishedState(target: Assignment, next: boolean) {
        try {
            if (!user?.token) throw new Error('Not authenticated');
            setError(null);
            const res = await authFetch('/api/assignments', {
                method: 'PATCH',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ id: target.id, published: next })
            });
            if (!res.ok) {
                const err = await res.json().catch(() => ({}));
                throw new Error(err?.error || 'Failed to update publish state');
            }
            const updated = await res.json();
            setAssignments(list => list.map(a => a.id === updated.id ? updated : a));
            setNote(updated.published ? 'Assignment published' : 'Assignment unpublished');
        } catch (e: any) {
            setError(e.message);
        }
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

    const headerActions = (
        <>
            {view === 'list' && !pickerMode && (
                <HeaderActions
                    actions={[
                        { key: 'add', ariaLabel: 'Add assignment', title: 'Add assignment', icon: <AddIcon fontSize='small' />, onClick: startCreate },
                        { key: 'reload', ariaLabel: 'Reload assignments', title: 'Reload assignments', icon: <RefreshIcon fontSize='small' />, onClick: () => loadAssignments(), disabled: loading }
                    ]}
                />
            )}
            {view === 'editor' && !pickerMode && (
                <HeaderActionButton onClick={() => { setView('list'); setEditingId(null); }} aria-label='Back to list' title='Back to list'>
                    <ArrowBackIcon fontSize='small' />
                </HeaderActionButton>
            )}
            {pickerMode && (
                <HeaderActions
                    actions={[
                        { key: 'back', ariaLabel: 'Back to assignments', title: 'Back to assignments', icon: <ArrowBackIcon fontSize='small' />, onClick: () => setPickerMode(false) },
                        { key: 'save', ariaLabel: 'Save changes', title: 'Save changes', icon: <SaveIcon fontSize='small' />, onClick: () => pickerRef?.save?.() }
                    ]}
                />
            )}
        </>
    );

    const pageTitle = pickerMode ? `Assignment Questions${selected ? ': ' + selected.title : ''}` : (view === 'editor' ? (editingId ? 'Edit Assignment' : 'Create Assignment') : 'Assignments');
    const actionTarget = useMemo(() => assignments.find(a => a.id === actionTargetId) || null, [actionTargetId, assignments]);

    return (
        <LocalizationProvider dateAdapter={AdapterDayjs} adapterLocale='en-gb'>
            <PageCard
                headerTitle={pageTitle}
                headerProps={{ height: 56 }}
                headerActions={headerActions}
                headerActionsVariant='plain'
            >
                {pickerMode ? (
                    <AssignmentQuestionPicker assignment={selected} ref={pickerRef} onClose={() => setPickerMode(false)} />
                ) : view === 'editor' ? (
                    <Box sx={{ p: { xs: 1.5, md: 2 }, borderRadius: 2, bgcolor: 'transparent' }}>
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
                        <FormControlLabel
                            control={<Switch checked={published} onChange={(_, v) => setPublished(v)} size='small' />}
                            label={published ? 'Published' : 'Draft'}
                            sx={{ mt: 1 }}
                        />
                        {error && <Typography variant='caption' color='error' sx={{ mt: 1, display: 'block' }}>{error}</Typography>}
                        <Box sx={{ mt: 2, display: 'flex', justifyContent: 'flex-end', gap: 1, flexWrap: 'wrap' }}>
                            <Button variant='outlined' size='small' onClick={() => { setEditingId(null); setView('list'); }}>Cancel</Button>
                            {editingId ? (
                                <Button variant='contained' size='small' onClick={saveEdit} disabled={!title.trim()}>Update</Button>
                            ) : (
                                <Button variant='contained' size='small' onClick={createAssignment} disabled={creating || !title.trim()}>{creating ? 'Creating…' : 'Create'}</Button>
                            )}
                        </Box>
                    </Box>
                ) : (
                    <Box sx={{ p: { xs: 1.5, md: 2 }, pt: 2 }}>
                        <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', mb: 2, flexWrap: 'wrap' }}>
                            <TextField
                                label='Search assignments'
                                placeholder='Title, course, tag'
                                value={filter}
                                onChange={e => setFilter(e.target.value)}
                                variant='outlined'
                                size='small'
                            />
                        </Box>
                        {loading && <Box sx={{ py: 3, textAlign: 'center' }}><CircularProgress size={26} /></Box>}
                        {!loading && (
                            <TilesGrid>
                                {filteredAssignments.map(a => (
                                    <TileCard
                                        key={a.id}
                                        title={a.title}
                                        subtitle={a.course}
                                        chips={[
                                            <Chip key="published" size="small" label={a.published ? 'Published' : 'Draft'} color={a.published ? 'success' : 'default'} variant={a.published ? 'filled' : 'outlined'} />,
                                            <DifficultyChip key="difficulty" value={a.difficulty} />,
                                            <Chip key="points" size="small" label={`${a.points} pts`} variant="outlined" />,
                                            <Chip key="attempts" size="small" label={`Attempts: ${(a as any).attemptsAllowed ?? 3}`} variant="outlined" />,
                                            <Chip key="due" size="small" label={`Due: ${a.due || '—'}`} variant="outlined" />
                                        ]}
                                        onClick={() => { setSelectedId(a.id); setPickerMode(true); }}
                                        tabIndex={0}
                                        role="button"
                                        onKeyDown={(e: React.KeyboardEvent) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); setSelectedId(a.id); setPickerMode(true); } }}
                                        headerRight={(
                                            <Tooltip title='Actions'>
                                                <IconButton size='small' onClick={(e) => { e.stopPropagation(); openActions(e, a); }} aria-label={`Actions for ${a.title}`}>
                                                    <MoreVertIcon fontSize='small' />
                                                </IconButton>
                                            </Tooltip>
                                        )}
                                    />
                                ))}
                            </TilesGrid>
                        )}
                        {!loading && !filteredAssignments.length && (
                            <Typography variant='body2' color='text.secondary' sx={{ mt: 2 }}>No assignments found.</Typography>
                        )}
                        {error && <Typography variant='caption' color='error' sx={{ mt: 1 }}>{error}</Typography>}
                        {/* Overflow actions menu (stays outside grid mapping) */}
                        <Menu
                            anchorEl={actionAnchor}
                            open={!!actionAnchor}
                            onClose={closeActions}
                            keepMounted
                        >
                            <MenuItem
                                onClick={() => {
                                    closeActions();
                                    if (actionTarget) togglePublishedState(actionTarget, !actionTarget.published);
                                }}
                            >
                                <ListItemIcon>{actionTarget?.published ? <UnpublishedIcon fontSize='small' /> : <PublishIcon fontSize='small' />}</ListItemIcon>
                                <ListItemText>{actionTarget?.published ? 'Unpublish' : 'Publish'}</ListItemText>
                            </MenuItem>
                            <MenuItem
                                onClick={() => {
                                    closeActions();
                                    if (actionTarget) openAssignmentPreview(actionTarget);
                                }}
                            >
                                <ListItemIcon><PreviewIcon fontSize='small' /></ListItemIcon>
                                <ListItemText>Preview as student</ListItemText>
                            </MenuItem>
                            <MenuItem
                                onClick={() => {
                                    closeActions();
                                    if (actionTarget) startEdit(actionTarget);
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
                    </Box>
                )}
            </PageCard>

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
