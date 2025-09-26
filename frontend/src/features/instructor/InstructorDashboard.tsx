'use client';
import { Grid, Typography, Box, Button } from '@mui/material';
import StatCard from '@/components/StatCard';
import DescriptionIcon from '@mui/icons-material/DescriptionOutlined';
import CheckCircleIcon from '@mui/icons-material/TaskAlt';
import ErrorOutlineIcon from '@mui/icons-material/ErrorOutline';
import GroupIcon from '@mui/icons-material/Group';
import SoftTable from '@/components/SoftTable';
import ResultsCharts from '@/components/ResultsCharts';
import { useEffect, useState } from 'react';
import { useAuth } from '@/features/auth/AuthProvider';
import PageCard from '@/components/PageCard';
import { useRosterMap } from '@/lib/useRosterMap';

type SubmissionRow = { id: string; student: string; assignment: string; date: string; grade: number; status: string };
type AssignmentRow = { id: string; title: string; due: string };

export default function InstructorDashboard() {
    const [subs, setSubs] = useState<SubmissionRow[]>([]);
    const [assignments, setAssignments] = useState<AssignmentRow[]>([]);
    const [rosterCount, setRosterCount] = useState<number>(0);
    const now = new Date();
    const { user, authFetch } = useAuth();
    const rosterMap = useRosterMap();

    useEffect(() => {
        if (!user?.token) return; // wait for auth
        let cancelled = false;
        (async () => {
            try {
                const [sRes, aRes, rRes] = await Promise.all([
                    authFetch('/api/submissions'),
                    authFetch('/api/assignments'),
                    authFetch('/api/roster'),
                ]);
                if (!sRes.ok || !aRes.ok) return; // unauthorized etc.
                const sJson = (await sRes.json()) as SubmissionRow[];
                const aJson = (await aRes.json()) as any[];
                const rJson = rRes.ok ? (await rRes.json()) as unknown : [] as unknown;
                if (!cancelled) {
                    setSubs(Array.isArray(sJson) ? sJson : []);
                    setAssignments((Array.isArray(aJson) ? aJson : []).map(a => ({ id: a.id, title: a.title, due: a.due || '' })));
                    setRosterCount(Array.isArray(rJson as any[]) ? (rJson as any[]).length : 0);
                }
            } catch { /* ignore */ }
        })();
        return () => { cancelled = true; };
    }, [user, authFetch]);
    const withinDays = (d: string, days: number) => {
        const dt = new Date(d.replace(' ', 'T') + 'Z');
        const diff = (now.getTime() - dt.getTime()) / (1000 * 60 * 60 * 24);
        return diff >= 0 && diff <= days;
    };

    const submissions24h = subs.filter(s => withinDays(s.date, 1)).length;
    const needsReview = subs.filter(s => (s.status || '').toLowerCase().includes('needs')).length;
    const autoGraded = subs.filter(s => (s.status || '').toLowerCase().includes('auto')).length;
    const uniqueSubmitters7d = new Set(subs.filter(s => withinDays(s.date, 7)).map(s => s.student)).size;
    const passRateAll = (() => {
        const graded = subs.filter((s: SubmissionRow) => typeof s.grade === 'number');
        if (!graded.length) return '—';
        const passed = graded.filter((s: SubmissionRow) => (s.grade as number) >= 6).length;
        return `${Math.round((passed / graded.length) * 100)}%`;
    })();

    const dueSoon = assignments.filter(a => {
        const due = new Date(a.due + 'T00:00:00Z');
        const diff = (due.getTime() - now.getTime()) / (1000 * 60 * 60 * 24);
        return diff >= 0 && diff <= 3;
    });

    const openAssignments = assignments.length;
    const enrolled = rosterCount; // updated to use roster count

    const stats: [string, number | string, React.ElementType, string?][] = [
        ['Submissions (24h)', submissions24h, DescriptionIcon],
        ['Auto-graded', autoGraded, CheckCircleIcon],
        ['Needs review', needsReview, ErrorOutlineIcon],
        ['Active students (7d)', uniqueSubmitters7d || '—', GroupIcon],
    ];
    const stats2: [string, number | string, React.ElementType, string?][] = [
        ['Open assignments', openAssignments, DescriptionIcon],
        ['Due next 72h', dueSoon.length, ErrorOutlineIcon],
        ['Pass rate (all)', passRateAll, CheckCircleIcon],
        ['Enrolled', enrolled, GroupIcon],
    ];

    // Format grade helper for table display
    const fmt = (g: number | null | undefined) => (typeof g === 'number' && isFinite(g) ? g.toFixed(2) : '—');

    return (
        <PageCard>
            {/* Section heading */}
            <Typography variant="h6" sx={{ mb: 1 }}>Dashboard</Typography>

            {/* Top stats */}
            <Grid container spacing={2} sx={{ mb: 2 }}>
                {stats.map(([label, value, Icon]) => (
                    <Grid key={label} size={{ xs: 12, sm: 6, md: 3 }}>
                        <StatCard icon={Icon} label={label} value={value} />
                    </Grid>
                ))}
            </Grid>
            {/* Secondary stats */}
            <Grid container spacing={2} sx={{ mb: 2 }}>
                {stats2.map(([label, value, Icon]) => (
                    <Grid key={label} size={{ xs: 12, sm: 6, md: 3 }}>
                        <StatCard icon={Icon} label={label} value={value} />
                    </Grid>
                ))}
            </Grid>

            {/* Two-column: recent submissions + charts */}
            <Grid container spacing={2}>
                <Grid size={{ xs: 12, md: 6 }}>
                    <SoftTable
                        title="Recent Submissions"
                        rows={subs.map(s => ({
                            Student: rosterMap[s.student] || s.student,
                            Assignment: s.assignment,
                            Date: s.date,
                            Grade: fmt(s.grade as any),
                            Status: s.status,
                        }))}
                    />
                </Grid>
                <Grid size={{ xs: 12, md: 6 }}>
                    <ResultsCharts results={subs.map(s => ({
                        'Q #': s.assignment.includes('JOIN') ? 2 : 1,
                        Score: (typeof s.grade === 'number' ? s.grade : 0) / 10, // normalize to 0..1 for charts
                        'Out Of': 1,
                    }))} />
                </Grid>
            </Grid>

            {/* Quick actions */}
            <Box sx={{ mt: 2, display: 'flex', gap: 1.5, flexWrap: 'wrap' }}>
                <Button variant="contained" size="small" onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-assignments' } }))}>Create assignment</Button>
                <Button variant="outlined" size="small" onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-questions' } }))}>Add question</Button>
                <Button variant="outlined" size="small" onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-class' } }))}>Import roster</Button>
            </Box>
        </PageCard>
    );
}
