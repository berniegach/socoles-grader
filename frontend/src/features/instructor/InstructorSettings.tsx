'use client';
import { useMemo, useState, useEffect } from 'react';
import Grid from '@mui/material/Grid';
import {
    Box,
    Button,
    Card,
    CardActions,
    CardContent,
    CardHeader,
    Snackbar,
    TextField,
    Alert,
    Typography,
} from '@mui/material';
import SaveIcon from '@mui/icons-material/Save';
import DownloadIcon from '@mui/icons-material/Download';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import CheckCircleOutlineIcon from '@mui/icons-material/CheckCircleOutline';
import WarningAmberIcon from '@mui/icons-material/WarningAmber';
import DeleteForeverIcon from '@mui/icons-material/DeleteForever';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogActions from '@mui/material/DialogActions';
import IconButton from '@mui/material/IconButton';
import GradingOptions from '@/features/instructor/GradingOptions';
import type { GradingOptions as GradingOptionsType } from '@/lib/types';
import { DEFAULT_GRADING_OPTIONS } from '@/lib/api';
import PageCard from '@/components/PageCard';
import HeaderActions from '@/components/HeaderActions';
import { useAuth } from '@/features/auth/AuthProvider';

type Settings = {
    courseName: string;
    enrollmentCode: string;
    attempts: number;
    latePenalty: number;     // % per day
    passThreshold: number;   // % overall
    gradingDefaults: GradingOptionsType;
};

const DEFAULTS: Settings = {
    courseName: 'DB101 - Intermediate SQL',
    enrollmentCode: 'DB101-SA10',
    attempts: 3,
    latePenalty: 10,
    passThreshold: 0.60,
    gradingDefaults: { ...DEFAULT_GRADING_OPTIONS },
};

export default function InstructorSettings() {
    const { user, setUser, authFetch } = useAuth();
    const stored = useMemo(() => {
        try {
            const raw = localStorage.getItem('sqlgrader.settings');
            if (!raw) return DEFAULTS;
            const parsed = JSON.parse(raw) || {};
            // drop deprecated keys if present
            const { theme: _t, denseTables: _d, allowPartialCredit: _ap, maxRows: _mr, timeoutSec: _to, randomSeed: _rs, plagiarismCheck: _pc, apiBase: _ab, gradePath: _gp, ...rest } = parsed;
            const merged = { ...DEFAULTS, ...(rest as Partial<Settings>) } as Settings;
            if (!merged.gradingDefaults) merged.gradingDefaults = { ...DEFAULT_GRADING_OPTIONS };
            return merged;
        } catch { return DEFAULTS; }
    }, []);

    const [s, setS] = useState<Settings>(stored);
    const [accountEmail, setAccountEmail] = useState<string>('');
    const [accountName, setAccountName] = useState<string>('');
    const [nameDirty, setNameDirty] = useState(false);
    useEffect(() => {
        try {
            const raw = localStorage.getItem('auth.current');
            if (raw) {
                const parsed = JSON.parse(raw);
                if (parsed?.email) setAccountEmail(parsed.email);
                if (parsed?.name) setAccountName(parsed.name);
            }
        } catch { /* ignore */ }
    }, []);

    function saveAccountName() {
        try {
            const raw = localStorage.getItem('auth.current');
            const parsed = raw ? JSON.parse(raw) : {};
            const next = { ...parsed, name: accountName };
            localStorage.setItem('auth.current', JSON.stringify(next));
            if (user) setUser({ ...user, name: accountName });
            setNote({ type: 'success', msg: 'Display name updated.' });
            setNameDirty(false);
        } catch {
            setNote({ type: 'error', msg: 'Failed to update display name.' });
        }
    }
    const [note, setNote] = useState<{ type: 'success' | 'error' | 'info'; msg: string } | null>(null);
    const [purgeOpen, setPurgeOpen] = useState(false);
    const [purging, setPurging] = useState(false);
    const [confirmText, setConfirmText] = useState('');

    function save() {
        localStorage.setItem('sqlgrader.settings', JSON.stringify(s));
        setNote({ type: 'success', msg: 'Settings saved locally.' });
    }
    function reset() {
        setS(DEFAULTS);
        localStorage.removeItem('sqlgrader.settings');
        setNote({ type: 'info', msg: 'Settings reset to defaults.' });
    }


    function downloadStudentsTemplate() {
        const headers = ['Org Defined ID', 'Attempt #', 'Q #', 'Answer', 'Out Of'];
        const csv = headers.join(',') + '\n' + 's12345,1,1,"SELECT * FROM Book;",10\n';
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a'); a.href = url; a.download = 'students_template.csv'; a.click(); URL.revokeObjectURL(url);
    }
    function downloadRefsTemplate() {
        const csv = 'SELECT title FROM Book WHERE author = \'Pratchett\';\n';
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a'); a.href = url; a.download = 'references_template.csv'; a.click(); URL.revokeObjectURL(url);
    }

    function purgeAll() {
        if (purging) return;
        setPurging(true);
        (async () => {
            try {
                const res = await authFetch('/api/instructor/purge', { method: 'DELETE' });
                if (!res.ok) {
                    const d = await res.json().catch(() => ({}));
                    throw new Error(d.error || 'Failed');
                }
                // Clear local storage and trigger full sign-out (app + NextAuth + IdP)
                try { localStorage.clear(); sessionStorage.clear(); } catch { }
                setNote({ type: 'success', msg: 'Account deleted. Signing out…' });
                // Redirect to IdP logout endpoint which will clear local session then IdP SSO
                window.location.href = '/api/auth/idp-logout?redirect=/';
            } catch (e: any) {
                setNote({ type: 'error', msg: e.message || 'Delete failed' });
            } finally {
                setPurging(false); setPurgeOpen(false);
            }
        })();
    }

    const headerActions = (
        <HeaderActions
            actions={[
                { key: 'save-all', label: 'Save All', ariaLabel: 'Save all settings', icon: <SaveIcon fontSize='small' />, onClick: save },
                { key: 'reset', label: 'Reset', ariaLabel: 'Reset settings', icon: <RestartAltIcon fontSize='small' />, onClick: reset },
            ]}
        />
    );

    return (
        <>
            <PageCard headerTitle='Settings' headerProps={{ height: 56 }} headerActions={headerActions} headerActionsVariant='plain'>
                <Box sx={{ mb: 2 }}>
                    <Grid container spacing={2}>
                        {/* Account */}
                        <Grid size={{ xs: 12, md: 6 }}>
                            <Card>
                                <CardHeader title="Account" subheader="General instructor account details." />
                                <CardContent>
                                    <Grid container spacing={2}>
                                        <Grid size={{ xs: 12, md: 6 }}>
                                            <TextField
                                                label="Display name"
                                                value={accountName}
                                                onChange={(e) => { setAccountName(e.target.value); setNameDirty(true); }}
                                                onBlur={() => { if (nameDirty && accountName.trim()) saveAccountName(); }}
                                                helperText={nameDirty ? 'Press Save or blur to persist' : ' '}
                                                fullWidth
                                                variant='outlined'
                                                size='small'
                                                inputProps={{ maxLength: 60 }}
                                            />
                                        </Grid>
                                        <Grid size={{ xs: 12, md: 6 }}>
                                            <TextField label="Email" value={accountEmail || ''} fullWidth variant='outlined' size='small' InputProps={{ readOnly: true }} />
                                        </Grid>
                                    </Grid>
                                </CardContent>
                            </Card>
                        </Grid>

                        {/* Course Settings */}
                        <Grid size={{ xs: 12, md: 6 }}>
                            <Card>
                                <CardHeader title="Course Settings" subheader="General parameters for this course." />
                                <CardContent>
                                    <Grid container spacing={2}>
                                        <Grid size={12}>
                                            <TextField
                                                label="Course name"
                                                value={s.courseName}
                                                onChange={(e) => setS({ ...s, courseName: e.target.value })}
                                                variant="outlined"
                                                size="small"
                                                fullWidth
                                            />
                                        </Grid>
                                        <Grid size={{ xs: 12, md: 6 }}>
                                            <TextField
                                                label="Enrollment code"
                                                value={s.enrollmentCode}
                                                onChange={(e) => setS({ ...s, enrollmentCode: e.target.value })}
                                                variant="outlined"
                                                size="small"
                                                fullWidth
                                            />
                                        </Grid>
                                        <Grid size={{ xs: 6, md: 3 }}>
                                            <TextField
                                                label="Attempts"
                                                type="number"
                                                inputProps={{ min: 1, max: 20 }}
                                                value={s.attempts}
                                                onChange={(e) => setS({ ...s, attempts: Math.max(1, Math.min(20, Number(e.target.value || 1))) })}
                                                variant="outlined"
                                                size="small"
                                                fullWidth
                                            />
                                        </Grid>
                                        <Grid size={{ xs: 6, md: 3 }}>
                                            <TextField
                                                label="Pass threshold (%)"
                                                type="number"
                                                inputProps={{ min: 0, max: 100 }}
                                                value={s.passThreshold}
                                                onChange={(e) => setS({ ...s, passThreshold: Math.max(0, Math.min(100, Number(e.target.value || 0))) })}
                                                variant="outlined"
                                                size="small"
                                                fullWidth
                                            />
                                        </Grid>
                                        <Grid size={{ xs: 6, md: 3 }}>
                                            <TextField
                                                label="Late penalty (%/day)"
                                                type="number"
                                                inputProps={{ min: 0, max: 50 }}
                                                value={s.latePenalty}
                                                onChange={(e) => setS({ ...s, latePenalty: Math.max(0, Math.min(50, Number(e.target.value || 0))) })}
                                                variant="outlined"
                                                size="small"
                                                fullWidth
                                            />
                                        </Grid>
                                    </Grid>
                                </CardContent>
                                <CardActions sx={{ justifyContent: 'flex-end', gap: 1 }}>
                                    {nameDirty && (
                                        <Button startIcon={<SaveIcon />} color='secondary' variant='outlined' onClick={saveAccountName} disabled={!accountName.trim()}>Save Name</Button>
                                    )}
                                    <Button startIcon={<SaveIcon />} variant="contained" onClick={save}>Save Course</Button>
                                </CardActions>
                            </Card>
                        </Grid>

                        {/* Grading Defaults */}
                        <Grid size={{ xs: 12, md: 6 }}>
                            <Card>
                                <CardHeader title="Grading Defaults" subheader="Backend grading parameters used as defaults across tools." />
                                <CardContent>
                                    <GradingOptions
                                        options={s.gradingDefaults}
                                        onChange={(key, value) => setS((prev) => ({ ...prev, gradingDefaults: { ...prev.gradingDefaults, [key]: value } }))}
                                    />
                                </CardContent>
                                <CardActions sx={{ justifyContent: 'flex-end' }}>
                                    <Button startIcon={<SaveIcon />} variant="contained" onClick={save}>Save</Button>
                                </CardActions>
                            </Card>
                        </Grid>

                        {/* CSV Templates */}
                        <Grid size={{ xs: 12, md: 6 }}>
                            <Card>
                                <CardHeader title="CSV Templates" subheader="Download starter files for bulk grading." />
                                <CardContent>
                                    <Box sx={{ display: 'flex', gap: 1.5, flexWrap: 'wrap' }}>
                                        <Button startIcon={<DownloadIcon />} variant="outlined" onClick={downloadStudentsTemplate}>
                                            Students answers CSV
                                        </Button>
                                        <Button startIcon={<DownloadIcon />} variant="outlined" onClick={downloadRefsTemplate}>
                                            Reference statements CSV
                                        </Button>
                                    </Box>
                                </CardContent>
                            </Card>
                        </Grid>

                        {/* Danger Zone */}
                        <Grid size={{ xs: 12, md: 6 }}>
                            <Card>
                                <CardHeader title="Danger Zone" subheader="Developer-only tools while wiring things up." />
                                <CardContent>
                                    <Alert severity="warning" icon={<WarningAmberIcon />}>
                                        This resets local settings stored in your browser (safe to use in development).
                                    </Alert>
                                </CardContent>
                                <CardActions sx={{ justifyContent: 'space-between' }}>
                                    <Button startIcon={<RestartAltIcon />} color="warning" variant="outlined" onClick={reset}>
                                        Reset to defaults
                                    </Button>
                                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                                        <IconButton color="error" onClick={() => setPurgeOpen(true)} title="Delete account & all data">
                                            <DeleteForeverIcon />
                                        </IconButton>
                                        <Button startIcon={<CheckCircleOutlineIcon />} variant="contained" onClick={save}>
                                            Save all
                                        </Button>
                                    </Box>
                                </CardActions>
                            </Card>
                        </Grid>
                    </Grid>
                </Box>
            </PageCard>

            <Snackbar
                open={!!note}
                autoHideDuration={3200}
                onClose={() => setNote(null)}
                anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
            >
                {note ? <Alert onClose={() => setNote(null)} severity={note.type} sx={{ width: '100%' }}>{note.msg}</Alert> : undefined}
            </Snackbar>

            <Dialog open={purgeOpen} onClose={() => (!purging && setPurgeOpen(false))} maxWidth="sm" fullWidth>
                <DialogTitle>Delete Account & All Data</DialogTitle>
                <DialogContent>
                    <DialogContentText>
                        This will permanently delete ALL of your instructor data: assignments, questions, submissions, per-question answers, datasets, and settings. This cannot be undone.
                    </DialogContentText>
                    <DialogContentText sx={{ mt: 2, fontWeight: 600 }}>
                        Type DELETE to confirm.
                    </DialogContentText>
                    <TextField
                        autoFocus
                        fullWidth
                        variant="outlined"
                        size="small"
                        placeholder="DELETE"
                        onChange={(e) => setConfirmText(e.target.value)}
                        value={confirmText}
                        sx={{ mt: 1 }}
                    />
                </DialogContent>
                <DialogActions sx={{ justifyContent: 'space-between' }}>
                    <Button onClick={() => setPurgeOpen(false)} disabled={purging}>Cancel</Button>
                    <Button color="error" variant="contained" onClick={purgeAll} disabled={purging || confirmText !== 'DELETE'}>
                        {purging ? 'Deleting…' : 'Delete Everything'}
                    </Button>
                </DialogActions>
            </Dialog>
        </>
    );
}
