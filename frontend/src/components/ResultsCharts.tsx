'use client';
import { Box, Card, CardContent, CardHeader, Typography, Table, TableBody, TableCell, TableHead, TableRow } from '@mui/material';
import { BarChart } from '@mui/x-charts/BarChart';
import { PieChart } from '@mui/x-charts/PieChart';
import PeopleIcon from '@mui/icons-material/People';
import AssignmentIcon from '@mui/icons-material/Assignment';
import GradeIcon from '@mui/icons-material/Grade';
import ArrowUpwardIcon from '@mui/icons-material/ArrowUpward';
import ArrowDownwardIcon from '@mui/icons-material/ArrowDownward';
import { useTheme } from '@mui/material/styles';

export default function ResultsCharts({ results = [] as any[] }) {
    const theme = useTheme();
    // Normalize grades 0..1 and compute aggregates
    const dist = Array.from({ length: 11 }, (_, i) => ({ bin: (i / 10).toFixed(1), n: 0 })); // bins labeled 0.0..1.0
    const normalizedGrades: number[] = [];
    const answers: string[] = [];

    for (const r of results) {
        const q = String((r as any)['Q #'] ?? (r as any).q ?? (r as any).question ?? '?');
        const rawScore = Number((r as any).Score ?? (r as any).score ?? (r as any).Grade ?? (r as any).grade ?? 0);
        const outOf = Number((r as any)['Out Of'] ?? (r as any).outOf ?? (r as any)['OutOf'] ?? 1) || 1;
        const ans = String((r as any).Answer ?? (r as any).Query ?? (r as any).query ?? '').trim();

        // Normalize to 0..1
        let normalized = outOf > 1 ? rawScore / outOf : rawScore;
        if (!isFinite(normalized) || isNaN(normalized)) normalized = 0;
        normalized = Math.max(0, Math.min(1, normalized));
        normalizedGrades.push(normalized);
        if (ans) answers.push(ans);

        // Distribution
        let bin = Math.round(normalized * 10);
        bin = Math.max(0, Math.min(10, bin));
        dist[bin].n += 1;
    }

    // Summary stats
    const totalAnswers = results.length;
    const uniqueAnswers = new Set(answers).size;
    const avgGrade = normalizedGrades.length ? +(normalizedGrades.reduce((a, b) => a + b, 0) / normalizedGrades.length).toFixed(2) : 0;
    const maxGrade = normalizedGrades.length ? +Math.max(...normalizedGrades).toFixed(2) : 0;
    const minGrade = normalizedGrades.length ? +Math.min(...normalizedGrades).toFixed(2) : 0;

    // Pass/Fail pie (threshold 0.6)
    const pass = normalizedGrades.filter((g) => g >= 0.6).length;
    const fail = normalizedGrades.filter((g) => g < 0.6).length;
    const passFailData = [
        { id: 0, value: pass, label: 'Pass' },
        { id: 1, value: fail, label: 'Fail' },
    ];

    // Colors for bars (11 bins)
    const COLORS = [
        theme.palette.primary.light,
        theme.palette.secondary.light,
        theme.palette.success.light,
        theme.palette.warning.light,
        theme.palette.info.light,
        theme.palette.error.light,
        theme.palette.primary.main,
        theme.palette.secondary.main,
        theme.palette.success.main,
        theme.palette.warning.main,
        theme.palette.info.main,
    ];

    // Single series with per-category colors to keep bars thick
    const distDataset = dist.map((d) => ({ bin: d.bin, n: d.n }));

    // Common incorrect answers (grade < 1.0)
    const incorrectCounts: Record<string, number> = {};
    results.forEach((r) => {
        const rawScore = Number((r as any).Score ?? (r as any).score ?? (r as any).Grade ?? 0);
        const outOf = Number((r as any)['Out Of'] ?? (r as any).outOf ?? 1) || 1;
        let normalized = outOf > 1 ? rawScore / outOf : rawScore;
        if (!isFinite(normalized) || isNaN(normalized)) normalized = 0;
        const ans = String((r as any).Answer ?? (r as any).Query ?? '').trim();
        if (normalized < 1 && ans) incorrectCounts[ans] = (incorrectCounts[ans] || 0) + 1;
    });
    const commonIncorrect = Object.entries(incorrectCounts)
        .map(([answer, count]) => ({ answer, count }))
        .sort((a, b) => b.count - a.count)
        .slice(0, 5);

    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            {/* Summary stats */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', sm: 'repeat(2, 1fr)', md: 'repeat(3, 1fr)' }, gap: 2 }}>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <PeopleIcon color="primary" />
                        <Box>
                            <Typography variant="h5">{totalAnswers}</Typography>
                            <Typography color="text.secondary">Total Answers</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <AssignmentIcon color="secondary" />
                        <Box>
                            <Typography variant="h5">{uniqueAnswers}</Typography>
                            <Typography color="text.secondary">Unique Answers</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <GradeIcon color="success" />
                        <Box>
                            <Typography variant="h5">{avgGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Average Grade (0–1)</Typography>
                        </Box>
                    </CardContent>
                </Card>
            </Box>

            {/* High/Low */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', sm: '1fr 1fr' }, gap: 2 }}>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <ArrowUpwardIcon color="info" />
                        <Box>
                            <Typography variant="h5">{maxGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Highest Grade</Typography>
                        </Box>
                    </CardContent>
                </Card>
                <Card>
                    <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <ArrowDownwardIcon color="error" />
                        <Box>
                            <Typography variant="h5">{minGrade.toFixed(2)}</Typography>
                            <Typography color="text.secondary">Lowest Grade</Typography>
                        </Box>
                    </CardContent>
                </Card>
            </Box>

            {/* Visualizations */}
            <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', md: '1fr 1fr' }, gap: 2 }}>
                <Card>
                    <CardHeader title={<Typography variant="subtitle1">Grade Distribution (0–1)</Typography>} />
                    <CardContent>
                        <BarChart
                            height={260}
                            dataset={distDataset}
                            xAxis={[{
                                dataKey: 'bin',
                                label: 'Grade (0–1)',
                                categoryGapRatio: 0.15,
                                barGapRatio: 0.0,
                                colorMap: { type: 'ordinal', colors: COLORS, values: distDataset.map((d) => d.bin) },
                            }]}
                            series={[{ dataKey: 'n' }]}
                            hideLegend
                            borderRadius={2}
                            grid={{ horizontal: true }}
                        />
                    </CardContent>
                </Card>
                <Card>
                    <CardHeader title={<Typography variant="subtitle1">Pass/Fail Rate</Typography>} />
                    <CardContent>
                        <PieChart series={[{ data: passFailData }]} height={260} />
                    </CardContent>
                </Card>
            </Box>

            {/* Common Incorrect Answers */}
            <Card>
                <CardHeader title="Common Incorrect Answers" />
                <CardContent>
                    {commonIncorrect.length ? (
                        <Table size="small">
                            <TableHead>
                                <TableRow>
                                    <TableCell sx={{ fontWeight: 600 }}>Count</TableCell>
                                    <TableCell sx={{ fontWeight: 600 }}>Answer</TableCell>
                                </TableRow>
                            </TableHead>
                            <TableBody>
                                {commonIncorrect.map((item, idx) => (
                                    <TableRow key={idx}>
                                        <TableCell>{item.count}</TableCell>
                                        <TableCell sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>{item.answer}</TableCell>
                                    </TableRow>
                                ))}
                            </TableBody>
                        </Table>
                    ) : (
                        <Typography color="text.secondary">No common incorrect answers available.</Typography>
                    )}
                </CardContent>
            </Card>
        </Box>
    );
}
