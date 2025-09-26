// src/theme/theme.ts
'use client';
import { createTheme, darken } from '@mui/material/styles';

const theme = createTheme({
    palette: {
        mode: 'light',
        // Blue-centric palette
        primary: { light: '#60a5fa', main: '#1d4ed8', dark: '#1e3a8a', contrastText: '#ffffff' }, // blue 600
        secondary: { light: '#7dd3fc', main: '#0ea5e9', dark: '#0369a1', contrastText: '#ffffff' }, // cyan / light blue
        info: { light: '#93c5fd', main: '#3b82f6', dark: '#1d4ed8', contrastText: '#ffffff' },
        background: { default: '#f0f6ff', paper: '#ffffff' },
        divider: 'rgba(29, 78, 216, 0.18)',
        // keep semantic colors for meaning but they will be less visually dominant next to blues
        success: { main: '#15803d' },
        warning: { main: '#d97706' },
        error: { main: '#dc2626' },
    },
    shape: { borderRadius: 3 },
    typography: {
        fontFamily: 'Inter, system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif',
    },
    components: {
        MuiPaper: {
            defaultProps: { elevation: 0 },
            styleOverrides: {
                root: {
                    border: '1px solid rgba(29, 78, 216, 0.10)',
                    boxShadow: '0 6px 20px rgba(29, 78, 216, 0.08)',
                },
            },
        },
        MuiCardHeader: {
            styleOverrides: { root: { padding: '12px 16px' }, title: { fontSize: 14, fontWeight: 600 } },
        },
        MuiCardContent: { styleOverrides: { root: { padding: 16 } } },
        MuiCardActions: { styleOverrides: { root: { padding: 12 } } },

        // Compact table density
        MuiTableCell: {
            styleOverrides: {
                root: { paddingTop: 8, paddingBottom: 8 },
                head: { fontWeight: 600, background: '#e3efff' },
            },
        },

        // Default all text inputs to outlined style
        MuiTextField: { defaultProps: { variant: 'outlined', size: 'small' } },
        // Gentle outlined input styling
        MuiOutlinedInput: {
            styleOverrides: {
                root: ({ theme }) => ({
                    borderRadius: 5,
                    '& .MuiOutlinedInput-notchedOutline': {
                        borderColor: theme.palette.divider,
                    },
                    '&:hover .MuiOutlinedInput-notchedOutline': {
                        borderColor: theme.palette.primary.light,
                    },
                    '&.Mui-focused .MuiOutlinedInput-notchedOutline': {
                        borderColor: theme.palette.primary.main,
                        borderWidth: 2,
                    },
                }),
                input: { paddingTop: 12, paddingBottom: 12 },
            },
        },

        // Chips used for tiny icon backgrounds
        MuiChip: {
            styleOverrides: {
                root: { borderRadius: 10 },
                sizeSmall: { height: 24 },
            },
        },
        MuiTabs: {
            styleOverrides: { root: { minHeight: 36 } },
        },
        MuiTab: {
            styleOverrides: {
                root: { minHeight: 36, paddingTop: 6, paddingBottom: 6, textTransform: 'none' },
            },
        },

        MuiButton: { styleOverrides: { root: { borderRadius: 5 } } },
        MuiDivider: { styleOverrides: { root: { opacity: 1 } } },
        MuiListItemButton: {
            styleOverrides: {
                root: ({ theme }) => ({
                    borderRadius: 5,
                    '&.Mui-selected': {
                        backgroundColor: theme.palette.primary.main,
                        color: theme.palette.primary.contrastText,
                        // icons inside selected item
                        '& .MuiListItemIcon-root, & .MuiSvgIcon-root': {
                            color: theme.palette.primary.contrastText,
                        },
                        '&:hover': {
                            backgroundColor: darken(theme.palette.primary.main, 0.06),
                        },
                    },
                }),
            },
        },
    },
});

export default theme;
