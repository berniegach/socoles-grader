'use client';
import { useState } from 'react';
import { Box, Button, Card, CardActions, CardContent, CardHeader, Divider, FormControlLabel, Switch, TextField, Typography } from '@mui/material';
import Logo from '@/components/Logo';
import GroupIcon from '@mui/icons-material/GroupOutlined';
import AssignmentIcon from '@mui/icons-material/AssignmentTurnedInOutlined';
import LoginIcon from '@mui/icons-material/Login';
import { useAuth } from './AuthProvider';
import type { Role } from '@/lib/types';


export default function Login() {
    const { setUser } = useAuth();
    const [role, setRole] = useState<Role>('student');
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [isSignup, setIsSignup] = useState(false);
    const [confirm, setConfirm] = useState('');
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);


    return (
        <Box sx={{ minHeight: '100vh', display: 'grid', placeItems: 'center', p: 2, background: (t) => `linear-gradient(to bottom, ${t.palette.background.default}, #eef2f7)` }}>
            <Card sx={{ width: '100%', maxWidth: 420, borderRadius: 3, boxShadow: 6 }}>
                <CardHeader
                    sx={{ pb: 1, textAlign: 'center' }}
                    title={
                        <Box sx={{ display: 'grid', placeItems: 'center' }}>
                            <Logo height={48} variant="default" sx={{ mb: 1 }} />

                            <Typography variant="h6" sx={{ fontWeight: 700, mb: 0.5 }}>
                                SOCOLES
                            </Typography>
                            <Typography variant="body2" color="text.secondary">
                                Sign in to continue
                            </Typography>
                        </Box>
                    }
                />
                <CardContent>
                    <Box sx={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 1 }}>
                        <Button variant={role === 'student' ? 'contained' : 'outlined'} startIcon={<GroupIcon />} onClick={() => setRole('student')}>Student</Button>
                        <Button variant={role === 'instructor' ? 'contained' : 'outlined'} startIcon={<AssignmentIcon />} onClick={() => setRole('instructor')}>Instructor</Button>
                    </Box>
                    <Box sx={{ mt: 2, display: 'grid', gap: 1.5 }}>
                        <TextField variant="outlined" size="small" label="Email" placeholder="you@uni.nl" value={email} onChange={(e) => setEmail(e.target.value)} />
                        <TextField variant="outlined" size="small" type="password" label="Password" value={password} onChange={(e) => setPassword(e.target.value)} />
                        {role === 'instructor' && isSignup && (
                            <TextField variant="outlined" size="small" type="password" label="Confirm Password" value={confirm} onChange={(e) => setConfirm(e.target.value)} />
                        )}
                        {error && <Typography variant="caption" color="error">{error}</Typography>}
                        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
                            <FormControlLabel control={<Switch defaultChecked />} label={<Typography variant="caption">Remember me</Typography>} />
                            <Button
                                variant="contained"
                                endIcon={<LoginIcon />}
                                disabled={loading}
                                onClick={async () => {
                                    setError(null);
                                    if (!email || !password) { setError('Email & password required'); return; }
                                    if (role === 'instructor' && isSignup && password !== confirm) { setError('Passwords do not match'); return; }
                                    setLoading(true);
                                    try {
                                        if (role === 'student') {
                                            const res = await fetch('/api/student/login', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ email, password }) });
                                            const data = await res.json();
                                            if (!res.ok) throw new Error(data?.error || 'Login failed');
                                            const { token, student } = data;
                                            localStorage.setItem('sqlgrader.student', JSON.stringify({ token, email: student.email, id: student.id, name: student.name, instructorId: student.instructorId }));
                                            setUser({ name: student.email, role: 'student', token, instructorId: student.instructorId });
                                        } else {
                                            const endpoint = isSignup ? '/api/auth/signup' : '/api/auth';
                                            const res = await fetch(endpoint, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ email, password, name: email.split('@')[0] }) });
                                            const data = await res.json();
                                            if (!res.ok) throw new Error(data?.error || 'Login failed');
                                            const { token, instructor } = data;
                                            localStorage.setItem('sqlgrader.instructor', JSON.stringify({ token, email: instructor.email, id: instructor.id, name: instructor.name }));
                                            setUser({ name: instructor.email, role: 'instructor', token, instructorId: instructor.id });
                                            if (isSignup) setIsSignup(false);
                                        }
                                    } catch (e: any) {
                                        setError(e.message || 'Authentication failed');
                                    } finally {
                                        setLoading(false);
                                    }
                                }}
                            >{isSignup ? 'Create Account' : 'Sign in'}</Button>
                        </Box>
                        <Box sx={{ textAlign: 'right' }}>
                            {role === 'instructor' && (
                                <Button size="small" onClick={() => { setIsSignup(s => !s); setError(null); }}>
                                    {isSignup ? 'Have an account? Sign in' : 'Need an account? Sign up'}
                                </Button>
                            )}
                        </Box>
                    </Box>
                </CardContent>
                <Divider />

            </Card>
        </Box>
    );
}