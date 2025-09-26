'use client';
import Grid from '@mui/material/Grid';
import { Card, CardHeader, CardContent, CardActions, Typography, TextField, Button, Alert, LinearProgress } from '@mui/material';
import { useEffect, useState } from 'react';
import { useAuth } from '@/features/auth/AuthProvider';
import PageCard from '@/components/PageCard';

export default function StudentProfile() {
    const { user, authFetch } = useAuth();
    const [fullName, setFullName] = useState('');
    const [email, setEmail] = useState('');
    const [currentPassword, setCurrentPassword] = useState('');
    const [newPassword, setNewPassword] = useState('');
    const [confirm, setConfirm] = useState('');
    const [busy, setBusy] = useState(false);
    const [note, setNote] = useState<string | null>(null);
    const [err, setErr] = useState<string | null>(null);

    useEffect(() => {
        // Prefer student session data if available
        try {
            const raw = typeof window !== 'undefined' ? localStorage.getItem('sqlgrader.student') : null;
            if (raw) {
                const parsed = JSON.parse(raw);
                if (parsed?.name) setFullName(parsed.name);
                if (parsed?.email) setEmail(parsed.email);
            } else {
                setFullName(user?.name || '');
                setEmail(user?.name || '');
            }
        } catch {
            setFullName(user?.name || '');
            setEmail(user?.name || '');
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    async function changePassword() {
        setErr(null); setNote(null); setBusy(true);
        try {
            if (!currentPassword || !newPassword) throw new Error('All fields are required');
            if (newPassword !== confirm) throw new Error('Passwords do not match');
            const res = await authFetch('/api/student/change-password', {
                method: 'POST', headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ currentPassword, newPassword })
            });
            const data = await res.json().catch(() => ({}));
            if (!res.ok) throw new Error(data?.error || 'Failed to update password');
            setNote('Password updated');
            setCurrentPassword(''); setNewPassword(''); setConfirm('');
        } catch (e: unknown) {
            setErr(e instanceof Error ? e.message : 'Failed');
        } finally { setBusy(false); }
    }

    return (
        <PageCard>
            <Grid container spacing={2}>
                <Grid size={12}>
                    <Card>
                        <CardHeader title="Account" subheader="Your name and email are managed by your instructor." />
                        {busy && <LinearProgress />}
                        <CardContent>
                            {note && <Alert severity="success" sx={{ mb: 2 }}>{note}</Alert>}
                            {err && <Alert severity="error" sx={{ mb: 2 }}>{err}</Alert>}
                            <Grid container spacing={2}>
                                <Grid size={{ xs: 12, md: 6 }}>
                                    <TextField label="Full name" size="small" value={fullName} disabled variant="outlined" fullWidth />
                                </Grid>
                                <Grid size={{ xs: 12, md: 6 }}>
                                    <TextField label="Email" size="small" value={email} disabled variant="outlined" fullWidth />
                                </Grid>
                            </Grid>
                        </CardContent>
                    </Card>
                </Grid>

                <Grid size={12}>
                    <Card>
                        <CardHeader title="Change Password" subheader="Update your password for future sign-ins." />
                        {busy && <LinearProgress />}
                        <CardContent>
                            <Grid container spacing={2}>
                                <Grid size={{ xs: 12, md: 4 }}>
                                    <TextField label="Current password" size="small" type="password" value={currentPassword} onChange={(e) => setCurrentPassword(e.target.value)} fullWidth />
                                </Grid>
                                <Grid size={{ xs: 12, md: 4 }}>
                                    <TextField label="New password" size="small" type="password" value={newPassword} onChange={(e) => setNewPassword(e.target.value)} fullWidth />
                                </Grid>
                                <Grid size={{ xs: 12, md: 4 }}>
                                    <TextField label="Confirm new password" size="small" type="password" value={confirm} onChange={(e) => setConfirm(e.target.value)} fullWidth />
                                </Grid>
                            </Grid>
                        </CardContent>
                        <CardActions sx={{ justifyContent: 'flex-end' }}>
                            <Button variant="contained" onClick={changePassword} disabled={busy || !currentPassword || !newPassword || newPassword !== confirm}>Update Password</Button>
                        </CardActions>
                    </Card>
                </Grid>
            </Grid>
        </PageCard>
    );
}
