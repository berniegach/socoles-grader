'use client';
import { Card, CardContent, CardHeader, Typography, Table, TableHead, TableRow, TableCell, TableBody } from '@mui/material';

export default function SoftTable({
    title,
    rows,
}: { title: string; rows: any[] }) {
    const headers = rows?.length ? Object.keys(rows[0]) : [];

    const formatCell = (header: string, value: unknown) => {
        const key = String(header).toLowerCase();
        if (['grade', 'score', 'percent', 'percentage'].includes(key) && typeof value === 'number' && isFinite(value)) {
            return String(value.toFixed(2));
        }
        return String(value ?? '');
    };

    return (
        <Card>
            <CardHeader title={<Typography variant="subtitle1">{title}</Typography>} />
            <CardContent>
                {rows?.length ? (
                    <Table size="small">
                        <TableHead>
                            <TableRow>
                                {headers.map((h) => <TableCell key={h}>{h}</TableCell>)}
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {rows.map((r, i) => (
                                <TableRow key={i}>
                                    {headers.map((h) => <TableCell key={h}>{formatCell(h, (r as any)[h])}</TableCell>)}
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                ) : (
                    <Typography color="text.secondary">No data.</Typography>
                )}
            </CardContent>
        </Card>
    );
}
