'use client';
import {
    Typography, Table, TableHead, TableRow, TableCell, TableBody, Button, Chip,
    Box
} from '@mui/material';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
import VisibilityIcon from '@mui/icons-material/Visibility';
import DeleteIcon from '@mui/icons-material/Delete';
import { useAuth } from '@/features/auth/AuthProvider';
import { useState, useEffect } from 'react';

export default function StudentSubmissions({ rows = [] as any[], onRefresh, onOpenAssignment }: { rows?: any[]; onRefresh?: () => void; onOpenAssignment?: (assignmentTitle: string) => void }) {
    const [localRows, setLocalRows] = useState<any[]>(rows);
    const { authFetch, user } = useAuth();

    const formatGrade = (g: number | null | undefined) => (typeof g === 'number' && isFinite(g) ? g.toFixed(2) : '—');

    useEffect(() => { setLocalRows(rows); }, [rows]);

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
        <Box sx={{ display: 'grid', gap: 1.5 }}>
            <Box sx={{ border: '1px solid', borderColor: 'divider', borderRadius: 1, overflow: 'hidden' }}>
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
                            <TableRow key={s.id} hover>
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
                                        onClick={() => onOpenAssignment?.(s.assignment)}
                                        sx={{ mr: 1 }}
                                    >View</Button>
                                    {s.status === 'In Progress' && (
                                        <Button size="small" color="error" variant="outlined" startIcon={<DeleteIcon />} onClick={() => handleDelete(s.id)}>Delete</Button>
                                    )}
                                </TableCell>
                            </TableRow>
                        ))}
                        {!localRows.length && (
                            <TableRow><TableCell colSpan={5}><Typography color="text.secondary">No submissions yet.</Typography></TableCell></TableRow>
                        )}
                    </TableBody>
                </Table>
            </Box>
        </Box>
    );
}
