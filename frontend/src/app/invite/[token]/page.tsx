"use client";
import { useEffect, useRef, useState } from 'react';
import { useRouter } from 'next/navigation';
import { Box, Card, CardContent, CardHeader, TextField, Button, Alert, LinearProgress, Typography, Stack } from '@mui/material';
import KeyIcon from '@mui/icons-material/VpnKey';
import { signIn } from 'next-auth/react';
import { useAuth } from '@/features/auth/AuthProvider';

export default function InviteAcceptPage({ params }: any) {
    const [token, setToken] = useState<string>('');
    // Support both Promise-style params and plain object params to satisfy local typing
    useEffect(() => {
        let cancelled = false;
        const p: any = params;
        if (p && typeof p.then === 'function') {
            p.then((v: any) => { if (!cancelled) setToken(v?.token || ''); }).catch(() => { if (!cancelled) setToken(''); });
        } else {
            setToken((p && typeof p === 'object' ? p.token : '') || '');
        }
        return () => { cancelled = true; };
    }, [params]);
    const router = useRouter();
    const [name, setName] = useState('');
    const [email, setEmail] = useState('');
    const [busy, setBusy] = useState(false);
    const [loading, setLoading] = useState(true);
    const [inviteOk, setInviteOk] = useState(false);
    const [note, setNote] = useState('');
    const [courseName, setCourseName] = useState<string>('');
    const [err, setErr] = useState('');
    const { setUser } = useAuth();
    // SSO session detection
    const [ssoReady, setSsoReady] = useState(false);
    const [ssoEmail, setSsoEmail] = useState<string | null>(null);
    const autoExchangedRef = useRef(false);

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
                setCourseName(data?.courseName || '');
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

    // Detect SSO session and auto-exchange on return from Keycloak
    useEffect(() => {
        let cancelled = false;
        (async () => {
            try {
                const res = await fetch('/api/auth/session', { cache: 'no-store' });
                if (!res.ok) { if (!cancelled) { setSsoReady(false); setSsoEmail(null); } return; }
                const data = await res.json();
                const email: string | undefined = data?.user?.email;
                if (cancelled) return;
                const ready = !!email;
                setSsoReady(ready);
                setSsoEmail(email ?? null);
            } catch {
                if (!cancelled) { setSsoReady(false); setSsoEmail(null); }
            }
        })();
        return () => { cancelled = true; };
    }, []);

    useEffect(() => {
        // After returning from IdP with a session, automatically complete the exchange once
        if (ssoReady && inviteOk && !loading && !busy && !autoExchangedRef.current) {
            autoExchangedRef.current = true;
            (async () => {
                try {
                    setBusy(true);
                    const ex = await fetch('/api/auth/student/exchange', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ token }) });
                    const data = await ex.json();
                    if (!ex.ok) {
                        // Show error but don't loop
                        throw new Error(data?.error || 'SSO accept failed');
                    }
                    const { token: studentToken, student } = data || {};
                    localStorage.setItem('sqlgrader.student', JSON.stringify({ token: studentToken, id: student?.id, email: student?.email, name: student?.name, instructorId: student?.instructorId }));
                    setUser({ name: student?.email, role: 'student', token: studentToken, instructorId: student?.instructorId });
                    setNote('Invite accepted via SSO. Redirecting...');
                    try { sessionStorage.setItem('appshell.active', 's-dash'); } catch { }
                    window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 's-dash' } }));
                    router.push('/');
                } catch (e: unknown) {
                    setErr(e instanceof Error ? e.message : 'SSO accept failed');
                } finally {
                    setBusy(false);
                }
            })();
        }
    }, [ssoReady, inviteOk, loading, busy, router, setUser, token]);

    async function acceptWithSSO() {
        // Always send the user to the Keycloak sign-in page; they will return here
        // and the automatic exchange will complete the flow.
        await signIn('keycloak', { callbackUrl: `/invite/${encodeURIComponent(token)}` });
    }

    return (
        <Box
            sx={{
                minHeight: '100vh',
                display: 'grid',
                placeItems: 'center',
                p: 3,
                background: (t) => `radial-gradient(900px 480px at 80% -10%, rgba(29, 78, 216, 0.08), transparent 60%), ${t.palette.background.default}`,
            }}
        >
            <Card
                sx={{
                    width: '100%',
                    maxWidth: 560,
                    borderRadius: '6px',
                    overflow: 'hidden',
                    borderColor: 'rgba(29, 78, 216, 0.10)',
                    boxShadow: '0 6px 20px rgba(29, 78, 216, 0.08)',
                }}
                variant="outlined"
            >
                {/* Gradient header to match auth pages */}
                <CardHeader
                    titleTypographyProps={{ sx: { fontWeight: 700 } }}
                    title={`Join ${courseName ? courseName : 'Course'}`}
                    subheader={
                        <Typography variant="body2" sx={{ opacity: 0.9 }}>
                            {courseName
                                ? `You're accepting an invite to join the course ${courseName}. Create your account to continue.`
                                : 'Create your account to join and sign in later'}
                        </Typography>
                    }
                    sx={{
                        px: 2,
                        py: 2,
                        textAlign: 'center',
                        color: 'primary.contrastText',
                        background: 'linear-gradient(90deg,#191654,#43C6AC)',
                    }}
                />

                {(busy || loading) && <LinearProgress />}

                <CardContent sx={{ p: 3 }}>
                    {!!note && <Alert severity="success" sx={{ mb: 2 }}>{note}</Alert>}
                    {!!err && <Alert severity="error" sx={{ mb: 2 }}>
                        {err}
                        {err.toLowerCase().includes('email') && (
                            <Button
                                variant="text"
                                color="secondary"
                                size="small"
                                sx={{ ml: 2, textTransform: 'none' }}
                                onClick={() => { window.location.href = '/api/auth/idp-logout?redirect=/invite/' + encodeURIComponent(token); }}
                            >
                                Log out of Keycloak
                            </Button>
                        )}
                    </Alert>}

                    <Stack spacing={1.5}>
                        <TextField label="Full name" size="small" fullWidth value={name} disabled />
                        <TextField label="Email" size="small" fullWidth value={email} disabled />
                        <Button
                            variant="outlined"
                            color="secondary"
                            startIcon={<KeyIcon />}
                            onClick={acceptWithSSO}
                            disabled={busy || loading || !inviteOk}
                            sx={{
                                alignSelf: 'center',
                                borderColor: (t) => t.palette.secondary.main,
                                '&:hover': { background: (t) => `${t.palette.secondary.main}12` },
                            }}
                        >
                            Continue with Keycloak
                        </Button>
                    </Stack>
                </CardContent>
            </Card>
        </Box>
    );
}