import React from 'react';
import { Box, Tabs, Tab, Typography, CircularProgress } from '@mui/material';

export interface AttemptTabsProps {
    attempts: Array<any>;
    loading?: boolean;
    error?: string;
    attemptTab: number;
    setAttemptTab: (idx: number) => void;
    renderAttempt: (attempt: any) => React.ReactNode;
    noAttemptsText?: string;
}

const AttemptTabs: React.FC<AttemptTabsProps> = ({
    attempts,
    loading,
    error,
    attemptTab,
    setAttemptTab,
    renderAttempt,
    noAttemptsText = 'No attempts yet.'
}) => {
    if (loading) {
        return <Typography variant='caption' color='text.secondary' sx={{ display: 'inline-flex', alignItems: 'center', gap: .75 }}><CircularProgress size={12} /> Loading…</Typography>;
    }
    if (error) {
        return <Typography variant='caption' color='error'>{error}</Typography>;
    }
    if (!attempts.length) {
        return <Typography variant='caption' color='text.secondary'>{noAttemptsText}</Typography>;
    }
    const current = attempts[Math.min(Math.max(0, attemptTab), attempts.length - 1)];
    return (
        <Box>
            <Tabs value={Math.min(attemptTab, attempts.length - 1)} onChange={(_, v) => setAttemptTab(v)} variant='scrollable' scrollButtons='auto' sx={{ mb: 1 }}>
                {attempts.map((a, i) => (
                    <Tab key={a.id || i} label={`Attempt ${a.attempt}`} value={i} />
                ))}
            </Tabs>
            {renderAttempt(current)}
        </Box>
    );
};

export default AttemptTabs;
