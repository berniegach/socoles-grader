"use client";
import * as React from 'react';
import { Box, Chip, Typography, IconButton, Tooltip } from '@mui/material';
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
                <Chip
                    label={difficulty}
                    size='small'
                    sx={(theme) => ({
                        fontWeight: 500,
                        textTransform: 'capitalize',
                        background: {
                            Beginner: 'linear-gradient(135deg,#56ab2f,#a8e063)',
                            Intermediate: 'linear-gradient(135deg,#36d1dc,#5b86e5)',
                            Advanced: 'linear-gradient(135deg,#ff512f,#dd2476)'
                        }[difficulty as 'Beginner' | 'Intermediate' | 'Advanced'] || theme.palette.action.selected,
                        color: '#fff',
                        px: 0.75
                    })}
                />
                <Chip
                    label={status}
                    size='small'
                    variant={status === 'Published' ? 'filled' : 'outlined'}
                    color={status === 'Published' ? 'primary' : (status === 'Draft' ? 'default' : 'secondary') as any}
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
