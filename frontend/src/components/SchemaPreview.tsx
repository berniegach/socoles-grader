"use client";
import React from 'react';
import { Box, Typography } from '@mui/material';
import { extractSchema } from '@/lib/sqlSchema';

export default function SchemaPreview({ sql, emptyText = 'No CREATE TABLE statements detected.' }: { sql: string; emptyText?: string }) {
    const schema = extractSchema(sql);
    if (!schema.length) return <Typography variant='caption' color='text.secondary'>{emptyText}</Typography>;
    return (
        <Box sx={{ mt: .5, display: 'grid', gap: .5 }}>
            {schema.map(t => (
                <Typography key={t.name} variant='caption' sx={{ fontFamily: 'ui-monospace,monospace' }}>
                    {t.name}(
                    {t.columns.slice(0, 12).map((c, i, arr) => (
                        <Box key={c.name + i} component='span' sx={c.pk ? { textDecoration: 'underline', textDecorationColor: 'success.main', textDecorationThickness: '2px', textUnderlineOffset: '2px' } : undefined}>
                            {c.name}{i < arr.length - 1 ? ', ' : ''}
                        </Box>
                    ))}
                    {t.columns.length > 12 ? ', …' : ''}
                    )
                </Typography>
            ))}
        </Box>
    );
}
