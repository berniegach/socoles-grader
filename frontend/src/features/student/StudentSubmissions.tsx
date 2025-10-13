'use client';
import { Typography, Button, Chip, Box, Tooltip } from '@mui/material';
import GenericTile from '@/components/GenericTile';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
import DeleteIcon from '@mui/icons-material/Delete';
import { useAuth } from '@/features/auth/AuthProvider';
import { useState, useEffect, useMemo } from 'react';
import DifficultyChip from '@/components/DifficultyChip';

// Local parse of assignment due date (mirrors logic in StudentArea)
function parseDueMs(input: string): number {
    if (!input) return NaN;
    const trimmed = input.trim();
    const m = trimmed.match(/^(\d{1,2})-(\d{1,2})-(\d{2,4})$/); // DD-MM-YYYY
    if (m) {
        const day = parseInt(m[1], 10);
        const month = parseInt(m[2], 10);
        let year = parseInt(m[3], 10);
        if (year < 100) year += 2000;
        return new Date(year, month - 1, day, 23, 59, 59, 999).getTime();
    }
    const t = Date.parse(trimmed);
    return Number.isNaN(t) ? NaN : t;
}

export default function StudentSubmissions({ rows = [] as any[], onRefresh, onOpenAssignment }: { rows?: any[]; onRefresh?: () => void; onOpenAssignment?: (assignmentTitle: string) => void }) {
    const [localRows, setLocalRows] = useState<any[]>(rows);
    const { authFetch, user } = useAuth();
    const [assignmentsMeta, setAssignmentsMeta] = useState<any[]>([]);
    const [pendingBySubmission, setPendingBySubmission] = useState<Record<string, boolean>>({});

    const formatGrade = (g: number | null | undefined) => (typeof g === 'number' && isFinite(g) ? g.toFixed(2) : '—');

    useEffect(() => { setLocalRows(rows); }, [rows]);

    // Fetch assignments metadata (difficulty, tags, points, due)
    useEffect(() => {
        if (!user?.token) return;
        let cancelled = false;
        (async () => {
            try {
                const res = await authFetch('/api/assignments');
                if (res.ok) {
                    const data = await res.json();
                    if (!cancelled) setAssignmentsMeta(Array.isArray(data) ? data : []);
                }
            } catch { /* ignore */ }
        })();
        return () => { cancelled = true; };
    }, [user?.token, authFetch]);

    // Fetch student's review requests to mark submissions under review (Pending only)
    useEffect(() => {
        if (!user?.token || !user?.name) return;
        let cancelled = false;
        (async () => {
            try {
                const res = await authFetch(`/api/review-requests?student=${encodeURIComponent(user.name)}`);
                if (!res.ok) return;
                const data = await res.json();
                const map: Record<string, boolean> = {};
                (Array.isArray(data) ? data : []).forEach((r: any) => {
                    if (r.status === 'Pending' && r.submissionId) map[r.submissionId] = true;
                });
                if (!cancelled) setPendingBySubmission(map);
            } catch { /* ignore */ }
        })();
        return () => { cancelled = true; };
    }, [user?.token, user?.name, authFetch]);

    const metaById = useMemo(() => {
        const map = new Map<string, any>();
        assignmentsMeta.forEach(a => { if (a?.id) map.set(a.id, a); });
        return map;
    }, [assignmentsMeta]);

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
            } else {
                const d = await res.json();
                alert(d?.error || 'Failed to delete');
            }
        } catch { /* ignore */ }
    }

    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            {localRows.length === 0 && (
                <Typography color="text.secondary" variant="body2">No submissions yet.</Typography>
            )}
            <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr', sm: 'repeat(auto-fill,minmax(340px,1fr))' } }}>
                {localRows.map((s: any) => {
                    const isFinished = ['Submitted', 'Auto-graded', 'Needs review'].includes(s.status);
                    const underReview = pendingBySubmission[s.id];
                    const meta = metaById.get(s.assignmentId);
                    const dueMs = parseDueMs(meta?.due);
                    const overdue = !isFinished && !Number.isNaN(dueMs) && dueMs < Date.now();

                    // Prepare grade summary: show grade and percent if possible
                    const pts = typeof meta?.points === 'number' ? meta.points : null;
                    const rawGrade = typeof s.grade === 'number' && isFinite(s.grade) ? s.grade : null;
                    const percent = rawGrade != null && pts ? ((rawGrade / pts) * 100).toFixed(0) : null;
                    const gradeLabel = rawGrade != null ? `${rawGrade.toFixed(2)}${pts ? '/' + pts : ''}${percent ? ' (' + percent + '%)' : ''}` : '—';

                    const tags: string[] = Array.isArray(meta?.tags) ? meta.tags : [];
                    const visibleTags = tags.slice(0, 3);
                    const overflow = tags.length - visibleTags.length;

                    return (
                        <GenericTile
                            key={s.id}
                            onClick={() => onOpenAssignment?.(meta?.title)}
                            sx={(theme) => ({
                                minHeight: 160,
                                borderColor: overdue ? theme.palette.error.main : undefined,
                                gridTemplateRows: 'auto auto 1fr auto'
                            })}
                        >
                            {/* Header: Title + difficulty + status chip right */}
                            <Box sx={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', gap: 1 }}>
                                <Box sx={{ flex: 1, minWidth: 0 }}>
                                    <Typography variant="subtitle2" sx={{ fontWeight: 600, lineHeight: 1.2, pr: 1, overflow: 'hidden', textOverflow: 'ellipsis' }}>{meta?.title ?? '—'}</Typography>
                                    <Typography variant="caption" color="text.secondary">{isFinished ? 'Submitted' : 'Started'}: {formatDateTimeDDMMYYYYHHmm(s.date)}</Typography>
                                </Box>
                                <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-end', gap: 0.5 }}>
                                    {meta?.difficulty && <DifficultyChip value={meta.difficulty} size='small' />}
                                    <Chip size="small" label={underReview ? 'Under Review' : s.status} color={underReview ? 'warning' : s.status === 'Auto-graded' ? 'primary' : (isFinished ? 'success' : overdue ? 'error' : 'default')} variant={underReview || s.status === 'Auto-graded' ? 'outlined' : 'filled'} />
                                </Box>
                            </Box>

                            {/* Meta line */}
                            <Box sx={{ display: 'flex', flexWrap: 'wrap', columnGap: 1.2, rowGap: 0.5, alignItems: 'center' }}>
                                <Typography variant="caption" color={overdue ? 'error.main' : 'text.secondary'}>
                                    Due: {meta?.due ? meta.due : '—'}{overdue ? ' (overdue)' : ''}
                                </Typography>
                                {pts != null && (
                                    <Typography variant="caption" color="text.secondary">Points: {pts}</Typography>
                                )}
                                <Typography variant="caption" color="text.secondary">Grade: {gradeLabel}</Typography>
                            </Box>

                            {/* Tags block */}
                            {visibleTags.length > 0 && (
                                <Box sx={{ display: 'flex', justifyContent: 'flex-start', flexWrap: 'wrap', gap: 0.6 }}>
                                    {visibleTags.map(t => <Chip key={t} size='small' label={t} variant='outlined' />)}
                                    {overflow > 0 && (
                                        <Tooltip title={tags.slice(3).join(', ')} placement='top'>
                                            <Chip size='small' label={`+${overflow}`} variant='outlined' />
                                        </Tooltip>
                                    )}
                                </Box>
                            )}

                            {/* Bottom row: left = status summary, right = action */}
                            <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mt: 0.25 }}>
                                <Typography variant="caption" color="text.secondary">
                                    {underReview ? 'Under Review' : overdue ? 'Overdue' : isFinished ? 'Finished' : s.status || 'In Progress'}
                                </Typography>
                                {s.status === 'In Progress' && (
                                    <Tooltip title="Delete in-progress submission">
                                        <Button size="small" color="error" variant="outlined" onClick={(e) => { e.stopPropagation(); handleDelete(s.id); }}>Delete</Button>
                                    </Tooltip>
                                )}
                            </Box>
                        </GenericTile>
                    );
                })}
            </Box>
        </Box>
    );
}
