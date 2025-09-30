import React from 'react';
import { Card, CardHeader, CardContent, Typography, List, ListItem, ListItemText } from '@mui/material';

export const RecentActivityCard: React.FC<{ data: Array<{ id: string; student: string; assignment: string; date: string; grade: number | null; status: string }> }> = ({ data }) => {
    return (
        <Card>
            <CardHeader title={<Typography variant='subtitle1'>Recent Activity</Typography>} />
            <CardContent sx={{ pt: 0 }}>
                {data.length ? (
                    <List dense>
                        {data.map(item => (
                            <ListItem key={item.id} disableGutters sx={{ py: 0.5 }}>
                                <ListItemText
                                    primary={`${item.student} • ${item.assignment}`}
                                    secondary={`${item.date} • ${item.status}${typeof item.grade === 'number' ? ' • ' + item.grade.toFixed(2) : ''}`}
                                />
                            </ListItem>
                        ))}
                    </List>
                ) : <Typography variant='caption' color='text.secondary'>No recent submissions</Typography>}
            </CardContent>
        </Card>
    );
};
