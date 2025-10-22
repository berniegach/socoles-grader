"use client";
import React, { useEffect, useState } from 'react';
import { Box, Button, Card, CardContent, CardHeader, Typography } from '@mui/material';
import MuiLink from '@mui/material/Link';
import NextLink from 'next/link';
import Logo from '@/components/Logo';
import KeyIcon from '@mui/icons-material/VpnKey';
import { signIn } from 'next-auth/react';
import { useAuth } from '@/features/auth/AuthProvider';

export default function StudentLoginPage() {
    const { setUser } = useAuth();
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [ssoReady, setSsoReady] = useState(false);
    const [ssoEmail, setSsoEmail] = useState<string | null>(null);

    // Detect SSO session to show sign out helper
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
                {/* Gradient header to match the homepage cards */}
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
                                <Typography variant="h6" sx={{ fontWeight: 700, mt: 0.5 }}>Student</Typography>
                            </Box>
                        </Box>
                    }
                />

                <CardContent sx={{ p: 3 }}>
                    <Button
                        fullWidth
                        color="secondary"
                        variant="outlined"
                        startIcon={<KeyIcon />}
                        onClick={() => signIn('keycloak', { callbackUrl: '/auth/student/callback' })}
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