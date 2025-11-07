"use client";
import { useEffect, useRef, useState } from 'react';
import { useRouter } from 'next/navigation';
import { Box, Card, CardContent, CardHeader, TextField, Button, Alert, LinearProgress, Typography, Stack } from '@mui/material';
import KeyIcon from '@mui/icons-material/VpnKey';
import CheckIcon from '@mui/icons-material/CheckCircleOutline';
import ReplayIcon from '@mui/icons-material/Replay';
import LogoutIcon from '@mui/icons-material/Logout';
import { signIn } from 'next-auth/react';
import { useAuth } from '@/features/auth/AuthProvider';

// More explicit typing; params are synchronous in Next.js App Router
export default function InviteAcceptPage({ params }: any) {
    const [token, setToken] = useState<string>('');
    const router = useRouter();
    const [name, setName] = useState('');
    const [email, setEmail] = useState('');
    const [busy, setBusy] = useState(false);          // busy during exchange / manual actions
    const [loading, setLoading] = useState(true);      // loading invite itself
    const [inviteOk, setInviteOk] = useState(false);   // invite is valid & unused
    const [note, setNote] = useState('');
    const [courseName, setCourseName] = useState<string>('');
    const [err, setErr] = useState('');                // general error message
    const [exchangeErrCode, setExchangeErrCode] = useState<number | null>(null);
    const { setUser } = useAuth();
    // SSO session detection
    const [ssoReady, setSsoReady] = useState(false);   // session has an email claim
    const [ssoSession, setSsoSession] = useState(false); // session exists at all
    const [ssoEmail, setSsoEmail] = useState<string | null>(null);
    const autoExchangedRef = useRef(false);            // prevent repeated auto attempts
    const [manualMode, setManualMode] = useState(false); // show manual accept controls after failure

    // Derived helper flags
    const canSignIn = !busy && !loading; // Always allow sign-in regardless of invite validity
    const canManualAccept = inviteOk && ssoReady && !busy && !loading; // manual accept only when preconditions satisfied

    // Resolve token from params (handle Promise or plain object) then load invite
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

    // Load invite details (name/email) and lock them in the UI
    useEffect(() => {
        let cancelled = false;
        (async () => {
            if (!token) { setLoading(false); return; }
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

    // Detect SSO session; differentiate between presence vs email claim availability
    useEffect(() => {
        let cancelled = false;
        (async () => {
            try {
                const res = await fetch('/api/auth/session', { cache: 'no-store' });
                if (!res.ok) { if (!cancelled) { setSsoReady(false); setSsoEmail(null); setSsoSession(false); } return; }
                const data = await res.json();
                const email: string | undefined = data?.user?.email;
                if (cancelled) return;
                setSsoSession(!!data?.user);
                setSsoReady(!!email); // ready only when email claim exists
                setSsoEmail(email ?? null);
            } catch {
                if (!cancelled) { setSsoReady(false); setSsoEmail(null); setSsoSession(false); }
            }
        })();
        return () => { cancelled = true; };
    }, []);

    useEffect(() => {
        // One automatic attempt after successful sign-in when all preconditions met
        if (ssoReady && inviteOk && !loading && !busy && !autoExchangedRef.current) {
            autoExchangedRef.current = true;
            performExchange(true);
        }
    }, [ssoReady, inviteOk, loading, busy]);

    async function performExchange(isAuto = false) {
        setExchangeErrCode(null);
        setErr('');
        try {
            setBusy(true);
            const ex = await fetch('/api/auth/student/exchange', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ token }) });
            const data = await ex.json();
            if (!ex.ok) {
                setExchangeErrCode(ex.status);
                let message = data?.error || 'Invite acceptance failed';
                if (ex.status === 401) message = 'Please sign in with Keycloak first.';
                else if (ex.status === 403) message = `You are signed in as ${ssoEmail || 'an account'} but this invite is for ${email}. Sign out and sign in with the invited email.`;
                else if (ex.status === 400) message = data?.error || 'Invite invalid or already used.';
                throw new Error(message);
            }
            const { token: studentToken, student } = data || {};
            localStorage.setItem('sqlgrader.student', JSON.stringify({ token: studentToken, id: student?.id, email: student?.email, name: student?.name, instructorId: student?.instructorId }));
            setUser({ name: student?.email, role: 'student', token: studentToken, instructorId: student?.instructorId });
            setNote(isAuto ? 'Invite accepted via SSO. Redirecting...' : 'Invite accepted. Redirecting...');
            try { sessionStorage.setItem('appshell.active', 's-dash'); } catch { }
            window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 's-dash' } }));
            router.push('/');
        } catch (e: unknown) {
            setErr(e instanceof Error ? e.message : 'Invite acceptance failed');
            // Switch to manual mode on first failure so user can retry
            setManualMode(true);
        } finally {
            setBusy(false);
        }
    }

    async function acceptWithSSO() {
        // Establish / refresh Keycloak session; automatic attempt or manual accept will follow
        setErr('');
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
                    {!!err && (
                        <Alert severity="error" sx={{ mb: 2 }}>
                            <Stack direction="row" spacing={1} alignItems="center" justifyContent="space-between">
                                <Typography variant="body2" sx={{ flex: 1 }}>{err}</Typography>
                                <Stack direction="row" spacing={1}>
                                    {exchangeErrCode === 403 && (
                                        <Button size="small" variant="text" color="secondary" startIcon={<LogoutIcon fontSize="small" />} onClick={() => { window.location.href = '/api/auth/idp-logout?redirect=/invite/' + encodeURIComponent(token); }}>Sign out</Button>
                                    )}
                                    {exchangeErrCode === 401 && (
                                        <Button size="small" variant="outlined" color="secondary" onClick={acceptWithSSO}>Sign in</Button>
                                    )}
                                    {manualMode && inviteOk && ssoReady && (
                                        <Button size="small" variant="outlined" color="primary" startIcon={<ReplayIcon fontSize="small" />} disabled={busy} onClick={() => performExchange(false)}>Retry</Button>
                                    )}
                                </Stack>
                            </Stack>
                        </Alert>
                    )}
                    {ssoSession && !ssoReady && !note && !err && (
                        <Alert severity="warning" sx={{ mb: 2 }}>
                            Your Keycloak profile does not provide an email claim. Please update your profile or contact an administrator.
                        </Alert>
                    )}
                    <Stack spacing={1.5}>
                        <TextField label="Full name" size="small" fullWidth value={name} disabled />
                        <TextField label="Email" size="small" fullWidth value={email} disabled />
                        <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1} justifyContent="center" alignItems="stretch">
                            <Button
                                variant="outlined"
                                color="secondary"
                                startIcon={<KeyIcon />}
                                onClick={acceptWithSSO}
                                disabled={!canSignIn}
                                sx={{
                                    borderColor: (t) => t.palette.secondary.main,
                                    '&:hover': { background: (t) => `${t.palette.secondary.main}12` },
                                }}
                            >
                                {ssoSession ? (ssoReady ? `Signed in as ${ssoEmail}` : 'Session detected') : 'Continue with Keycloak'}
                            </Button>
                            {canManualAccept && (
                                <Button
                                    variant="contained"
                                    color="primary"
                                    startIcon={<CheckIcon />}
                                    onClick={() => performExchange(false)}
                                    disabled={!canManualAccept}
                                >
                                    Accept invite
                                </Button>
                            )}
                        </Stack>
                        {!inviteOk && !loading && (
                            <Typography variant="caption" color="text.secondary" sx={{ textAlign: 'center' }}>
                                You can still sign in; request a new invite afterward if needed.
                            </Typography>
                        )}
                    </Stack>
                </CardContent>
            </Card>
        </Box>
    );
}