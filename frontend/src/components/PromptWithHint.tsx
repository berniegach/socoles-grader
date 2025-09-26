"use client";
import React from 'react';
import { Box, Button, Collapse, Typography } from '@mui/material';
import SafeRichText from '@/components/SafeRichText';

export default function PromptWithHint({
    prompt,
    hint,
    showHint,
    onToggle,
    promptKey,
    hintKeyPrefix,
}: {
    prompt: string;
    hint?: string | null;
    showHint: boolean;
    onToggle: () => void;
    promptKey?: string;
    hintKeyPrefix?: string;
}) {
    return (
        <Box>
            <SafeRichText
                key={promptKey}
                html={String(prompt || 'No prompt provided.')}
                sx={{ '& p': { my: .5 }, '& ul, & ol': { pl: 3, my: .5 } }}
            />
            {!!hint && (
                <Box sx={{ mt: 1 }}>
                    <Button size='small' variant='text' onClick={onToggle}>{showHint ? 'Hide hint' : 'Show hint'}</Button>
                    <Collapse in={showHint} unmountOnExit>
                        <Box sx={{ mt: .75, p: 1, borderRadius: 1, bgcolor: 'action.hover' }}>
                            <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mb: .5 }}>Hint</Typography>
                            <SafeRichText
                                key={`${hintKeyPrefix}-${showHint ? 'open' : 'closed'}`}
                                html={String(hint || '')}
                                sx={{ '& p': { my: .5 }, '& ul, & ol': { pl: 3, my: .5 } }}
                            />
                        </Box>
                    </Collapse>
                </Box>
            )}
        </Box>
    );
}
