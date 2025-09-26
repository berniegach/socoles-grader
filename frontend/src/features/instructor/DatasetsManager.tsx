"use client";
import { useEffect, useRef, useState } from 'react';
import {
    Box,
    Button,
    Card,
    CardContent,
    CardHeader,
    Dialog,
    DialogActions,
    DialogContent,
    DialogTitle,
    Divider,
    IconButton,
    LinearProgress,
    MenuItem,
    Snackbar,
    Alert,
    TextField,
    Typography,
} from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import EditIcon from '@mui/icons-material/Edit';
import DeleteIcon from '@mui/icons-material/DeleteOutline';
import RefreshIcon from '@mui/icons-material/Refresh';
import PageCard from '@/components/PageCard';
import { useAuth } from '@/features/auth/AuthProvider';

export default function DatasetsManager() {
    const { authFetch, user } = useAuth();
    const [datasets, setDatasets] = useState<Array<{ id: string; name: string; sql: string }>>([]);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [note, setNote] = useState<string | null>(null);
    const importInputRef = useRef<HTMLInputElement | null>(null);

    // Create/edit dialog state
    const [dlgOpen, setDlgOpen] = useState(false);
    const [editingId, setEditingId] = useState<string | null>(null);
    const [draftName, setDraftName] = useState('');
    const [draftSql, setDraftSql] = useState('');
    const [saving, setSaving] = useState(false);
    const [deletingId, setDeletingId] = useState<string | null>(null);

    async function load() {
        setLoading(true); setError(null);
        try {
            const res = await authFetch('/api/datasets');
            const data = await res.json();
            setDatasets(Array.isArray(data) ? data : []);
        } catch (e: any) { setError(e.message || 'Failed to load datasets'); } finally { setLoading(false); }
    }

    useEffect(() => { if (user?.token) void load(); }, [user?.token]);

    function openCreate() { setEditingId(null); setDraftName(''); setDraftSql(''); setDlgOpen(true); }
    function openEdit(ds: { id: string; name: string; sql: string }) { setEditingId(ds.id); setDraftName(ds.name); setDraftSql(ds.sql); setDlgOpen(true); }
    function closeDlg() { if (!saving) setDlgOpen(false); }

    async function onImportSql(file: File) {
        try {
            const text = await file.text();
            const base = file.name.replace(/\.[^.]+$/, '');
            setEditingId(null);
            setDraftName(base);
            setDraftSql(text);
            setDlgOpen(true);
            setNote(`Imported ${file.name}`);
        } catch (e: any) {
            setError(e?.message || 'Failed to read SQL file');
        }
    }
    function triggerImport() { importInputRef.current?.click(); }
    function onFileChange(e: React.ChangeEvent<HTMLInputElement>) {
        const f = e.target.files?.[0];
        if (f) { void onImportSql(f); }
        // allow selecting the same file again later
        e.currentTarget.value = '';
    }

    // Quick-and-simple schema extractor from CREATE TABLE statements with PK detection
    function extractSchema(sql: string): Array<{ name: string; columns: Array<{ name: string; pk: boolean }> }> {
        try {
            const cleaned = (sql || '')
                .replace(/--.*$/gm, '')
                .replace(/\/\*[\s\S]*?\*\//g, '');
            const out: Array<{ name: string; columns: Array<{ name: string; pk: boolean }> }> = [];
            const re = /create\s+table\s+(["`\[]?[\w.]+["`\]]?)\s*\(([^;]*)\)/gi;
            let m: RegExpExecArray | null;
            while ((m = re.exec(cleaned)) !== null) {
                const rawName = (m[1] || '').trim();
                const name = rawName.replace(/["`\[\]]/g, '');
                const body = (m[2] || '').trim();
                const parts = body.split(/,(?![^()]*\))/).map(s => s.trim()).filter(Boolean);
                const pkSet = new Set<string>();
                // First pass: table-level PK constraints
                for (const line of parts) {
                    const l = line.replace(/\s+/g, ' ').trim();
                    // Matches: PRIMARY KEY (col, col2) or CONSTRAINT xyz PRIMARY KEY (col,...)
                    const pkMatch = l.match(/^(?:constraint\s+\S+\s+)?primary\s+key\s*\(([^)]+)\)/i);
                    if (pkMatch) {
                        const cols = pkMatch[1].split(',').map(s => s.trim().replace(/["`\[\]]/g, '').toLowerCase());
                        cols.forEach(c => pkSet.add(c));
                    }
                }
                // Second pass: collect columns and inline PKs
                const colDefs: Array<{ name: string; pk: boolean }> = [];
                for (const line of parts) {
                    const l = line.replace(/\s+/g, ' ').trim();
                    if (/^(constraint|primary|foreign|unique|check|key|references)\b/i.test(l)) continue;
                    const colNameRaw = l.split(' ')[0] || '';
                    const colName = colNameRaw.replace(/["`\[\]]/g, '');
                    const isInlinePk = /\bprimary\s+key\b/i.test(l);
                    const isPk = isInlinePk || pkSet.has(colName.toLowerCase());
                    colDefs.push({ name: colName, pk: !!isPk });
                }
                out.push({ name, columns: colDefs });
            }
            return out;
        } catch { return []; }
    }

    async function save() {
        setSaving(true); setError(null);
        try {
            if (editingId) {
                const res = await authFetch('/api/datasets', { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: editingId, name: draftName, sql: draftSql }) });
                const data = await res.json();
                if (!res.ok) throw new Error(data?.error || 'Update failed');
                setDatasets(list => list.map(d => d.id === editingId ? data : d));
                setNote('Dataset updated');
            } else {
                const res = await authFetch('/api/datasets', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name: draftName.trim(), sql: draftSql }) });
                const data = await res.json();
                if (!res.ok) throw new Error(data?.error || 'Create failed');
                setDatasets(list => [...list, data]);
                setNote('Dataset created');
            }
            setDlgOpen(false);
        } catch (e: any) {
            setError(e.message || 'Save failed');
        } finally { setSaving(false); }
    }

    async function del(id: string) {
        if (!confirm('Delete this dataset?')) return;
        setDeletingId(id);
        try {
            const res = await authFetch(`/api/datasets?id=${id}`, { method: 'DELETE' });
            const data = await res.json().catch(() => ({}));
            if (!res.ok) throw new Error(data?.error || 'Delete failed');
            setDatasets(list => list.filter(d => d.id !== id));
            setNote('Dataset deleted');
        } catch (e: any) { setError(e.message || 'Delete failed'); } finally { setDeletingId(null); }
    }

    return (
        <PageCard>
            <CardHeader title={<Typography variant="subtitle1" fontWeight={600}>Datasets</Typography>} action={
                <Box sx={{ display: 'flex', gap: 1 }}>
                    <Button size="small" variant="contained" startIcon={<AddIcon />} onClick={openCreate}>New dataset</Button>
                    <Button size="small" variant="outlined" onClick={triggerImport}>Import .sql</Button>
                    <IconButton onClick={() => load()} disabled={loading} title="Reload"><RefreshIcon /></IconButton>
                    <input ref={importInputRef} hidden type="file" accept=".sql" onChange={onFileChange} />
                </Box>
            } />
            {loading && <LinearProgress />}
            <CardContent>
                {error && <Alert severity="error" sx={{ mb: 1 }}>{error}</Alert>}
                <Box sx={{ display: 'grid', gap: 1.25 }}>
                    {!datasets.length && !loading && (
                        <Typography variant="body2" color="text.secondary">No datasets yet.</Typography>
                    )}
                    {datasets.map(ds => {
                        const schema = extractSchema(ds.sql || '');
                        return (
                            <Card key={ds.id} variant="outlined">
                                <CardHeader title={ds.name} subheader={<Typography variant="caption" color="text.secondary">SQL length: {ds.sql?.length || 0} chars</Typography>} action={
                                    <Box sx={{ display: 'flex', gap: .5 }}>
                                        <Button size="small" startIcon={<EditIcon fontSize='small' />} onClick={() => openEdit(ds)}>Edit</Button>
                                        <Button size="small" color="error" startIcon={<DeleteIcon fontSize='small' />} onClick={() => del(ds.id)} disabled={deletingId === ds.id}>{deletingId === ds.id ? 'Deleting…' : 'Delete'}</Button>
                                    </Box>
                                } />
                                <CardContent sx={{ pt: 0 }}>
                                    {schema.length ? (
                                        <Box sx={{ display: 'grid', gap: .5 }}>
                                            {schema.map(t => (
                                                <Typography key={t.name} variant="caption" sx={{ fontFamily: 'ui-monospace,monospace' }}>
                                                    {t.name}(
                                                    {t.columns.slice(0, 8).map((c, i) => (
                                                        <Box key={c.name + i} component="span" sx={c.pk ? { textDecoration: 'underline', textDecorationColor: 'success.main', textDecorationThickness: '2px', textUnderlineOffset: '2px' } : undefined}>
                                                            {c.name}
                                                        </Box>
                                                    )).reduce<React.ReactNode[]>((acc, el, i, arr) => acc.concat(el, i < arr.length - 1 ? ', ' : ''), [])}
                                                    {t.columns.length > 8 ? ', …' : ''}
                                                    )
                                                </Typography>
                                            ))}
                                        </Box>
                                    ) : (
                                        <Typography variant="caption" color="text.secondary">No CREATE TABLE statements detected.</Typography>
                                    )}
                                </CardContent>
                            </Card>
                        );
                    })}
                </Box>
            </CardContent>

            <Dialog open={dlgOpen} onClose={closeDlg} fullWidth maxWidth='md'>
                <DialogTitle>{editingId ? 'Edit dataset' : 'New dataset'}</DialogTitle>
                <DialogContent dividers>
                    <Box sx={{ display: 'grid', gap: 1.5 }}>
                        <TextField label='Name' value={draftName} onChange={e => setDraftName(e.target.value)} size='small' />
                        <TextField label='Init SQL' value={draftSql} onChange={e => setDraftSql(e.target.value)} multiline minRows={10} InputProps={{ sx: { fontFamily: 'ui-monospace,monospace' } }} />
                    </Box>
                </DialogContent>
                <DialogActions>
                    <Button onClick={closeDlg}>Cancel</Button>
                    <Button variant='contained' onClick={save} disabled={saving || !draftName.trim() || !draftSql.trim()}>{saving ? 'Saving…' : 'Save'}</Button>
                </DialogActions>
            </Dialog>

            <Snackbar open={!!note} autoHideDuration={2800} onClose={() => setNote(null)} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                {!!note ? <Alert severity='success' variant='filled' onClose={() => setNote(null)}>{note}</Alert> : undefined}
            </Snackbar>
        </PageCard>
    );
}
