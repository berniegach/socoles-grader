'use client';
import { Box, Button, Dialog, DialogTitle, DialogContent, DialogActions, Tabs, Tab, LinearProgress, Typography, Card, CardContent, CardHeader } from '@mui/material';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SendIcon from '@mui/icons-material/Send';
import { useState } from 'react';
import CodeMirror from '@uiw/react-codemirror';
import { sql as sqlLang } from '@codemirror/lang-sql';

export default function EditorDialog({
    open, setOpen,
    sql, setSql,
    grading, grade, feedback, rubric,
    onRun,
}: {
    open: boolean; setOpen: (v: boolean) => void;
    sql: string; setSql: (v: string) => void;
    grading: boolean; grade: number | null; feedback: string[];
    rubric: { syntax: number | null; semantics: number | null; results: number | null; absent?: { syntax?: boolean; semantics?: boolean; results?: boolean } };
    onRun: () => void;
}) {
    const [tab, setTab] = useState(1); // 0=schema,1=editor
    function SqlEditor({ value, onChange, placeholder, readOnly }: { value: string; onChange: (v: string) => void; placeholder?: string; readOnly?: boolean }) {
        return (
            <Box sx={{ border: '1px solid', borderColor: 'divider', borderRadius: 1, overflow: 'hidden', resize: 'vertical', minHeight: 220, '& .cm-editor': { height: '100%', fontSize: 13, fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace', backgroundColor: 'background.paper' }, '& .cm-gutters': { borderRightColor: 'divider' } }}>
                <CodeMirror value={value} height='100%' extensions={[sqlLang()]} placeholder={placeholder} basicSetup={{ lineNumbers: true, highlightActiveLine: false, foldGutter: true, bracketMatching: true, autocompletion: true }} onChange={(v) => onChange(v)} editable={!readOnly} />
            </Box>
        );
    }
    return (
        <Dialog open={open} onClose={() => setOpen(false)} maxWidth="lg" fullWidth>
            <DialogTitle>Answer Editor</DialogTitle>
            <DialogContent dividers>
                <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { lg: '1fr 340px' } }}>
                    <Box>
                        <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mb: 1 }}>
                            <Tab label="Schema" />
                            <Tab label="Editor" />
                        </Tabs>

                        {tab === 0 && (
                            <Box sx={{ color: 'text.secondary', fontSize: 14 }}>
                                <p>Tables available:</p>
                                <ul style={{ marginTop: 8, paddingLeft: 18 }}>
                                    <li><code>Book(isbn, title, author)</code></li>
                                    <li><code>Copy(isbn, serial_number, weight, bookcase)</code></li>
                                </ul>
                            </Box>
                        )}

                        {tab === 1 && (
                            <SqlEditor value={sql} onChange={setSql} placeholder={'-- Write your SQL here\nSELECT * FROM Book;'} />
                        )}
                    </Box>

                    <Box sx={{ display: 'grid', gap: 2 }}>
                        <Card>
                            <CardHeader title={<Typography variant="subtitle1">Auto-Grader</Typography>} />
                            <CardContent>
                                <Box sx={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 1.5 }}>
                                    <Button onClick={onRun} disabled={grading} startIcon={<PlayArrowIcon />} variant="contained">
                                        {grading ? 'Grading…' : 'Run'}
                                    </Button>
                                    <Button startIcon={<SendIcon />} variant="outlined">Submit</Button>
                                </Box>

                                <Box sx={{ mt: 2 }}>
                                    <Typography variant="body2" color="text.secondary">Grade</Typography>
                                    {/* Format grade to two decimals and adjust denominator for ≤ 1 */}
                                    {(() => {
                                        const formatted = (typeof grade === 'number' && isFinite(grade)) ? grade.toFixed(2) : '—';
                                        const denom = (typeof grade === 'number') ? (grade <= 1 ? 1 : 10) : null;
                                        return (
                                            <Typography variant="h4" sx={{ lineHeight: 1 }}>
                                                {formatted}{denom != null ? <Typography component="span" variant="body2" color="text.secondary"> / {denom}</Typography> : null}
                                            </Typography>
                                        );
                                    })()}
                                </Box>

                                <Box sx={{ mt: 2, display: 'grid', gap: 1 }}>
                                    <Typography variant="body2" color="text.secondary">Rubric</Typography>
                                    {(['syntax', 'semantics', 'results'] as const).map(key => {
                                        const val = rubric[key];
                                        const absent = !!rubric.absent?.[key];
                                        const pct = typeof val === 'number' ? Math.round(val * 100) : 0;
                                        const label = key.charAt(0).toUpperCase() + key.slice(1);
                                        return (
                                            <Box key={key} sx={{ opacity: absent ? 0.35 : 1 }}>
                                                <Typography variant="caption">{label} {absent ? '(n/a)' : `(${pct}%)`}</Typography>
                                                <LinearProgress variant="determinate" value={absent ? 0 : pct} />
                                            </Box>
                                        );
                                    })}
                                </Box>
                            </CardContent>
                        </Card>

                        <Card>
                            <CardHeader title={<Typography variant="subtitle1">Feedback</Typography>} />
                            <CardContent>
                                {feedback?.length ? (
                                    <ul style={{ paddingLeft: 18, margin: 0 }}>
                                        {feedback.map((f, i) => <li key={i}><Typography variant="body2">{f}</Typography></li>)}
                                    </ul>
                                ) : (
                                    <Typography variant="body2" color="text.secondary">Run the auto-grader to see comments.</Typography>
                                )}
                            </CardContent>
                        </Card>
                    </Box>
                </Box>
            </DialogContent>
            <DialogActions>
                <Typography variant="caption" color="text.secondary">Your instructor may allow multiple submissions. Best score counts.</Typography>
                <Box sx={{ flexGrow: 1 }} />
                <Button onClick={() => setOpen(false)}>Close</Button>
            </DialogActions>
        </Dialog>
    );
}
