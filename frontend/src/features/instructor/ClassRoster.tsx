'use client';
import { useEffect, useState, useRef } from 'react';
import { Box, Button, Card, CardContent, Chip, LinearProgress, Snackbar, Alert, Table, TableBody, TableCell, TableHead, TableRow, TextField, Stack, IconButton, Tooltip, Typography, Switch, FormControlLabel } from '@mui/material';
import RefreshIcon from '@mui/icons-material/Refresh';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import PersonAddIcon from '@mui/icons-material/PersonAdd';
import DeleteIcon from '@mui/icons-material/Delete';
import LinkIcon from '@mui/icons-material/Link';
import CancelIcon from '@mui/icons-material/CancelOutlined';
import Papa from 'papaparse';
import PageCard from '@/components/PageCard';
import HeaderActions from '@/components/HeaderActions';
import { useAuth } from '@/features/auth/AuthProvider';
import type { RosterEntry } from '@/lib/types';

export default function ClassRoster() {
    const { authFetch } = useAuth();
    const [rows, setRows] = useState<RosterEntry[]>([]);
    const [busy, setBusy] = useState(false);
    const [note, setNote] = useState<string>('');
    const [err, setErr] = useState<string>('');

    // manual add form
    const [name, setName] = useState('');
    const [email, setEmail] = useState('');
    // hidden file input ref for import CSV
    const inputRef = useRef<HTMLInputElement | null>(null);

    async function load() {
        try {
            setBusy(true);
            const res = await authFetch('/api/roster');
            const data = await res.json();
            if (Array.isArray(data)) setRows(data);
        } catch (e: any) { setErr(e.message || 'Failed to load'); } finally { setBusy(false); }
    }
    // Initial load
    useEffect(() => { load(); }, []);

    // Polling & focus/visibility-based refresh
    const pollRef = useRef<number | null>(null);
    const stopPolling = () => { if (pollRef.current) { clearInterval(pollRef.current); pollRef.current = null; } };
    const startPolling = () => {
        stopPolling();
        // Poll every 20s while component is mounted & tab visible
        pollRef.current = window.setInterval(() => {
            if (document.visibilityState === 'visible') {
                load();
            }
        }, 20000);
    };
    useEffect(() => {
        startPolling();
        function onVisibility() { if (document.visibilityState === 'visible') load(); }
        function onFocus() { load(); }
        window.addEventListener('visibilitychange', onVisibility);
        window.addEventListener('focus', onFocus);
        return () => {
            stopPolling();
            window.removeEventListener('visibilitychange', onVisibility);
            window.removeEventListener('focus', onFocus);
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    async function addOne() {
        if (!name.trim() || !email.trim()) { setErr('Name and email required'); return; }
        try {
            setBusy(true);
            const res = await authFetch('/api/roster', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name: name.trim(), email: email.trim(), status: 'Invited' }) });
            const data = await res.json();
            if (Array.isArray(data) && data.length) setRows((r) => [...data, ...r]); else if (data?.id) setRows((r) => [data, ...r]);
            setName(''); setEmail(''); setNote('Added');
        } catch (e: any) { setErr(e.message || 'Add failed'); } finally { setBusy(false); }
    }

    async function remove(id: string) {
        try {
            setBusy(true);
            const res = await authFetch(`/api/roster?id=${encodeURIComponent(id)}`, { method: 'DELETE' });
            if (res.ok) setRows((r) => r.filter((x) => x.id !== id)); else setErr('Delete failed');
        } catch (e: any) { setErr(e.message || 'Delete failed'); } finally { setBusy(false); }
    }

    async function onCsv(evt: React.ChangeEvent<HTMLInputElement>) {
        const file = evt.target.files?.[0];
        if (!file) return;
        setBusy(true);
        setErr('');
        Papa.parse(file, {
            header: true,
            skipEmptyLines: true,
            complete: async (results: { data: Array<Record<string, unknown>> }) => {
                try {
                    const records = (results.data || [])
                        .map((r) => ({ name: String((r as any).name || (r as any).Name || '').trim(), email: String((r as any).email || (r as any).Email || '').trim() }))
                        .filter((r) => r.name && r.email);
                    if (!records.length) { setErr('No valid rows (need "name" and "email" columns)'); return; }
                    const res = await authFetch('/api/roster', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ entries: records }) });
                    const data = await res.json();
                    if (Array.isArray(data)) setRows((r) => [...data, ...r]); else setErr('Import failed');
                    setNote(`Imported ${Array.isArray(data) ? data.length : 0} students`);
                } catch (err: unknown) {
                    setErr((err as Error)?.message || 'Import failed');
                } finally {
                    setBusy(false);
                    evt.target.value = '';
                }
            },
            error: (error: { message?: string }) => { setErr(error.message || 'Parse error'); setBusy(false); },
        });
    }


    async function createInvite(row: RosterEntry) {
        try {
            setBusy(true);
            const res = await authFetch('/api/invites', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ rosterId: row.id, email: row.email, name: row.name }) });
            const data = await res.json();
            if (!res.ok) throw new Error(data?.error || 'Failed to create invite');
            const link = data?.link as string;
            await navigator.clipboard.writeText(link);
            if (data?.emailSent) {
                setNote('Invite sent and link copied to clipboard');
            } else {
                setNote('Invite link copied. Email not sent (SMTP not configured).');
            }
        } catch (e: any) {
            setErr(e.message || 'Failed to create invite');
        } finally {
            setBusy(false);
        }
    }

    async function cancelInvite(row: RosterEntry) {
        try {
            setBusy(true);
            const res = await authFetch('/api/invites/cancel', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ rosterId: row.id }) });
            const data = await res.json();
            if (!res.ok) throw new Error(data?.error || 'Failed to cancel invite');
            setRows(r => r.map(x => x.id === row.id ? { ...x, status: 'Pending' } as RosterEntry : x));
            setNote('Invite canceled; status set to Pending');
        } catch (e: any) {
            setErr(e.message || 'Failed to cancel invite');
        } finally {
            setBusy(false);
        }
    }

    const headerActions = (
        <>
            <HeaderActions
                actions={[
                    {
                        key: 'import',
                        label: 'Import CSV',
                        ariaLabel: 'Import CSV',
                        icon: <UploadFileIcon fontSize='small' />,
                        onClick: () => { /* trigger hidden input */ inputRef.current?.click(); },
                    },
                    {
                        key: 'refresh',
                        ariaLabel: 'Refresh roster',
                        label: 'Refresh',
                        icon: <RefreshIcon fontSize='small' />,
                        onClick: () => load(),
                        disabled: busy,
                    }
                ]}
            />
            <input ref={inputRef} hidden accept='.csv' type='file' onChange={onCsv} />
        </>
    );

    return (
        <PageCard headerTitle='Class Roster' headerProps={{ height: 56 }} headerActions={headerActions} headerActionsVariant='plain' loadingProgress={busy}>
            <Box sx={{ p: { xs: 1.5, md: 2 } }}>
                <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1} alignItems={{ xs: 'stretch', sm: 'flex-end' }} sx={{ mb: 2 }}>
                    <TextField size="small" label="Full name" value={name} onChange={(e) => setName(e.target.value)} sx={{ flex: 1 }} />
                    <TextField size="small" label="Email" value={email} onChange={(e) => setEmail(e.target.value)} sx={{ flex: 1 }} />
                    <Button variant="contained" startIcon={<PersonAddIcon />} onClick={addOne} disabled={busy}>Add</Button>
                </Stack>
                <Table size="small">
                    <TableHead>
                        <TableRow>
                            <TableCell>Name</TableCell>
                            <TableCell>Email</TableCell>
                            <TableCell>Status</TableCell>
                            <TableCell>Privileges</TableCell>
                            <TableCell align="right">Actions</TableCell>
                        </TableRow>
                    </TableHead>
                    <TableBody>
                        {rows.map((r) => (
                            <TableRow key={r.id} hover>
                                <TableCell>{r.name}</TableCell>
                                <TableCell>{r.email}</TableCell>
                                <TableCell>
                                    <Chip
                                        size="small"
                                        label={r.status}
                                        color={r.status === 'Active' ? 'success' : r.status === 'Invited' ? 'primary' : r.status === 'Pending' ? 'warning' : 'default'}
                                        variant={r.status === 'Invited' || r.status === 'Pending' ? 'outlined' : 'filled'}
                                    />
                                </TableCell>
                                <TableCell>
                                    <Tooltip title="Teaching Assistant" arrow>
                                        <FormControlLabel
                                            control={<Switch size="small" checked={!!r.evaluator} onChange={async (_, checked) => {
                                                try {
                                                    setBusy(true);
                                                    const res = await authFetch('/api/roster', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: r.id, evaluator: checked }) });
                                                    const data = await res.json();
                                                    if (!res.ok) throw new Error(data?.error || 'Failed to update');
                                                    setRows(rows => rows.map(x => x.id === r.id ? { ...x, evaluator: data.evaluator } : x));
                                                    setNote(checked ? 'Granted Teaching Assistant privilege' : 'Revoked Teaching Assistant privilege');
                                                } catch (e: any) {
                                                    setErr(e.message || 'Update failed');
                                                } finally {
                                                    setBusy(false);
                                                }
                                            }} />}
                                            label="TA"
                                        />
                                    </Tooltip>
                                </TableCell>
                                <TableCell align="right">
                                    {r.status === 'Pending' && (
                                        <Tooltip title="Create & copy invite link">
                                            <span>
                                                <IconButton size="small" color="primary" onClick={() => createInvite(r)} disabled={busy}><LinkIcon fontSize="small" /></IconButton>
                                            </span>
                                        </Tooltip>
                                    )}
                                    {r.status === 'Invited' && (
                                        <Tooltip title="Cancel invite">
                                            <span>
                                                <IconButton size="small" color="warning" onClick={() => cancelInvite(r)} disabled={busy}><CancelIcon fontSize="small" /></IconButton>
                                            </span>
                                        </Tooltip>
                                    )}
                                    <IconButton size="small" color="error" onClick={() => remove(r.id)} disabled={busy}><DeleteIcon fontSize="small" /></IconButton>
                                </TableCell>
                            </TableRow>
                        ))}
                        {!rows.length && (
                            <TableRow><TableCell colSpan={4}>No students yet.</TableCell></TableRow>
                        )}
                    </TableBody>
                </Table>
            </Box>
            <Snackbar open={!!note} autoHideDuration={3000} onClose={() => setNote('')} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                <Alert onClose={() => setNote('')} severity="success" sx={{ width: '100%' }}>{note}</Alert>
            </Snackbar>
            <Snackbar open={!!err} autoHideDuration={5000} onClose={() => setErr('')} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                <Alert onClose={() => setErr('')} severity="error" sx={{ width: '100%' }}>{err}</Alert>
            </Snackbar>
        </PageCard>
    );
}