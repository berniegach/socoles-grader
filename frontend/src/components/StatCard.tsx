'use client';
import { Card, CardContent, Stack, Typography, Chip } from '@mui/material';

export default function StatCard({
    icon: Icon,
    label,
    value,
    postfix,
}: { icon: React.ElementType; label: string; value: number | string; postfix?: string }) {
    return (
        <Card>
            <CardContent>
                <Stack direction="row" spacing={1.5} alignItems="center">
                    <Chip
                        size="small"
                        icon={<Icon fontSize="small" />}
                        label=""
                        variant="outlined"
                        sx={{
                            '& .MuiChip-icon': { fontSize: 16 },
                            px: 0.5,
                            bgcolor: 'rgba(15,23,42,0.04)',
                            borderColor: 'rgba(15,23,42,0.12)',
                        }}
                    />
                    <div>
                        <Typography variant="caption" color="text.secondary">{label}</Typography>
                        <Typography variant="h5" sx={{ lineHeight: 1, mt: 0.3 }}>
                            {value}
                            {postfix && (
                                <Typography component="span" variant="body2" color="text.secondary" sx={{ ml: 0.5 }}>
                                    {postfix}
                                </Typography>
                            )}
                        </Typography>
                    </div>
                </Stack>
            </CardContent>
        </Card>
    );
}
