"use client";
import React from 'react';
import { Box, LinearProgress, Typography } from '@mui/material';

export type Rubric = { correctness: number; style: number; efficiency: number };

export default function RubricBars({ rubric }: { rubric: Rubric }) {
    return (
        <>
            <Typography variant='caption' color='text.secondary'>Rubric</Typography>
            <Box sx={{ display: 'grid', gap: .75, mt: 1 }}>
                <Box>
                    <Typography variant='caption'>Correctness ({rubric.correctness}%)</Typography>
                    <LinearProgress variant='determinate' value={rubric.correctness} />
                </Box>
                <Box>
                    <Typography variant='caption'>Style ({rubric.style}%)</Typography>
                    <LinearProgress variant='determinate' value={rubric.style} />
                </Box>
                <Box>
                    <Typography variant='caption'>Efficiency ({rubric.efficiency}%)</Typography>
                    <LinearProgress variant='determinate' value={rubric.efficiency} />
                </Box>
            </Box>
        </>
    );
}
