"use client";
import React, { useEffect, useState } from 'react';
import { Box, Button, Card, CardContent, CardHeader, CircularProgress, FormControl, InputLabel, MenuItem, Select, Typography } from '@mui/material';
import Logo from '@/components/Logo';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/features/auth/AuthProvider';

export default function StudentCallback() {
    const router = useRouter();
    const { setUser } = useAuth();
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [choices, setChoices] = useState<Array<{ instructorId: string; courseName?: string | null }>>([]);
    const [selectedInstructorId, setSelectedInstructorId] = useState('');

    useEffect(() => {
        (async () => {
            setLoading(true);
            try {
                const res = await fetch('/api/auth/student/login-exchange', { method: 'POST' });
                const data = await res.json();
                if (res.status === 400 && data?.choices?.length) {
                    setChoices((data.choices as any[]).map((c) => ({ instructorId: c.instructorId, courseName: c.courseName })));
                    setLoading(false);
                    return;
                }
                if (!res.ok) throw new Error(data?.error || 'Login failed');
                const { token, student } = data;
                localStorage.setItem('sqlgrader.student', JSON.stringify({ token, email: student.email, id: student.id, name: student.name, instructorId: student.instructorId }));
                setUser({ name: student.email, role: 'student', token, instructorId: student.instructorId });
                router.replace('/');
            } catch (e: any) {
                setError(e.message || 'SSO login failed');
                setLoading(false);
            }
        })();
    }, [router, setUser]);

    const finishWithChoice = async () => {
        if (!selectedInstructorId) return;
        setLoading(true);
        setError(null);
        try {
            const res = await fetch('/api/auth/student/login-exchange', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ instructorId: selectedInstructorId }) });
            const data = await res.json();
            if (!res.ok) throw new Error(data?.error || 'Login failed');
            const { token, student } = data;
            localStorage.setItem('sqlgrader.student', JSON.stringify({ token, email: student.email, id: student.id, name: student.name, instructorId: student.instructorId }));
            setUser({ name: student.email, role: 'student', token, instructorId: student.instructorId });
            router.replace('/');
        } catch (e: any) {
            setError(e.message || 'Login failed');
            setLoading(false);
        }
    };

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
                {/* Gradient header consistent with other auth pages */}
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
                                <Typography variant="h6" sx={{ fontWeight: 700, mt: 0.5 }}>Finalizing student sign in…</Typography>
                                <Typography variant="body2" sx={{ opacity: 0.9 }}>Please wait while we verify your session</Typography>
                            </Box>
                        </Box>
                    }
                />

                <CardContent sx={{ p: 3 }}>
                    {loading && (
                        <Box sx={{ display: 'grid', placeItems: 'center', p: 4 }}>
                            <CircularProgress />
                        </Box>
                    )}

                    {!loading && choices.length > 1 && (
                        <Box sx={{ display: 'grid', gridTemplateColumns: '1fr auto', gap: 1 }}>
                            <FormControl size="small">
                                <InputLabel id="course-label">Course</InputLabel>
                                <Select labelId="course-label" label="Course" value={selectedInstructorId} onChange={(e) => setSelectedInstructorId(String(e.target.value))}>
                                    {choices.map((c) => (
                                        <MenuItem key={c.instructorId} value={c.instructorId}>{c.courseName || c.instructorId}</MenuItem>
                                    ))}
                                </Select>
                            </FormControl>
                            <Button variant="outlined" disabled={!selectedInstructorId} onClick={finishWithChoice}>Continue</Button>
                        </Box>
                    )}

                    {error && (
                        <Typography variant="caption" color="error" sx={{ mt: 2, display: 'block', textAlign: 'center' }}>
                            {error}
                        </Typography>
                    )}
                </CardContent>
            </Card>
        </Box>
    );
}