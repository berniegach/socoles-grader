"use client";
import * as React from 'react';
import { Box, Chip, Typography, IconButton, Tooltip } from '@mui/material';
import TileCard from '@/components/TileCard';
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
        <TileCard
            title={title || 'Untitled'}
            headerRight={onMenu ? (
                <Tooltip title='Actions'>
                    <IconButton size='small' onClick={(e) => { e.stopPropagation(); onMenu(e, id); }}>
                        <MoreVertIcon fontSize='small' />
                    </IconButton>
                </Tooltip>
            ) : undefined}
            chips={[
                <DifficultyChip key="difficulty" value={difficulty} />,
                <Chip key="status" label={status} size='small' sx={(theme) => {
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
                    return {
                        fontWeight: 500,
                        background: theme.palette.mode === 'dark' ? 'rgba(255,200,120,0.18)' : 'rgba(255,180,90,0.2)',
                        color: theme.palette.mode === 'dark' ? '#ffe7c5' : '#7a4400',
                        border: '1px solid',
                        borderColor: theme.palette.mode === 'dark' ? 'rgba(255,210,150,0.4)' : 'rgba(255,170,80,0.4)'
                    };
                }} variant='outlined' />,
                <Chip key="points" label={`${maxPoints} pts`} size='small' variant='outlined' />,
                <Chip key="grading" label={useDefaultGrading === false ? 'Custom' : 'Default'} size='small' variant='outlined' color={useDefaultGrading === false ? 'secondary' : 'default'} />,
                <Tooltip key="attempts" title='Unique students who achieved full credit (or auto-graded credit) for this question'>
                    <Chip label={`Attempts: ${attempts}`} size='small' variant='outlined' />
                </Tooltip>
            ]}
            onClick={() => onSelect && onSelect(id)}
            onKeyDown={(e: React.KeyboardEvent) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); onSelect && onSelect(id); } }}
            role="button"
            tabIndex={0}
            sx={(theme) => ({
                background: theme.palette.mode === 'dark'
                    ? (isSel ? 'linear-gradient(145deg, rgba(255,102,196,0.18), rgba(255,222,89,0.18))' : 'rgba(255,255,255,0.03)')
                    : (isSel ? 'linear-gradient(145deg, rgba(255,102,196,0.12), rgba(255,222,89,0.12))' : 'linear-gradient(145deg,#ffffff,#f9f9f9)'),
                borderColor: isSel ? 'secondary.light' : (theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)'),
                boxShadow: isSel ? '0 0 0 1px rgba(255,102,196,0.4), 0 4px 10px -2px rgba(0,0,0,0.25)' : '0 1px 2px rgba(0,0,0,0.08)',
                '&:hover': {
                    borderColor: isSel ? 'secondary.main' : 'primary.light'
                }
            })}
        />
    );
};

export default QuestionCard;
