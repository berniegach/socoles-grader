import { Box, Card, CardContent, Typography, Chip } from '@mui/material';
import React from 'react';
import { useTheme, alpha } from '@mui/material/styles';

interface MetricCardProps { label: string; value: number | string; icon?: React.ReactNode; helper?: string; delta?: number | null; color?: string; iconColor?: string; }

const MetricCard: React.FC<MetricCardProps> = ({ label, value, icon, helper, delta, color, iconColor }) => {
    const theme = useTheme();
    const showDelta = typeof delta === 'number' && !isNaN(delta) && delta !== 0;
    const positive = (delta || 0) > 0;
    const paletteColor = color ? (theme.palette as any)[color] : undefined;
    const fg = iconColor || (paletteColor ? paletteColor.main : undefined);
    return (
        <Card elevation={1} sx={{ height: '100%' }}>
            <CardContent sx={{ display: 'flex', flexDirection: 'column', gap: 0.5 }}>
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                    {icon && (
                        <Box
                            sx={{
                                p: 0.75,
                                borderRadius: 2,
                                border: '1px solid',
                                bgcolor: paletteColor ? alpha(paletteColor.main, 0.1) : 'rgba(15,23,42,0.06)',
                                borderColor: paletteColor ? paletteColor.light || paletteColor.main : 'rgba(15,23,42,0.12)',
                                display: 'flex',
                                alignItems: 'center',
                                justifyContent: 'center'
                            }}
                        >
                            {React.isValidElement(icon)
                                ? React.cloneElement(icon as React.ReactElement<any>, { fontSize: 'small', color: (color as any) || 'inherit' })
                                : icon}
                        </Box>
                    )}
                    <Typography variant="body2" color="text.secondary" sx={{ fontWeight: 500 }}>{label}</Typography>
                    {showDelta && <Chip size="small" label={(positive ? '+' : '') + (delta! * 100).toFixed(0) + '%'} color={positive ? 'success' : 'error'} variant="outlined" />}
                </Box>
                <Typography variant="h5" sx={{ lineHeight: 1.2 }}>
                    {typeof value === 'number'
                        ? (Number.isInteger(value) ? value.toString() : value.toFixed(2))
                        : value}
                </Typography>
                {helper && <Typography variant="caption" color="text.secondary">{helper}</Typography>}
            </CardContent>
        </Card>
    );
};

interface MetricsRowProps { metrics: Array<MetricCardProps>; }
export const MetricsRow: React.FC<MetricsRowProps> = ({ metrics }) => {
    return (
        <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { xs: '1fr 1fr', md: 'repeat(4, 1fr)' } }}>
            {metrics.map(m => <MetricCard key={m.label} {...m} />)}
        </Box>
    );
};
