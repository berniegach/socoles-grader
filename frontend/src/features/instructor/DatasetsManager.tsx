"use client";
import { useEffect, useRef, useState } from 'react';
import {
    Box,
    Button,
    Chip,
    Menu,
    MenuItem,
    Dialog,
    DialogActions,
    DialogContent,
    DialogTitle,
    IconButton,
    Snackbar,
    Alert,
    TextField,
    Typography,
} from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import EditIcon from '@mui/icons-material/Edit';
import DeleteIcon from '@mui/icons-material/DeleteOutline';
import RefreshIcon from '@mui/icons-material/Refresh';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import HeaderActions from '@/components/HeaderActions';
import TilesGrid from '@/components/TilesGrid';
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

    // tile menu state
    const [menuAnchor, setMenuAnchor] = useState<null | HTMLElement>(null);
    const [menuId, setMenuId] = useState<string | null>(null);
    const openMenu = Boolean(menuAnchor);
    const handleMenuOpen = (e: React.MouseEvent<HTMLElement>, id: string) => { e.stopPropagation(); setMenuAnchor(e.currentTarget); setMenuId(id); };
    const handleMenuClose = () => { setMenuAnchor(null); setMenuId(null); };
    const triggerMenu = (cb: (id: string) => void) => { if (menuId) cb(menuId); handleMenuClose(); };

    const headerActions = (
        <>
            <HeaderActions
                actions={[
                    { key: 'new', ariaLabel: 'New dataset', title: 'New dataset', icon: <AddIcon fontSize='small' />, onClick: openCreate },
                    { key: 'import', ariaLabel: 'Import .sql', title: 'Import .sql', icon: <UploadFileIcon fontSize='small' />, onClick: triggerImport },
                    { key: 'reload', ariaLabel: 'Reload', title: 'Reload datasets', icon: <RefreshIcon fontSize='small' />, onClick: () => load(), disabled: loading }
                ]}
            />
            <input ref={importInputRef} hidden type='file' accept='.sql' onChange={onFileChange} />
        </>
    );

    return (
        <PageCard headerTitle='Datasets' headerProps={{ height: 56 }} headerActions={headerActions} headerActionsVariant='plain' loadingProgress={loading}>
            <Box sx={{ p: { xs: 1.5, md: 2 } }}>
                {error && <Alert severity="error" sx={{ mb: 1 }}>{error}</Alert>}
                <Box sx={{ display: 'grid', gap: 1.25 }}>
                    {!datasets.length && !loading && (
                        <Typography variant="body2" color="text.secondary">No datasets yet.</Typography>
                    )}
                    <TilesGrid>
                        {datasets.map(ds => {
                            const schema = extractSchema(ds.sql || '');
                            const tables = schema.length;
                            const columns = schema.reduce((sum, t) => sum + t.columns.length, 0);
                            return (
                                <Box
                                    key={ds.id}
                                    sx={(theme) => ({
                                        position: 'relative',
                                        p: 1.25,
                                        borderRadius: 1.25,
                                        cursor: 'default',
                                        display: 'grid',
                                        gap: 0.75,
                                        minHeight: 112,
                                        background: theme.palette.mode === 'dark'
                                            ? 'rgba(255,255,255,0.03)'
                                            : 'linear-gradient(145deg,#ffffff,#f9f9f9)',
                                        border: '1px solid',
                                        borderColor: theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)',
                                        boxShadow: '0 1px 2px rgba(0,0,0,0.08)',
                                        transition: 'background .25s, box-shadow .25s, transform .25s, border-color .25s',
                                        outline: 'none',
                                        '&:hover': {
                                            boxShadow: '0 4px 12px -2px rgba(0,0,0,0.25)',
                                            transform: 'translateY(-2px)',
                                            borderColor: 'primary.light'
                                        },
                                        '&:focus-visible': {
                                            boxShadow: '0 0 0 2px #fff, 0 0 0 4px #ff66c4',
                                        }
                                    })}
                                >
                                    <Box sx={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', gap: 1 }}>
                                        <Box sx={{ flex: 1, minWidth: 0 }}>
                                            <Typography variant='subtitle2' sx={{ fontWeight: 600, lineHeight: 1.2, pr: 1, overflow: 'hidden', textOverflow: 'ellipsis' }}>{ds.name}</Typography>
                                            <Typography variant='caption' color='text.secondary'>SQL length: {ds.sql?.length || 0} chars</Typography>
                                        </Box>
                                        <IconButton size='small' onClick={(e) => handleMenuOpen(e, ds.id)} aria-label={`Actions for ${ds.name}`}>
                                            <MoreVertIcon fontSize='small' />
                                        </IconButton>
                                    </Box>
                                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.6, flexWrap: 'wrap' }}>
                                        <Chip size='small' label={`${tables} table${tables !== 1 ? 's' : ''}`} variant='outlined' />
                                        <Chip size='small' label={`${columns} col`} variant='outlined' />
                                        <Chip size='small' label={`${(ds.sql?.length || 0)} chars`} />
                                    </Box>
                                    <Box sx={{ display: 'grid', gap: .5, maxHeight: 160, overflowY: 'auto', pr: 0.5 }}>
                                        {schema.length ? schema.map(t => (
                                            <Typography key={t.name} variant='caption' sx={{ fontFamily: 'ui-monospace,monospace', display: 'block' }}>
                                                {t.name}(
                                                {t.columns.map((c, i) => (
                                                    <Box key={c.name + i} component='span' sx={c.pk ? { textDecoration: 'underline', textDecorationColor: 'success.main', textDecorationThickness: '2px', textUnderlineOffset: '2px' } : undefined}>
                                                        {c.name}
                                                    </Box>
                                                )).reduce<React.ReactNode[]>((acc, el, i, arr) => acc.concat(el, i < arr.length - 1 ? ', ' : ''), [])}
                                                )
                                            </Typography>
                                        )) : (
                                            <Typography variant='caption' color='text.secondary'>No CREATE TABLE statements detected.</Typography>
                                        )}
                                    </Box>
                                </Box>
                            );
                        })}
                    </TilesGrid>
                    <Menu anchorEl={menuAnchor} open={openMenu} onClose={handleMenuClose} keepMounted>
                        <MenuItem onClick={() => { triggerMenu(id => { const ds = datasets.find(d => d.id === id); if (ds) openEdit(ds); }); }}><EditIcon fontSize='small' style={{ marginRight: 8 }} /> Edit</MenuItem>
                        <MenuItem onClick={() => { triggerMenu(id => del(id)); }}>
                            <DeleteIcon fontSize='small' color='error' style={{ marginRight: 8 }} />
                            <Typography variant='body2' color='error.main'>Delete</Typography>
                        </MenuItem>
                    </Menu>
                </Box>
            </Box>

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
