import React from 'react';
import { Card, CardHeader, CardContent, Typography, Table, TableHead, TableRow, TableCell, TableBody, Tooltip } from '@mui/material';

export const AtRiskQuestionsCard: React.FC<{ data: Array<{ id: string; title: string; assignmentId?: string; assignmentTitle?: string; attempts: number; best: number }> }> = ({ data }) => {
    // Group by assignmentId for numbering within assignment
    const numbering: Record<string, number> = {};
    return (
        <Card>
            <CardHeader title={<Typography variant='subtitle1'>At-Risk Questions</Typography>} subheader={<Typography variant='caption'>High attempts / low best grade</Typography>} />
            <CardContent>
                {data.length ? (
                    <Table size='small'>
                        <TableHead>
                            <TableRow>
                                <TableCell>Assignment</TableCell>
                                <TableCell>Q #</TableCell>
                                <TableCell>Attempts</TableCell>
                                <TableCell>Best</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {data.map((q) => {
                                const aid = q.assignmentTitle || q.assignmentId || '—';
                                numbering[aid] = (numbering[aid] || 0) + 1;
                                const qNum = numbering[aid];
                                return (
                                    <TableRow key={q.id}>
                                        <TableCell sx={{ maxWidth: 120, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{aid}</TableCell>
                                        <TableCell>
                                            <Tooltip title={q.title} placement='right'>
                                                <span>{qNum}</span>
                                            </Tooltip>
                                        </TableCell>
                                        <TableCell>{q.attempts}</TableCell>
                                        <TableCell>{q.best.toFixed(2)}</TableCell>
                                    </TableRow>
                                );
                            })}
                        </TableBody>
                    </Table>
                ) : <Typography variant='caption' color='text.secondary'>None</Typography>}
            </CardContent>
        </Card>
    );
};
