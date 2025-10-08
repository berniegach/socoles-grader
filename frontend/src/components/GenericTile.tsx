"use client";
import React from 'react';
import { Box } from '@mui/material';
import type { SxProps, Theme } from '@mui/material/styles';

export interface GenericTileProps {
    onClick?: () => void;
    role?: string;
    tabIndex?: number;
    sx?: SxProps<Theme>;
    children?: React.ReactNode;
}

export const GenericTile: React.FC<GenericTileProps> = ({ onClick, role = onClick ? 'button' : undefined, tabIndex = 0, sx, children }) => {
    const sxArray = [
        (theme: Theme) => ({
            position: 'relative',
            p: 1.25,
            borderRadius: 1.25,
            cursor: onClick ? 'pointer' : 'default',
            display: 'grid',
            gap: 0.75,
            background: theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.03)' : 'linear-gradient(145deg,#ffffff,#f9f9f9)',
            border: '1px solid',
            borderColor: theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)',
            boxShadow: '0 1px 2px rgba(0,0,0,0.08)',
            transition: 'background .25s, box-shadow .25s, transform .25s, border-color .25s',
            outline: 'none',
            '&:hover': onClick ? { boxShadow: '0 4px 12px -2px rgba(0,0,0,0.25)', transform: 'translateY(-2px)', borderColor: theme.palette.primary.light } : {},
            '&:focus-visible': { boxShadow: '0 0 0 2px #fff, 0 0 0 4px #ff66c4' }
        }),
        ...(Array.isArray(sx) ? sx : sx ? [sx] : [])
    ];
    return (
        <Box onClick={onClick} role={role} tabIndex={role ? tabIndex : undefined} sx={sxArray}>
            {children}
        </Box>
    );
};

export default GenericTile;
