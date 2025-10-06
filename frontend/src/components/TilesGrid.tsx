"use client";
import * as React from 'react';
import { Box, BoxProps } from '@mui/material';

export interface TilesGridProps extends Omit<BoxProps, 'display'> {
    minColWidth?: number; // future enhancement: auto-fit (unused for now)
    columns?: { xs?: number; sm?: number; md?: number; lg?: number; xl?: number }; // override counts
    gapSize?: number | string; // spacing token
}

/**
 * TilesGrid: standard responsive grid for instructor dashboards.
 * Default columns: xs 1, sm 2, md 3, lg 4.
 * Accepts sx overrides; keeps consistent gap.
 */
const TilesGrid: React.FC<TilesGridProps> = ({
    children,
    columns,
    gapSize = 1.5,
    sx,
    ...rest
}) => {
    const cols = {
        xs: columns?.xs ?? 1,
        sm: columns?.sm ?? 2,
        md: columns?.md ?? 3,
        lg: columns?.lg ?? 4,
        xl: columns?.xl // optional
    };
    return (
        <Box
            sx={{
                display: 'grid',
                gap: gapSize,
                gridTemplateColumns: {
                    xs: `repeat(${cols.xs}, 1fr)`,
                    sm: `repeat(${cols.sm}, 1fr)`,
                    md: `repeat(${cols.md}, 1fr)`,
                    lg: `repeat(${cols.lg}, 1fr)`,
                    ...(cols.xl ? { xl: `repeat(${cols.xl}, 1fr)` } : {})
                },
                ...(sx as any)
            }}
            {...rest}
        >
            {children}
        </Box>
    );
};

export default TilesGrid;
