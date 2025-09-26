"use client";
import { use, useEffect, useState } from 'react';
import { Box, Card, CardContent, CardHeader, TextField, Button, Alert, LinearProgress, Typography, Stack } from '@mui/material';
import { useAuth } from '@/features/auth/AuthProvider';

export default function InviteAcceptPage({ params }: { params: Promise<{ token: string }> }) {
    const { token } = use(params);
    const [name, setName] = useState('');
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [confirm, setConfirm] = useState('');
    const [busy, setBusy] = useState(false);
    const [loading, setLoading] = useState(true);
    const [inviteOk, setInviteOk] = useState(false);
    const [note, setNote] = useState('');
    const [err, setErr] = useState('');
    const { setUser } = useAuth();

    // Load invite details (name/email) and lock them in the UI
    useEffect(() => {
        let cancelled = false;
        (async () => {
            setLoading(true);
            setErr('');
            try {
                const res = await fetch(`/api/invites/${encodeURIComponent(token)}`);
                const data = await res.json();
                if (!res.ok) throw new Error(data?.error || 'Invite not found');
                if (cancelled) return;
                setName(data?.name || '');
                setEmail(data?.email || '');
                const used = !!data?.used_at;
                const expired = data?.expires_at ? new Date(data.expires_at) < new Date() : false;
                if (used) {
                    setErr('This invite was already used.');
                    setInviteOk(false);
                } else if (expired) {
                    setErr('This invite has expired.');
                    setInviteOk(false);
                } else {
                    setInviteOk(true);
                }
            } catch (e: unknown) {
                setErr(e instanceof Error ? e.message : 'Failed to load invite');
                setInviteOk(false);
            } finally {
                if (!cancelled) setLoading(false);
            }
        })();
        return () => { cancelled = true; };
    }, [token]);

    async function accept() {
        setErr(''); setNote(''); setBusy(true);
        try {
            if (!password || password !== confirm) throw new Error('Passwords do not match');
            const res = await fetch('/api/student/signup-from-invite', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ token, password }) });
            const data = await res.json();
            if (!res.ok) throw new Error(data?.error || 'Failed to accept invite');
            // store student session
            const { token: studentToken, student } = data || {};
            localStorage.setItem('sqlgrader.student', JSON.stringify({ token: studentToken, id: student?.id, email: student?.email, name: student?.name, instructorId: student?.instructorId }));
            setUser({ name: student?.email, role: 'student', token: studentToken, instructorId: student?.instructorId });
            setNote('Invite accepted. Redirecting...');
            // navigate by telling AppShell to focus student dashboard
            window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 's-dash' } }));
        } catch (e: unknown) {
            setErr(e instanceof Error ? e.message : 'Failed');
        } finally {
            setBusy(false);
        }
    }

    return (
        <Box sx={{ minHeight: '100vh', display: 'grid', placeItems: 'center', p: 2 }}>
            <Card sx={{ width: '100%', maxWidth: 520 }}>
                <CardHeader title="Join Class" subheader={<Typography variant="body2">Create your password to join and sign in later</Typography>} />
                {(busy || loading) && <LinearProgress />}
                <CardContent>
                    {!!note && <Alert severity="success" sx={{ mb: 2 }}>{note}</Alert>}
                    {!!err && <Alert severity="error" sx={{ mb: 2 }}>{err}</Alert>}
                    <Stack spacing={1.5}>
                        <TextField label="Full name" size="small" fullWidth value={name} disabled />
                        <TextField label="Email" size="small" fullWidth value={email} disabled />
                        <TextField label="Password" type="password" size="small" fullWidth value={password} onChange={(e) => setPassword(e.target.value)} />
                        <TextField label="Confirm Password" type="password" size="small" fullWidth value={confirm} onChange={(e) => setConfirm(e.target.value)} />
                        <Button variant="contained" onClick={accept} disabled={busy || loading || !inviteOk || !password || password !== confirm}>Accept & Create Account</Button>
                    </Stack>
                </CardContent>
            </Card>
        </Box>
    );
}
