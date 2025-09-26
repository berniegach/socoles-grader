'use client';
import { useRef, useState } from 'react';
import { Box, Stack, Typography, Tooltip, IconButton } from '@mui/material';
import UploadIcon from '@mui/icons-material/Upload';
import InfoOutlinedIcon from '@mui/icons-material/InfoOutlined';


export default function FileDrop({ label, accept, onFile, help }: { label: string; accept: string; onFile: (f: File) => Promise<void> | void; help?: string; }) {
    const [name, setName] = useState('');
    const [error, setError] = useState('');
    const [dragActive, setDragActive] = useState(false);
    const inputRef = useRef<HTMLInputElement | null>(null);

    function matchesAccept(file: File, acceptStr?: string) {
        if (!acceptStr) return true;
        const tokens = acceptStr.split(',').map((t) => t.trim().toLowerCase()).filter(Boolean);
        if (tokens.length === 0) return true;
        const fileName = file.name.toLowerCase();
        const mime = (file.type || '').toLowerCase();
        return tokens.some((t) => {
            if (t === '*/*') return true;
            if (t.startsWith('.')) return fileName.endsWith(t);
            if (t.endsWith('/*')) {
                const prefix = t.slice(0, -1); // includes '/'
                return mime.startsWith(prefix);
            }
            return mime === t; // exact mime match
        });
    }

    async function handleChosen(file: File | undefined | null) {
        if (!file) return;
        if (!matchesAccept(file, accept)) {
            setError(`Invalid file type. Expected ${accept}`);
            return;
        }
        setName(`${file.name} (${(file.size / 1024).toFixed(1)} KB)`);
        setError('');
        try {
            await onFile(file);
        } catch (err: any) {
            setError(String(err?.message || err));
        }
    }

    return (
        <Stack spacing={1}
            onDragOver={(e) => { e.preventDefault(); e.dataTransfer.dropEffect = 'copy'; }}
            onDragEnter={(e) => { e.preventDefault(); setDragActive(true); }}
            onDragLeave={(e) => { e.preventDefault(); setDragActive(false); }}
            onDrop={(e) => {
                e.preventDefault();
                setDragActive(false);
                const f = e.dataTransfer.files?.[0];
                void handleChosen(f);
            }}
        >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.75 }}>
                <Typography fontWeight={600}>{label}</Typography>
                {!!help && (
                    <Tooltip title={help} arrow>
                        <IconButton size="small" aria-label="Help for file input">
                            <InfoOutlinedIcon fontSize="small" />
                        </IconButton>
                    </Tooltip>
                )}
            </Box>
            <Box
                role="button"
                tabIndex={0}
                aria-label={`${label} file input`}
                onClick={() => inputRef.current?.click()}
                onKeyDown={(e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); inputRef.current?.click(); } }}
                sx={{
                    border: '1px dashed',
                    borderColor: dragActive ? 'primary.main' : 'divider',
                    bgcolor: dragActive ? 'action.hover' : 'transparent',
                    p: 2,
                    borderRadius: 1,
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    gap: 1,
                }}
            >
                <span>{name || `Drop & drop or click to browse (${accept})`}</span>
                <UploadIcon color={dragActive ? 'primary' : 'inherit'} />
                <input
                    ref={inputRef}
                    hidden
                    type="file"
                    accept={accept}
                    onChange={async (e) => {
                        const f = (e.target as HTMLInputElement).files?.[0];
                        void handleChosen(f);
                        // reset value so selecting the same file again re-triggers onChange
                        if (e.target) (e.target as HTMLInputElement).value = '';
                    }}
                />
            </Box>
            {/* Show caption only for errors */}
            {!!error && <Typography variant="caption" color="error">{error}</Typography>}
        </Stack>
    );
}