'use client';
import * as React from 'react';
import { Box } from '@mui/material';

export interface LogoProps {
    variant?: 'default' | 'white' | 'bg';
    height?: number;
    title?: string;
    sx?: any; // allow passing sx directly (lightweight)
}

/**
 * Centralized logo component so we can swap assets in one place.
 * variant:
 *  - default: colored logo (logo.svg) for light / neutral backgrounds
 *  - white: monochrome white logo (white.svg) for dark / primary backgrounds
 *  - bg: framed logo with its own background (logo + bg.svg) good for favicon/app icons
 */
export const Logo: React.FC<LogoProps> = ({ variant = 'default', height = 56, title = 'SOCOLES', sx }) => {
    // For now all variants point to the unified socoles-logo.png asset.
    // (If white/bg variants are re-introduced later, branch here by variant.)
    const src = '/icons/socoles-logo.png';
    return (
        <Box
            component="img"
            src={src}
            alt={title}
            title={title}
            sx={{ display: 'block', height, width: 'auto', ...sx }}
        />
    );
};

export default Logo;
