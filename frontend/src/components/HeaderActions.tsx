"use client";
import * as React from 'react';
import { Box } from '@mui/material';
import HeaderActionButton from '@/components/HeaderActionButton';

export interface HeaderActionConfig {
    key: string;
    label?: string; // if omitted and only icon provided, show icon-only
    ariaLabel?: string;
    title?: string; // tooltip title
    icon?: React.ReactNode; // icon element; if provided w/ label appears at start
    onClick?: (e: React.MouseEvent) => void;
    disabled?: boolean;
    hidden?: boolean;
    emphasis?: 'default' | 'high';
}

export interface HeaderActionsProps {
    actions: HeaderActionConfig[];
    gap?: number;
    wrap?: boolean;
}

/** Renders a consistent row of header action buttons based on config array. */
const HeaderActions: React.FC<HeaderActionsProps> = ({ actions, gap = 0.75, wrap = false }) => {
    const visible = actions.filter(a => !a.hidden);
    if (!visible.length) return null;
    return (
        <Box sx={{ display: 'flex', alignItems: 'center', gap, flexWrap: wrap ? 'wrap' : 'nowrap' }}>
            {visible.map(a => (
                <HeaderActionButton
                    key={a.key}
                    onClick={a.onClick}
                    aria-label={a.ariaLabel || a.title || a.label}
                    title={a.title || a.label}
                    disabled={a.disabled}
                    emphasis={a.emphasis}
                    startIcon={a.icon && a.label ? a.icon as any : undefined}
                >
                    {a.label || (!a.label && a.icon ? a.icon : a.key)}
                </HeaderActionButton>
            ))}
        </Box>
    );
};

export default HeaderActions;
