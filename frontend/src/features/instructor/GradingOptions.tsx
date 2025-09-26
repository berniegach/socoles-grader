'use client';
import { ChangeEvent } from 'react';
import { Box, FormControl, InputLabel, Select, MenuItem, Switch, FormControlLabel, TextField, SelectChangeEvent, Typography, Stack } from '@mui/material';
import type { GradingOptions } from '@/lib/types';

export default function GradingOptions({ options, onChange, questionNumbers, selectedQuestionNumber, onChangeQuestionNumber, showHeader = true }: { options: GradingOptions; onChange: (key: keyof GradingOptions, value: any) => void; questionNumbers?: string[]; selectedQuestionNumber?: string; onChangeQuestionNumber?: (value: string) => void; showHeader?: boolean; }) {
    const handleSelect = (key: keyof GradingOptions) => (e: SelectChangeEvent<string>) => onChange(key, e.target.value);
    const handleText = (key: keyof GradingOptions) => (e: ChangeEvent<HTMLInputElement>) => onChange(key, e.target.value);

    return (
        <Box sx={{ mt: 2 }}>
            {showHeader && (
                <Typography variant="subtitle1" fontWeight={600} sx={{ mb: 1 }}>Grading Options</Typography>
            )}
            <Stack spacing={2}>
                {/* Question Number Selector */}
                {!!(questionNumbers && questionNumbers.length) && (
                    <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                        <FormControl sx={{ flex: 1, minWidth: 240 }} size="small">
                            <InputLabel id="question-number-label">Question Number</InputLabel>
                            <Select
                                labelId="question-number-label"
                                id="question-number"
                                value={selectedQuestionNumber || ''}
                                label="Question Number"
                                onChange={(e) => onChangeQuestionNumber && onChangeQuestionNumber(e.target.value)}
                            >
                                <MenuItem value="">Select…</MenuItem>
                                {questionNumbers.map((q) => (
                                    <MenuItem key={q} value={q}>{q}</MenuItem>
                                ))}
                            </Select>
                        </FormControl>
                    </Box>
                )}

                <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                    <FormControl sx={{ flex: 1, minWidth: 240 }} size="small">
                        <InputLabel id="syntax-sensitivity-label">Syntax Sensitivity</InputLabel>
                        <Select labelId="syntax-sensitivity-label" id="syntax-sensitivity" value={options.syntaxSensitivity} label="Syntax Sensitivity" onChange={handleSelect('syntaxSensitivity')}>
                            <MenuItem value="">Select…</MenuItem>
                            <MenuItem value="0 Absent">Absent</MenuItem>
                            <MenuItem value="2 Levels">2 Levels</MenuItem>
                            <MenuItem value="3 Levels">3 Levels</MenuItem>
                            <MenuItem value="8 Levels">8 Levels</MenuItem>
                        </Select>
                    </FormControl>
                    <FormControl sx={{ flex: 1, minWidth: 240 }} size="small">
                        <InputLabel id="semantics-sensitivity-label">Semantics Sensitivity</InputLabel>
                        <Select labelId="semantics-sensitivity-label" id="semantics-sensitivity" value={options.semanticsSensitivity} label="Semantics Sensitivity" onChange={handleSelect('semanticsSensitivity')}>
                            <MenuItem value="">Select…</MenuItem>
                            <MenuItem value="0 Absent">Absent</MenuItem>
                            <MenuItem value="2 Levels">2 Levels</MenuItem>
                            <MenuItem value="3 Levels">3 Levels</MenuItem>
                            <MenuItem value="8 Levels">8 Levels</MenuItem>
                        </Select>
                    </FormControl>
                </Box>

                <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                    <FormControl sx={{ flex: 1, minWidth: 240 }} size="small">
                        <InputLabel id="results-sensitivity-label">Results Sensitivity</InputLabel>
                        <Select labelId="results-sensitivity-label" id="results-sensitivity" value={options.resultsSensitivity} label="Results Sensitivity" onChange={handleSelect('resultsSensitivity')}>
                            <MenuItem value="">Select…</MenuItem>
                            <MenuItem value="0 Absent">Absent</MenuItem>
                            <MenuItem value="2 Levels">2 Levels</MenuItem>
                            <MenuItem value="3 Levels">3 Levels</MenuItem>
                        </Select>
                    </FormControl>
                    <FormControl sx={{ flex: 1, minWidth: 240 }} size="small">
                        <InputLabel id="evaluation-priority-label">Evaluation Priority</InputLabel>
                        <Select labelId="evaluation-priority-label" id="evaluation-priority" value={options.evaluationPriority} label="Evaluation Priority" onChange={handleSelect('evaluationPriority')}>
                            <MenuItem value="">Select…</MenuItem>
                            <MenuItem value="1 - Syntax, Semantics, Results">1 - Syntax, Semantics, Results</MenuItem>
                            <MenuItem value="2 - Semantics, Syntax, Results">2 - Semantics, Syntax, Results</MenuItem>
                            <MenuItem value="3 - Results, Semantics, Syntax">3 - Results, Semantics, Syntax</MenuItem>
                            <MenuItem value="4 - Syntax, Results, Semantics">4 - Syntax, Results, Semantics</MenuItem>
                            <MenuItem value="5 - Semantics, Results, Syntax">5 - Semantics, Results, Syntax</MenuItem>
                            <MenuItem value="6 - Results, Syntax, Semantics">6 - Results, Syntax, Semantics</MenuItem>
                        </Select>
                    </FormControl>
                </Box>

                <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                    <TextField label="Text Edit Distance Threshold" type="number" inputProps={{ min: 0 }} sx={{ flex: 1, minWidth: 240 }} value={options.textEditDistance} onChange={handleText('textEditDistance')} variant="outlined" size="small" />
                    <TextField label="Tree Edit Distance Threshold" type="number" inputProps={{ min: 0 }} sx={{ flex: 1, minWidth: 240 }} value={options.treeEditDistance} onChange={handleText('treeEditDistance')} variant="outlined" size="small" />
                </Box>

                <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                    <FormControlLabel control={<Switch checked={options.checkOrder} onChange={(_, v) => onChange('checkOrder', v)} color="primary" />} label="Check Order" />
                    <FormControlLabel control={<Switch checked={options.use_postgresql} onChange={(_, v) => onChange('use_postgresql', v)} color="primary" />} label="Use PostgreSQL" />
                    <FormControlLabel control={<Switch checked={options.autoDB} onChange={(_, v) => onChange('autoDB', v)} color="primary" />} label="Auto DB" />
                </Box>

                {options.autoDB && (
                    <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
                        <TextField label="DB Name" sx={{ flex: 1, minWidth: 240 }} value={options.dbName} onChange={handleText('dbName')} variant="outlined" size="small" />
                        <TextField label="Number of DBs" type="number" inputProps={{ min: 1 }} sx={{ flex: 1, minWidth: 240 }} value={options.numberOfDBs} onChange={handleText('numberOfDBs')} variant="outlined" size="small" />
                    </Box>
                )}
            </Stack>
        </Box>
    );
}
