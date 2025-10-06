"use client";
import React from 'react';
import { Box, LinearProgress, Typography } from '@mui/material';

export type Rubric = { syntax: number | null; semantics: number | null; results: number | null; absent?: { syntax?: boolean; semantics?: boolean; results?: boolean } };

function Bar({ label, value, greyed }: { label: string; value: number | null; greyed?: boolean }) {
    const pct = typeof value === 'number' && isFinite(value) ? Math.round(value * 100) : 0;
    return (
        <Box sx={{ opacity: greyed ? 0.35 : 1 }}>
            <Typography variant='caption'>
                {label} {greyed ? '(n/a)' : `(${pct}%)`}
            </Typography>
            <LinearProgress variant='determinate' value={greyed ? 0 : pct} color={greyed ? 'inherit' : undefined} sx={greyed ? { bgcolor: (t) => t.palette.action.disabledBackground } : undefined} />
        </Box>
    );
}

export default function RubricBars({ rubric }: { rubric: Rubric }) {
    const absent = rubric.absent || {};
    return (
        <>
            <Typography variant='caption' color='text.secondary'>Rubric</Typography>
            <Box sx={{ display: 'grid', gap: .75, mt: 1 }}>
                <Bar label='Syntax' value={rubric.syntax} greyed={!!absent.syntax} />
                <Bar label='Semantics' value={rubric.semantics} greyed={!!absent.semantics} />
                <Bar label='Results' value={rubric.results} greyed={!!absent.results} />
            </Box>
        </>
    );
}
