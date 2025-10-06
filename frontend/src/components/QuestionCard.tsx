"use client";
import * as React from 'react';
import { Box, Chip, Typography, IconButton, Tooltip } from '@mui/material';
import DifficultyChip from './DifficultyChip';
import MoreVertIcon from '@mui/icons-material/MoreVert';

export interface QuestionCardProps {
    id: string;
    title: string;
    difficulty: string;
    status: string;
    maxPoints: number;
    attempts: number;
    useDefaultGrading?: boolean | null;
    selected?: boolean;
    onSelect?: (id: string) => void;
    onMenu?: (e: React.MouseEvent<HTMLElement>, id: string) => void;
}

/**
 * QuestionCard: reusable styled tile for question bank & other listings.
 * Encapsulates styling, gradient + state visuals and chip summary row.
 */
export const QuestionCard: React.FC<QuestionCardProps> = ({ id, title, difficulty, status, maxPoints, attempts, useDefaultGrading, selected, onSelect, onMenu }) => {
    const isSel = !!selected;
    return (
        <Box
            role="button"
            aria-pressed={isSel || undefined}
            tabIndex={0}
            onClick={() => onSelect && onSelect(id)}
            onKeyDown={(e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); onSelect && onSelect(id); } }}
            sx={(theme) => ({
                position: 'relative',
                p: 1.25,
                borderRadius: 1.25,
                cursor: 'pointer',
                display: 'grid',
                gap: 0.75,
                minHeight: 108,
                background: theme.palette.mode === 'dark'
                    ? (isSel ? 'linear-gradient(145deg, rgba(255,102,196,0.18), rgba(255,222,89,0.18))' : 'rgba(255,255,255,0.03)')
                    : (isSel ? 'linear-gradient(145deg, rgba(255,102,196,0.12), rgba(255,222,89,0.12))' : 'linear-gradient(145deg,#ffffff,#f9f9f9)'),
                border: '1px solid',
                borderColor: isSel ? 'secondary.light' : (theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)'),
                boxShadow: isSel ? '0 0 0 1px rgba(255,102,196,0.4), 0 4px 10px -2px rgba(0,0,0,0.25)' : '0 1px 2px rgba(0,0,0,0.08)',
                transition: 'background .25s, box-shadow .25s, transform .25s, border-color .25s',
                outline: 'none',
                '&:hover': {
                    boxShadow: '0 4px 12px -2px rgba(0,0,0,0.25)',
                    transform: 'translateY(-2px)',
                    borderColor: isSel ? 'secondary.main' : 'primary.light'
                },
                '&:focus-visible': {
                    boxShadow: '0 0 0 2px #fff, 0 0 0 4px #ff66c4',
                }
            })}
        >
            <Box sx={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', gap: 1 }}>
                <Typography variant='subtitle2' sx={{ fontWeight: 600, lineHeight: 1.2, pr: 1, flex: 1, minWidth: 0, overflow: 'hidden', textOverflow: 'ellipsis' }}>{title || 'Untitled'}</Typography>
                {onMenu && (
                    <Tooltip title='Actions'>
                        <IconButton size='small' onClick={(e) => { e.stopPropagation(); onMenu(e, id); }}>
                            <MoreVertIcon fontSize='small' />
                        </IconButton>
                    </Tooltip>
                )}
            </Box>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.6, flexWrap: 'wrap' }}>
                <DifficultyChip value={difficulty} />
                <Chip
                    label={status}
                    size='small'
                    sx={(theme) => {
                        if (status === 'Published') {
                            return {
                                fontWeight: 500,
                                background: theme.palette.mode === 'dark' ? 'rgba(120,190,255,0.15)' : 'rgba(100,160,220,0.15)',
                                color: theme.palette.mode === 'dark' ? '#d2ecff' : '#204766',
                                border: '1px solid',
                                borderColor: theme.palette.mode === 'dark' ? 'rgba(150,210,255,0.35)' : 'rgba(120,180,235,0.35)'
                            };
                        }
                        if (status === 'Draft') {
                            return {
                                fontWeight: 500,
                                background: theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.06)' : 'rgba(0,0,0,0.04)',
                                color: theme.palette.text.secondary,
                                border: '1px solid',
                                borderColor: theme.palette.divider
                            };
                        }
                        // Fallback or other statuses
                        return {
                            fontWeight: 500,
                            background: theme.palette.mode === 'dark' ? 'rgba(255,200,120,0.18)' : 'rgba(255,180,90,0.2)',
                            color: theme.palette.mode === 'dark' ? '#ffe7c5' : '#7a4400',
                            border: '1px solid',
                            borderColor: theme.palette.mode === 'dark' ? 'rgba(255,210,150,0.4)' : 'rgba(255,170,80,0.4)'
                        };
                    }}
                    variant='outlined'
                />
                <Chip label={`${maxPoints} pts`} size='small' variant='outlined' />
                <Chip
                    label={useDefaultGrading === false ? 'Custom' : 'Default'}
                    size='small'
                    variant='outlined'
                    color={useDefaultGrading === false ? 'secondary' : 'default'}
                />
                <Tooltip title='Unique students who achieved full credit (or auto-graded credit) for this question'>
                    <Chip label={`Attempts: ${attempts}`} size='small' variant='outlined' />
                </Tooltip>
            </Box>
        </Box>
    );
};

export default QuestionCard;
