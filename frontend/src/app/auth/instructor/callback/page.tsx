"use client";
import React, { useEffect, useState } from 'react';
import { Box, Card, CardContent, CardHeader, CircularProgress, Typography } from '@mui/material';
import Logo from '@/components/Logo';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/features/auth/AuthProvider';

export default function InstructorCallback() {
    const router = useRouter();
    const { setUser } = useAuth();
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);

    useEffect(() => {
        (async () => {
            setLoading(true);
            try {
                const res = await fetch('/api/auth/exchange', { method: 'POST' });
                const data = await res.json();
                if (!res.ok) {
                    if (data?.error === 'instructor not provisioned') {
                        router.replace('/login/instructor?provision=1');
                        return;
                    }
                    throw new Error(data?.error || 'Exchange failed');
                }
                const { token, instructor } = data;
                if (!token || !instructor?.id) throw new Error('Invalid session');
                localStorage.setItem('sqlgrader.instructor', JSON.stringify({ token, email: instructor.email, id: instructor.id, name: instructor.name }));
                setUser({ name: instructor.email, role: 'instructor', token, instructorId: instructor.id });
                router.replace('/');
            } catch (e: any) {
                setError(e.message || 'SSO exchange failed');
                setLoading(false);
            }
        })();
    }, [router, setUser]);

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
                {/* Gradient header, consistent with login and homepage cards */}
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
                                <Typography variant="h6" sx={{ fontWeight: 700, mt: 0.5 }}>Finalizing sign in…</Typography>
                                <Typography variant="body2" sx={{ opacity: 0.9 }}>Please wait while we verify your session</Typography>
                            </Box>
                        </Box>
                    }
                />

                <CardContent sx={{ p: 4 }}>
                    {loading && (
                        <Box sx={{ display: 'grid', placeItems: 'center', py: 3 }}>
                            <CircularProgress />
                        </Box>
                    )}
                    {error && (
                        <Typography variant="caption" color="error" sx={{ display: 'block', textAlign: 'center' }}>
                            {error}
                        </Typography>
                    )}
                </CardContent>
            </Card>
        </Box>
    );
}