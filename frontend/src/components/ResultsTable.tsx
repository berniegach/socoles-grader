'use client';
import { Table, TableBody, TableCell, TableHead, TableRow, Typography, TableContainer, Paper, Box } from '@mui/material';


export default function ResultsTable({ rows = [] as any[] }) {
    if (!rows.length) return <Typography color="text.secondary">No results yet.</Typography>;

    // Collect all headers across rows (in case rows have varying keys)
    const keySet = new Set<string>();
    for (const r of rows) Object.keys(r || {}).forEach((k) => keySet.add(k));
    const headers = Array.from(keySet);

    const feedbackKey = headers.find((h) => h.toLowerCase() === 'feedback');
    const preferredOrder = [
        'Org Defined ID',
        'Student',
        'Last Name',
        'First Name',
        'Student ID',
        'Q #',
        'Attempt #',
        'Answer',
        'Score',
        'Out Of',
        'Percent',
        'Percentage',
    ];
    const ordered = [
        ...preferredOrder.filter((h) => headers.includes(h)),
        ...headers.filter((h) => h !== feedbackKey && !preferredOrder.includes(h)),
    ];
    if (feedbackKey) ordered.push(feedbackKey);

    const renderFeedback = (val: any) => {
        if (Array.isArray(val)) {
            return (
                <Box component="ul" sx={{ pl: 2, m: 0 }}>
                    {val.map((v: any, i: number) => (
                        <Box key={i} component="li" sx={{ wordBreak: 'break-word', whiteSpace: 'pre-wrap' }}>
                            {String(v)}
                        </Box>
                    ))}
                </Box>
            );
        }
        if (typeof val === 'object' && val !== null) {
            return <Box sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>{JSON.stringify(val, null, 2)}</Box>;
        }
        return <Box sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>{String(val ?? '')}</Box>;
    };

    const formatCell = (header: string, value: unknown) => {
        if (header && String(header).toLowerCase() === String(feedbackKey || '').toLowerCase()) return renderFeedback(value);
        // Grade-like numeric columns to two decimals
        const gradeHeaders = new Set(['score', 'grade', 'percent', 'percentage']);
        if (gradeHeaders.has(String(header).toLowerCase()) && typeof value === 'number' && isFinite(value)) {
            return String(value.toFixed(2));
        }
        return String(value ?? '');
    };

    return (
        <TableContainer component={Paper} variant="outlined" sx={{ maxHeight: 520 }}>
            <Table size="small" stickyHeader>
                <TableHead>
                    <TableRow>
                        {ordered.map((h) => (
                            <TableCell key={h} sx={{ fontWeight: 600 }}>{h}</TableCell>
                        ))}
                    </TableRow>
                </TableHead>
                <TableBody>
                    {rows.map((r, i) => (
                        <TableRow key={i} hover>
                            {ordered.map((h) => (
                                <TableCell key={h} sx={h === feedbackKey ? { minWidth: 320, whiteSpace: 'normal' } : undefined}>
                                    {formatCell(h, (r as any)[h])}
                                </TableCell>
                            ))}
                        </TableRow>
                    ))}
                </TableBody>
            </Table>
        </TableContainer>
    );
}