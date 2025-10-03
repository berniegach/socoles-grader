'use client';
import * as React from 'react';
import { Button, ButtonProps } from '@mui/material';

export interface HeaderActionButtonProps extends Omit<ButtonProps, 'variant' | 'size'> {
    /** Keep visual consistent: small size, contained style with translucent background */
    emphasis?: 'default' | 'high';
}

// Extracted style from InstructorDashboard inline button styling; no visual changes intended.
export const HeaderActionButton: React.FC<HeaderActionButtonProps> = ({ children, emphasis = 'default', sx, ...rest }) => {
    const base = emphasis === 'high' ? 'rgba(255,255,255,0.30)' : 'rgba(255,255,255,0.22)';
    const hover = emphasis === 'high' ? 'rgba(255,255,255,0.40)' : 'rgba(255,255,255,0.33)';
    return (
        <Button
            variant='contained'
            size='small'
            disableElevation
            {...rest}
            sx={{
                px: 1.5,
                fontWeight: 600,
                letterSpacing: 0.25,
                backdropFilter: 'blur(4px)',
                WebkitBackdropFilter: 'blur(4px)',
                background: base,
                color: '#fff',
                textTransform: 'none',
                '&:hover': { background: hover, borderColor: 'rgba(255,255,255,0.55)' },
                ...sx
            }}
        >
            {children}
        </Button>
    );
};
export default HeaderActionButton;
