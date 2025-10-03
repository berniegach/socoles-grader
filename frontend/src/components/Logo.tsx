'use client';
import * as React from 'react';
import { Box } from '@mui/material';

export interface LogoProps {
    variant?: 'default' | 'white' | 'bg';
    height?: number;
    title?: string;
    sx?: any; // allow passing sx directly (lightweight)
}

export const Logo: React.FC<LogoProps> = ({ variant = 'default', height = 56, title = 'SOCOLES', sx }) => {
    // All variants currently map to the unified scalable SVG asset.
    const src = '/icons/socoles-logo.svg';
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
