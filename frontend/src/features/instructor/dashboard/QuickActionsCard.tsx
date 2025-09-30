import React from 'react';
import { Card, CardHeader, CardContent, Button, Stack, Typography } from '@mui/material';

export const QuickActionsCard: React.FC = () => {
    const nav = (id: string) => window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id } }));
    return (
        <Card>
            <CardHeader title={<Typography variant='subtitle1'>Quick Actions</Typography>} />
            <CardContent>
                <Stack direction='row' spacing={1} flexWrap='wrap'>
                    <Button size='small' variant='contained' onClick={() => nav('i-assignments')}>New Assignment</Button>
                    <Button size='small' variant='outlined' onClick={() => nav('i-questions')}>Add Question</Button>
                    <Button size='small' variant='outlined' onClick={() => nav('i-class')}>Import Roster</Button>
                </Stack>
            </CardContent>
        </Card>
    );
};
