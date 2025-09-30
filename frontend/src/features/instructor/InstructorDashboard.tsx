'use client';
import React, { useEffect, useState } from 'react';
import PageCard from '@/components/PageCard';
import { Typography, Box, Button } from '@mui/material';
import { useAuth } from '@/features/auth/AuthProvider';
import { fetchDashboardMetrics } from './dashboard/metricsApi';
import { DashboardMetrics, emptyMetrics } from './dashboard/types';
import { MetricsRow } from './dashboard/MetricsRow';
import { QuestionDifficultyCard } from './dashboard/QuestionDifficultyCard';
import { AtRiskQuestionsCard } from './dashboard/AtRiskQuestionsCard';
import { RecentActivityCard } from './dashboard/RecentActivityCard';
import DescriptionIcon from '@mui/icons-material/DescriptionOutlined';
import TaskAltIcon from '@mui/icons-material/TaskAlt';
import WarningIcon from '@mui/icons-material/WarningAmber';
import GroupIcon from '@mui/icons-material/Group';
import { useRosterMap } from '@/lib/useRosterMap';

export default function InstructorDashboard() {
    const { authFetch } = useAuth();
    const [metrics, setMetrics] = useState<DashboardMetrics>(emptyMetrics);
    const [loading, setLoading] = useState(true);
    const rosterMap = useRosterMap();
    useEffect(() => {
        let cancelled = false;
        (async () => {
            try { const m = await fetchDashboardMetrics(authFetch); if (!cancelled) setMetrics(m); } finally { if (!cancelled) setLoading(false); }
        })();
        return () => { cancelled = true; };
    }, [authFetch]);

    const hero = [
        { label: 'Pending Reviews', value: metrics.pendingReviews, icon: <WarningIcon fontSize='small' />, color: 'warning' },
        { label: 'Avg Grade', value: loading ? '—' : (metrics.avgGrade).toFixed(1), icon: <DescriptionIcon fontSize='small' />, color: 'info' },
        { label: 'Pass Rate', value: loading ? '—' : (metrics.passRate * 100).toFixed(0) + '%', icon: <TaskAltIcon fontSize='small' />, color: 'success' },
        { label: 'Avg Attempts', value: loading ? '—' : metrics.avgAttempts.toFixed(1), icon: <GroupIcon fontSize='small' />, color: 'secondary' },
    ];

    return (
        <PageCard>
            <Typography variant='h6' sx={{ mb: 2 }}>Dashboard</Typography>
            <MetricsRow metrics={hero} />
            <Box sx={{ display: 'grid', gap: 2, mt: 2, gridTemplateColumns: { xs: '1fr', md: '1fr 1fr' } }}>
                <QuestionDifficultyCard data={metrics.questionDifficulty.slice(0, 20)} />
                <AtRiskQuestionsCard data={metrics.atRisk} />
            </Box>
            <Box sx={{ display: 'grid', gap: 2, mt: 2, gridTemplateColumns: { xs: '1fr' } }}>
                <RecentActivityCard data={metrics.recent.slice(0, 3).map(r => ({ ...r, student: rosterMap[r.student] || r.student }))} />
            </Box>
            {/* Compact single-line quick actions */}
            <Box sx={{ mt: 2, display: 'flex', gap: 1, flexWrap: 'wrap' }}>
                <Button size='small' variant='outlined' onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-assignments' } }))}>New Assignment</Button>
                <Button size='small' variant='outlined' onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-questions' } }))}>Add Question</Button>
                <Button size='small' variant='outlined' onClick={() => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 'i-class' } }))}>Import Roster</Button>
            </Box>
        </PageCard>
    );
}
