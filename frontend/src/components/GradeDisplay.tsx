"use client";
import React from 'react';
import { Typography } from '@mui/material';

export default function GradeDisplay({ grade, denom, loadingLabel }: { grade: number | null | undefined; denom: number | string | null | undefined; loadingLabel?: string }) {
    const has = typeof grade === 'number' && isFinite(grade);
    return (
        <>
            <Typography variant='body2' color='text.secondary'>Grade</Typography>
            <Typography variant='h4' sx={{ lineHeight: 1 }}>
                {has ? (grade as number).toFixed(2) : '—'}
                {denom != null && (
                    <Typography component='span' variant='body2' color='text.secondary'> / {denom}</Typography>
                )}
            </Typography>
        </>
    );
}
