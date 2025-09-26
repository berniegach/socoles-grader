'use client';
import * as React from 'react';
import { Card, CardProps } from '@mui/material';

export interface PageCardProps extends CardProps {
    noPadding?: boolean;
}

/**
 * PageCard: consistent outer container for main page sections.
 * - Adds responsive padding, margin bottom
 * - Handles light/dark background contrast
 * - Keeps outlined variant & divider border
 */
export const PageCard: React.FC<PageCardProps> = ({ children, noPadding, sx, ...rest }) => {
    return (
        <Card
            variant="outlined"
            sx={{
                p: noPadding ? 0 : { xs: 2, md: 3 },
                mb: 2,
                bgcolor: (theme) => theme.palette.mode === 'dark' ? 'background.paper' : 'grey.50',
                borderColor: 'divider',
                width: '100%',
                overflow: 'visible',
                ...(sx || {}),
            }}
            {...rest}
        >
            {children}
        </Card>
    );
};

export default PageCard;
