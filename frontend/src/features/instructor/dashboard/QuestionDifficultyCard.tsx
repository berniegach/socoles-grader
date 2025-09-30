import React from 'react';
import { Card, CardHeader, CardContent, Typography } from '@mui/material';
import { ScatterChart } from '@mui/x-charts/ScatterChart';

export const QuestionDifficultyCard: React.FC<{ data: Array<{ id: string; title: string; attempts: number; best: number }> }> = ({ data }) => {
    const scatter = data.map(d => ({ x: d.attempts, y: d.best }));
    return (
        <Card>
            <CardHeader title={<Typography variant='subtitle1'>Question Difficulty</Typography>} subheader={<Typography variant='caption'>Attempts vs Best Grade</Typography>} />
            <CardContent>
                {scatter.length ? (
                    <ScatterChart height={220} series={[{ data: scatter }]} xAxis={[{ label: 'Attempts' }]} yAxis={[{ label: 'Best Grade' }]} />
                ) : <Typography variant='caption' color='text.secondary'>No data</Typography>}
            </CardContent>
        </Card>
    );
};
