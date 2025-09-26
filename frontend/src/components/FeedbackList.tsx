"use client";
import React from 'react';
import { Box, Typography } from '@mui/material';

export default function FeedbackList({ feedback, emptyText }: { feedback: string[]; emptyText: string }) {
    return feedback.length ? (
        <Box component='ul' sx={{ pl: 2.25, my: 0, listStyle: 'none' }}>
            {feedback.map((f, i) => (
                <li key={i}><Typography variant='body2' sx={{ whiteSpace: 'pre-line' }}>{f}</Typography></li>
            ))}
        </Box>
    ) : (
        <Typography variant='caption' color='text.secondary'>{emptyText}</Typography>
    );
}
