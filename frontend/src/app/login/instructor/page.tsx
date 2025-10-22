"use client";
import React, { Suspense, useEffect, useState } from 'react';
import { useRouter } from 'next/navigation';
import { useSearchParams } from 'next/navigation';
import { Box, Button, Card, CardContent, CardHeader, Divider, TextField, Typography } from '@mui/material';
import MuiLink from '@mui/material/Link';
import NextLink from 'next/link';
import Logo from '@/components/Logo';
import KeyIcon from '@mui/icons-material/VpnKey';
import { signIn, signOut } from 'next-auth/react';
import { useAuth } from '@/features/auth/AuthProvider';

function InstructorLoginPageInner() {
    const router = useRouter();
    const searchParams = useSearchParams();
    const { setUser } = useAuth();
    const [ssoReady, setSsoReady] = useState(false);
    const [ssoEmail, setSsoEmail] = useState<string | null>(null);
    const [signupCode, setSignupCode] = useState('');
    const [error, setError] = useState<string | null>(null);
    const [loading, setLoading] = useState(false);
    const [showProvision, setShowProvision] = useState(false);

    useEffect(() => {
        let cancelled = false;
        (async () => {
            try {
                const res = await fetch('/api/auth/session', { cache: 'no-store' });
                if (!res.ok) return;
                const data = await res.json();
                const email: string | undefined = data?.user?.email;
                if (!cancelled) { setSsoReady(!!email); setSsoEmail(email ?? null); }
            } catch { /* ignore */ }
        })();
        return () => { cancelled = true; };
    }, []);

    // If redirected with ?provision=1, show the code entry form
    useEffect(() => {
        if (searchParams && searchParams.get('provision') === '1') {
            setShowProvision(true);
        }
    }, [searchParams]);

    return (
        <Box
            sx={{
                minHeight: '100vh',
                display: 'grid',
                placeItems: 'center',
                p: 3,
                // Match homepage background: subtle radial accent over theme background
                background: (t) => `radial-gradient(900px 480px at 80% -10%, rgba(29, 78, 216, 0.08), transparent 60%), ${t.palette.background.default}`,
            }}
        >
            <style>{`
        @keyframes borderGradientMove {
          0% { background-position: 0% 50%; }
          50% { background-position: 100% 50%; }
          100% { background-position: 0% 50%; }
        }
      `}</style>

            <Card
                sx={{
                    width: '100%',
                    maxWidth: 460,
                    borderRadius: '6px',
                    overflow: 'hidden',
                    borderColor: 'rgba(29, 78, 216, 0.10)',
                    boxShadow: '0 6px 20px rgba(29, 78, 216, 0.08)',
                    transition: 'transform 120ms ease, box-shadow 120ms ease',
                    '&:hover': {
                        transform: 'translateY(-2px)',
                        boxShadow: '0 10px 26px rgba(29, 78, 216, 0.12)'
                    }
                }}
                variant="outlined"
            >
                {/* Gradient header bar to match homepage cards */}
                <CardHeader
                    sx={{ p: 0 }}
                    title={
                        <Box sx={{
                            background: 'linear-gradient(90deg,#191654,#43C6AC)',
                            color: 'primary.contrastText',
                            textAlign: 'center',
                            py: 1.25,
                            px: 2,
                        }}>
                            <Box sx={{ display: 'grid', placeItems: 'center' }}>
                                <Logo height={48} variant="default" />
                                <Typography variant="h6" sx={{ fontWeight: 700, mt: 0.5 }}>Instructor</Typography>
                            </Box>
                        </Box>
                    }
                />

                <CardContent sx={{ p: 3 }}>
                    {!showProvision ? (
                        <>
                            <Button
                                fullWidth
                                color="secondary"
                                variant="outlined"
                                startIcon={<KeyIcon />}
                                onClick={async () => {
                                    setError(null);
                                    setLoading(true);
                                    try {
                                        // Simulate SSO login and check provisioned status
                                        await signIn('keycloak', { callbackUrl: '/auth/instructor/callback' });
                                        // After SSO, try to exchange for legacy JWT
                                        const ex = await fetch('/api/auth/exchange', { method: 'POST' });
                                        if (ex.status === 403) {
                                            setShowProvision(true);
                                            setError(null);
                                            return;
                                        }
                                        if (!ex.ok) throw new Error('Failed to establish session. Please refresh.');
                                        const { token, instructor } = await ex.json();
                                        if (!token || !instructor?.id) throw new Error('Invalid session exchange result');
                                        try { localStorage.setItem('sqlgrader.instructor', JSON.stringify({ token, email: instructor.email, id: instructor.id, name: instructor.name })); } catch { }
                                        setUser({ name: instructor.email, role: 'instructor', token, instructorId: instructor.id });
                                    } catch (e: any) {
                                        setError(e.message || 'Sign in failed');
                                    } finally {
                                        setLoading(false);
                                    }
                                }}
                                sx={{
                                    borderColor: (t) => t.palette.secondary.main,
                                    '&:hover': { background: (t) => `${t.palette.secondary.main}12` },
                                }}
                            >
                                Sign in with Keycloak
                            </Button>

                            {ssoReady && (
                                <Box sx={{ mt: 1, display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
                                    <Typography variant="caption" color="text.secondary">
                                        {ssoEmail ? `Signed in with Keycloak as ${ssoEmail}` : 'Keycloak session detected'}
                                    </Typography>
                                    <Button size="small" variant="text" onClick={() => { window.location.href = '/api/auth/idp-logout?redirect=/'; }}>
                                        Sign out
                                    </Button>
                                </Box>
                            )}

                            {error && <Typography variant="caption" color="error" sx={{ mt: 1, display: 'block' }}>{error}</Typography>}
                        </>
                    ) : (
                        <>
                            <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                                Your instructor account is not yet set up. Enter your one-time signup code:
                            </Typography>

                            <Box sx={{ display: 'grid', gridTemplateColumns: '1fr auto', gap: 1 }}>
                                <TextField size="small" label="Signup code" value={signupCode} onChange={(e) => setSignupCode(e.target.value)} />
                                <Button
                                    variant="contained"
                                    onClick={async () => {
                                        setError(null);
                                        if (!signupCode) { setError('Signup code required'); return; }
                                        setLoading(true);
                                        try {
                                            const res = await fetch('/api/instructor/provision', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ signupCode }) });
                                            const data = await res.json();
                                            if (res.status === 401) throw new Error('Please sign in with Keycloak first');
                                            if (!res.ok) throw new Error(data?.error || 'Provisioning failed');
                                            const ex = await fetch('/api/auth/exchange', { method: 'POST' });
                                            if (!ex.ok) throw new Error('Provisioned, but failed to establish session. Please refresh.');
                                            const { token, instructor } = await ex.json();
                                            if (!token || !instructor?.id) throw new Error('Provisioned, but invalid session exchange result');
                                            try { localStorage.setItem('sqlgrader.instructor', JSON.stringify({ token, email: instructor.email, id: instructor.id, name: instructor.name })); } catch { }
                                            setUser({ name: instructor.email, role: 'instructor', token, instructorId: instructor.id });
                                            setShowProvision(false);
                                            router.replace('/');
                                        } catch (e: any) {
                                            setError(e.message || 'Provisioning failed');
                                        } finally {
                                            setLoading(false);
                                        }
                                    }}
                                    sx={{
                                        background: (t) => t.palette.primary.main,
                                        '&:hover': { background: (t) => t.palette.primary.dark },
                                    }}
                                >
                                    Submit
                                </Button>
                            </Box>

                            {error && <Typography variant="caption" color="error" sx={{ mt: 1, display: 'block' }}>{error}</Typography>}
                        </>
                    )}
                </CardContent>
            </Card>

            <Box sx={{ mt: 1.5, textAlign: 'center' }}>
                <MuiLink component={NextLink} href="/" color="text.secondary" underline="hover" sx={{ fontSize: '0.875rem' }}>
                    ← Back to role picker
                </MuiLink>
            </Box>
        </Box>
    );
}

export default function InstructorLoginPage() {
    return (
        <Suspense>
            <InstructorLoginPageInner />
        </Suspense>
    );
}
