'use client';
import { useState, useRef, useMemo } from 'react';
import { Box, Button, Card, CardContent, CardHeader, Typography, Snackbar, Alert, Chip, Stack, LinearProgress } from '@mui/material';
import TuneIcon from '@mui/icons-material/Tune';
import BarChartIcon from '@mui/icons-material/BarChart';
import TableChartIcon from '@mui/icons-material/TableChart';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import FileDrop from '@/components/FileDrop';
import ResultsTable from '@/components/ResultsTable';
import ResultsCharts from '@/components/ResultsCharts';
import { parseCsvPreview, parseCsvNoHeader, sendBatchToSocoles, DEFAULT_GRADING_OPTIONS } from '@/lib/api';
import Papa from 'papaparse';
import GradingOptions from '@/features/instructor/GradingOptions';
import type { GradingOptions as GradingOptionsType } from '@/lib/types';
import PageCard from '@/components/PageCard';
import HeaderActions from '@/components/HeaderActions';

export default function BatchGrader() {
    const gradingDefaults = useMemo(() => {
        try {
            const raw = localStorage.getItem('sqlgrader.settings');
            if (!raw) return { ...DEFAULT_GRADING_OPTIONS };
            const parsed = JSON.parse(raw) || {};
            return { ...DEFAULT_GRADING_OPTIONS, ...(parsed.gradingDefaults || {}) };
        } catch { return { ...DEFAULT_GRADING_OPTIONS }; }
    }, []);
    const [studentCsv, setStudentCsv] = useState<File | null>(null);
    const [studentPreview, setStudentPreview] = useState<{ headers: string[]; rows: any[] }>({ headers: [], rows: [] });
    const [refCsv, setRefCsv] = useState<File | null>(null);
    const [refPreview, setRefPreview] = useState<{ headers: string[]; rows: any[] }>({ headers: [], rows: [] });
    const [initSql, setInitSql] = useState<File | null>(null);
    const [busy, setBusy] = useState(false);
    const [error, setError] = useState('');
    const [showSuccess, setShowSuccess] = useState(false);
    const [results, setResults] = useState<any[]>([]);
    const [rawCsv, setRawCsv] = useState('');

    // Question number selection
    const [questionNumbers, setQuestionNumbers] = useState<string[]>([]);
    const [selectedQuestionNumber, setSelectedQuestionNumber] = useState<string>('');

    const [options, setOptions] = useState<GradingOptionsType>({ ...gradingDefaults });

    // Ref to the left column for scrolling on reset (mobile)
    const leftColumnRef = useRef<HTMLDivElement | null>(null);

    function resetAll() {
        setStudentCsv(null);
        setRefCsv(null);
        setInitSql(null);
        setStudentPreview({ headers: [], rows: [] });
        setRefPreview({ headers: [], rows: [] });
        setResults([]);
        setError('');
        // reset to saved grading defaults if available
        try {
            const raw = localStorage.getItem('sqlgrader.settings');
            const parsed = raw ? JSON.parse(raw) : null;
            const saved = parsed && parsed.gradingDefaults ? parsed.gradingDefaults : null;
            setOptions({ ...(saved || DEFAULT_GRADING_OPTIONS) });
        } catch {
            setOptions({ ...DEFAULT_GRADING_OPTIONS });
        }
        setQuestionNumbers([]);
        setSelectedQuestionNumber('');
        setRawCsv('');
        // Bring uploads/options into view on small screens
        try { leftColumnRef.current?.scrollIntoView({ behavior: 'smooth', block: 'start' }); } catch { }
    }

    const requiredHeaders = ['Org Defined ID', 'Attempt #', 'Q #', 'Answer', 'Out Of'];

    async function onStudents(f: File) {
        setStudentCsv(f);
        const preview = await parseCsvPreview(f);
        setStudentPreview(preview);
        const missing = requiredHeaders.filter((h) => !preview.headers.includes(h));
        if (missing.length) throw new Error(`Missing headers: ${missing.join(', ')}`);
        // compute unique question numbers
        const qset = new Set<string>();
        (preview.rows || []).forEach((row: any) => {
            const q = String((row && row['Q #']) ?? '').trim();
            if (q) qset.add(q);
        });
        setQuestionNumbers(Array.from(qset).sort());
        setSelectedQuestionNumber('');
    }
    async function onRefs(f: File) { setRefCsv(f); const preview = await parseCsvNoHeader(f); setRefPreview(preview); }
    async function onInit(f: File) { setInitSql(f); }

    function isOptionsValid(o: GradingOptionsType) {
        if (!o.syntaxSensitivity || !o.semanticsSensitivity || !o.resultsSensitivity || !o.evaluationPriority) return false;
        if (o.textEditDistance === '' || isNaN(Number(o.textEditDistance)) || Number(o.textEditDistance) < 0) return false;
        if (o.treeEditDistance === '' || isNaN(Number(o.treeEditDistance)) || Number(o.treeEditDistance) < 0) return false;
        if (o.autoDB) {
            if (!o.dbName || !o.numberOfDBs) return false;
            const n = Number(o.numberOfDBs);
            if (!Number.isInteger(n) || n < 1) return false;
        }
        return true;
    }

    async function runBatch() {
        setBusy(true); setError(''); setResults([]); setRawCsv('');
        try {
            // Ensure an SQL file is always sent. If missing, warn and optionally create a dummy file.
            let initFileToSend: File | undefined = initSql || undefined;
            if (!initFileToSend) {
                const proceed = typeof window !== 'undefined' ? window.confirm('No init SQL provided. The backend expects an .sql file. Continue with an empty SQL file?') : true;
                if (!proceed) { setBusy(false); return; }
                initFileToSend = new File([''], 'empty.sql', { type: 'text/sql' });
            }

            const { type, payload, raw } = await sendBatchToSocoles({
                studentsFile: studentCsv || undefined,
                referencesFile: refCsv || undefined,
                initSqlFile: initFileToSend,
                options,
                selectedQuestionNumber,
            });
            if (type === 'json') {
                const rows: any[] = Array.isArray(payload) ? payload : ((payload as any).rows || (payload as any).results || []);
                setResults(rows);
            } else {
                setRawCsv(raw || ''); setResults(payload as any[]);
            }
            setShowSuccess(true);
        } catch (e: any) { setError(String(e.message || e)); } finally { setBusy(false); }
    }

    function downloadCsv() {
        // Build ordered headers (Feedback last) for consistent CSV export
        const keySet = new Set<string>();
        for (const r of results) Object.keys(r || {}).forEach((k) => keySet.add(k));
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

        const text = Papa.unparse({ fields: ordered, data: results });
        const blob = new Blob([text], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a'); a.href = url; a.download = 'socoles-results.csv'; a.click(); URL.revokeObjectURL(url);
    }

    const allFilesReady = !!studentCsv && !!refCsv; // init SQL optional
    const canRun = !busy && allFilesReady && isOptionsValid(options) && !!selectedQuestionNumber;

    const headerActions = (
        <HeaderActions
            actions={[
                {
                    key: 'reset',
                    label: 'Reset batch grader',
                    ariaLabel: 'Reset',
                    onClick: resetAll,
                    disabled: busy,
                    icon: <RestartAltIcon fontSize='small' />,
                },
                {
                    key: 'run',
                    label: 'Run grader',
                    ariaLabel: 'Run grader',
                    onClick: runBatch,
                    disabled: !canRun,
                    icon: <PlayArrowIcon fontSize='small' />,
                    emphasis: 'high',
                },
            ]}
        />
    );

    return (
        <>
            <PageCard headerTitle='Batch Grader' headerProps={{ height: 56 }} headerActions={headerActions} headerActionsVariant='plain' loadingProgress={busy}>
                <Box sx={{ mb: 2, display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
                    <Chip size="small" label={allFilesReady ? 'Files ready' : 'Waiting for files'} color={allFilesReady ? 'success' : 'default'} />
                    <Chip size="small" label={selectedQuestionNumber ? `Q ${selectedQuestionNumber}` : 'No question selected'} color={selectedQuestionNumber ? 'info' : 'default'} />
                    {results.length > 0 && <Chip size="small" label={`${results.length} results`} />}
                </Box>

                <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', lg: '380px 1fr' }, gap: 2, alignItems: 'start' }}>
                    {/* Left column: Uploads + Options */}
                    <Box ref={leftColumnRef} sx={{ position: { lg: 'sticky' }, top: { lg: 16 }, alignSelf: 'start' }}>
                        <CardContent>
                            <Stack spacing={1.5}>
                                <FileDrop label="Student answers" accept=".csv" help={`Headers: ${requiredHeaders.join(', ')}`} onFile={onStudents} />
                                <FileDrop label="Reference statements" accept=".csv" help="1 column, no header; one statement per row." onFile={onRefs} />
                                <FileDrop label="Init database" accept=".sql" help="Optional for DQL-only grading." onFile={onInit} />
                            </Stack>
                            <Box sx={{ mt: 2, display: 'flex', flexWrap: 'wrap', gap: 1 }}>
                                {studentPreview.rows.length > 0 && (
                                    <Chip size="small" color="default" label={`Students: ${studentPreview.rows.length} rows`} />
                                )}
                                {refPreview.rows.length > 0 && (
                                    <Chip size="small" color="default" label={`Refs: ${refPreview.rows.length}`} />
                                )}
                                {!!initSql && <Chip size="small" label={initSql.name} />}
                            </Box>
                            {/* Removed developer-only form keys caption */}
                            {!!error && (
                                <Alert severity="error" sx={{ mt: 1 }}>{error}</Alert>
                            )}
                        </CardContent>

                        <Card variant="outlined">
                            <CardHeader titleTypographyProps={{ fontWeight: 700 }} title={<Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}><TuneIcon fontSize="small" />Grading Options</Box>} />
                            <CardContent>
                                <GradingOptions
                                    options={options}
                                    onChange={(key, value) => setOptions((prev) => ({ ...prev, [key]: value }))}
                                    questionNumbers={questionNumbers}
                                    selectedQuestionNumber={selectedQuestionNumber}
                                    onChangeQuestionNumber={setSelectedQuestionNumber}
                                    showHeader={false}
                                />
                                {(!isOptionsValid(options) || !selectedQuestionNumber) && (
                                    <Typography variant="caption" color="text.secondary" sx={{ mt: 1, display: 'block' }}>
                                        Complete all grading options and select a question number to enable grading.
                                    </Typography>
                                )}
                            </CardContent>
                        </Card>
                    </Box>

                    {/* Right column: Results */}
                    <Box>
                        <Card variant="outlined" sx={{ mb: 2 }}>
                            <CardHeader titleTypographyProps={{ fontWeight: 700 }} title={<Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}><BarChartIcon fontSize="small" />Overview</Box>} subheader={results.length ? `Graded ${results.length} rows` : 'No results yet'} />
                            {busy && <LinearProgress />}
                            <CardContent>
                                {results.length ? (
                                    <ResultsCharts results={results} />
                                ) : (
                                    <Typography variant="body2" color="text.secondary">Upload files, configure options, and run the grader to see charts.</Typography>
                                )}
                            </CardContent>
                        </Card>

                        <Card variant="outlined">
                            <CardHeader titleTypographyProps={{ fontWeight: 700 }} title={<Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}><TableChartIcon fontSize="small" />Results</Box>} action={
                                <Button size="small" variant="outlined" onClick={downloadCsv} disabled={!results.length}>Export CSV</Button>
                            } />
                            {busy && <LinearProgress />}
                            <CardContent>
                                <ResultsTable rows={results} />
                            </CardContent>
                        </Card>
                    </Box>
                </Box>
            </PageCard>

            {/* Success + Error snackbars */}
            <Snackbar open={!!error} autoHideDuration={6000} onClose={() => setError('')} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                <Alert onClose={() => setError('')} severity="error" sx={{ width: '100%' }}>
                    {error}
                </Alert>
            </Snackbar>
            <Snackbar open={showSuccess} autoHideDuration={3000} onClose={() => setShowSuccess(false)} anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}>
                <Alert onClose={() => setShowSuccess(false)} severity="success" sx={{ width: '100%' }}>
                    Grading complete{results.length ? ` — ${results.length} rows` : ''}.
                </Alert>
            </Snackbar>
        </>
    );
}
