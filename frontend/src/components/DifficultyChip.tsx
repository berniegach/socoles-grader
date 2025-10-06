"use client";
import * as React from 'react';
import { Chip } from '@mui/material';
import { useTheme } from '@mui/material/styles';

export interface DifficultyChipProps {
    value: string; // Beginner | Intermediate | Advanced (case-insensitive)
    size?: 'small' | 'medium';
    outlined?: boolean; // force outlined style
}

const palette = {
    beginner: { base: '#2E8B57' },      // sea green
    intermediate: { base: '#FF9800' },  // orange 500
    advanced: { base: '#9C27B0' }       // purple 600
};

export const DifficultyChip: React.FC<DifficultyChipProps> = ({ value, size = 'small', outlined }) => {
    const theme = useTheme();
    const key = (value || '').toLowerCase() as 'beginner' | 'intermediate' | 'advanced';
    const colorDef = palette[key] || palette.beginner;
    const dark = theme.palette.mode === 'dark';
    // Adjusts for dark mode: lighten foreground backgrounds slightly
    const bg = outlined ? 'transparent' : (dark ? `${colorDef.base}26` : `${colorDef.base}20`); // ~15% alpha dark, ~12.5% light
    const border = outlined ? colorDef.base : (dark ? `${colorDef.base}55` : `${colorDef.base}50`);
    const fg = outlined ? colorDef.base : (dark ? '#ffffff' : '#1d1d1d');

    return (
        <Chip
            size={size}
            label={value}
            sx={{
                fontWeight: 500,
                textTransform: 'capitalize',
                bgcolor: bg,
                color: fg,
                border: '1px solid',
                borderColor: border,
                letterSpacing: 0.15,
                '& .MuiChip-label': { px: 0.75 }
            }}
            variant={outlined ? 'outlined' : 'filled'}
        />
    );
};

export default DifficultyChip;
