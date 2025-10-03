'use client';
import * as React from 'react';
import { Card, CardProps, Box, Typography, LinearProgress } from '@mui/material';

export interface PageCardProps extends CardProps {
    noPadding?: boolean;
    headerTitle?: React.ReactNode; // optional header title
    headerProps?: { gradientFrom?: string; gradientTo?: string; height?: number };
    headerActions?: React.ReactNode; // right-aligned actions (buttons, menus)
    headerActionsVariant?: 'plain' | 'frosted' | 'chips'; // style variant for header actions container
    loadingProgress?: boolean | number; // true=indeterminate, number (0-100)=determinate
}

/**
 * PageCard: consistent outer container for main page sections.
 * - Adds responsive padding, margin bottom
 * - Handles light/dark background contrast
 * - Keeps outlined variant & divider border
 */
export const PageCard: React.FC<PageCardProps> = ({ children, noPadding, headerTitle, headerProps, headerActions, headerActionsVariant = 'frosted', loadingProgress, sx, ...rest }) => {
    // Default vibrant gradient
    const gradientFrom = headerProps?.gradientFrom || '#43C6AC';
    const gradientTo = headerProps?.gradientTo || '#191654';
    const headerHeight = headerProps?.height || 44;
    return (
        <Card
            variant="outlined"
            sx={{
                position: 'relative',
                p: noPadding ? 0 : { xs: 2, md: 3 },
                mb: 2,
                bgcolor: (theme) => theme.palette.mode === 'dark' ? 'background.paper' : 'grey.50',
                border: '1px solid transparent',
                width: '100%',
                overflow: 'visible',
                pt: headerTitle ? 0 : undefined,
                '::before': {
                    content: '""',
                    position: 'absolute',
                    inset: 0,
                    borderRadius: 1,
                    padding: '2px',
                    background: 'linear-gradient(90deg, #ff66c4, #ffde59)',
                    WebkitMask: 'linear-gradient(#fff 0 0) content-box, linear-gradient(#fff 0 0)',
                    WebkitMaskComposite: 'xor',
                    maskComposite: 'exclude',
                    pointerEvents: 'none'
                },
                ...(sx || {}),
            }}
            {...rest}
        >
            {headerTitle && (
                <Box sx={{
                    position: 'relative',
                    display: 'flex',
                    flexDirection: 'column',
                    borderTopLeftRadius: (theme) => `${theme.shape.borderRadius}px`,
                    borderTopRightRadius: (theme) => `${theme.shape.borderRadius}px`,
                    overflow: 'hidden',
                    mb: 1 // space below header so content isn't flush
                }}>
                    <Box sx={{
                        height: headerHeight,
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'space-between',
                        gap: 2,
                        px: 2,
                        background: `linear-gradient(90deg, ${gradientFrom}, ${gradientTo})`,
                        color: 'primary.contrastText'
                    }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', minWidth: 0 }}>
                            {typeof headerTitle === 'string' ? <Typography variant='subtitle1' sx={{ fontWeight: 600, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{headerTitle}</Typography> : headerTitle}
                        </Box>
                        {headerActions && (
                            <Box
                                sx={(theme) => ({
                                    display: 'flex',
                                    alignItems: 'center',
                                    gap: 0.75,
                                    maxWidth: '100%',
                                    ...(headerActionsVariant === 'frosted' && {
                                        background: 'rgba(255,255,255,0.15)',
                                        backdropFilter: 'blur(6px) saturate(140%)',
                                        WebkitBackdropFilter: 'blur(6px) saturate(140%)',
                                        border: '1px solid rgba(255,255,255,0.25)',
                                        px: 1,
                                        py: 0.5,
                                        borderRadius: 999,
                                        boxShadow: '0 0 0 1px rgba(0,0,0,0.15) inset'
                                    }),
                                    ...(headerActionsVariant === 'chips' && {
                                        '& > *': {
                                            borderRadius: 999,
                                            textTransform: 'none'
                                        }
                                    })
                                })}
                            >
                                {headerActions}
                            </Box>
                        )}
                    </Box>
                    {loadingProgress !== undefined && loadingProgress !== false && (
                        <LinearProgress
                            variant={typeof loadingProgress === 'number' ? 'determinate' : 'indeterminate'}
                            value={typeof loadingProgress === 'number' ? loadingProgress : undefined}
                            sx={{ height: 3, borderRadius: 0, opacity: 0.9 }}
                        />
                    )}
                </Box>
            )}
            {children}
        </Card>
    );
};

export default PageCard;
