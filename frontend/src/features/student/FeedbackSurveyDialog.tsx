import { Dialog, DialogTitle, DialogContent, DialogActions, Button, Typography, Slider, TextField, Box } from '@mui/material';
import { useState, useEffect } from 'react';

interface Props {
    open: boolean;
    assignmentId: string | null;
    assignmentTitle?: string | null;
    onClose: () => void;
    authFetch: (url: string, init?: RequestInit) => Promise<Response>;
}

export default function FeedbackSurveyDialog({ open, assignmentId, assignmentTitle, onClose, authFetch }: Props) {
    const [helpedFix, setHelpedFix] = useState<number | null>(null);
    const [improvedUnderstanding, setImprovedUnderstanding] = useState<number | null>(null);
    const [comment, setComment] = useState('');
    const [submitting, setSubmitting] = useState(false);
    const [improvement, setImprovement] = useState<number | null>(null);
    const [firstScore, setFirstScore] = useState<number | null>(null);
    const [finalScore, setFinalScore] = useState<number | null>(null);
    const disabled = submitting || !assignmentId;

    useEffect(() => {
        if (!open || !assignmentId) return;
        (async () => {
            try {
                const res = await authFetch(`/api/feedback?assignmentId=${encodeURIComponent(assignmentId)}`);
                if (res.ok) {
                    const data = await res.json();
                    if (data) {
                        setHelpedFix(data.helpedFix || null);
                        setImprovedUnderstanding(data.improvedUnderstanding || null);
                        setComment(data.comment || '');
                        setImprovement(data.improvement ?? null);
                        setFirstScore(data.firstScore ?? null);
                        setFinalScore(data.finalScore ?? null);
                    } else {
                        setHelpedFix(null); setImprovedUnderstanding(null); setComment(''); setImprovement(null); setFirstScore(null); setFinalScore(null);
                    }
                }
            } catch {/* ignore */ }
        })();
    }, [open, assignmentId, authFetch]);

    async function handleSubmit() {
        if (!assignmentId || helpedFix == null || improvedUnderstanding == null) return;
        setSubmitting(true);
        try {
            const res = await authFetch('/api/feedback', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ assignmentId, helpedFix, improvedUnderstanding, comment })
            });
            if (res.ok) {
                onClose();
            }
        } finally {
            setSubmitting(false);
        }
    }

    function sliderMarks() { return [1, 2, 3, 4, 5].map(v => ({ value: v, label: String(v) })); }

    return (
        <Dialog open={open} onClose={() => { }} fullWidth maxWidth="sm">
            <DialogTitle>Quick Feedback {assignmentTitle ? <Typography component="span" variant="subtitle2" sx={{ ml: 1, fontWeight: 400, color: 'text.secondary' }}>• {assignmentTitle}</Typography> : null}</DialogTitle>
            <DialogContent sx={{ display: 'flex', flexDirection: 'column', gap: 3, pt: 2 }}>
                <Typography variant="body2" color="text.secondary">Required (takes &lt; 10s). Helps improve the system.</Typography>
                {improvement != null && (
                    <Box sx={{ p: 1.5, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                        <Typography variant="caption" color="text.secondary">Your improvement</Typography>
                        <Typography variant="body2">First score: {firstScore ?? '—'} • Final score: {finalScore ?? '—'} {improvement != null && firstScore != null && finalScore != null ? ` (Δ ${finalScore - firstScore})` : ''}</Typography>
                    </Box>
                )}
                <Box>
                    <Typography variant="subtitle2" gutterBottom>The automated feedback helped me correct my SQL mistakes.</Typography>
                    <Slider value={helpedFix ?? 0} min={1} max={5} step={1} marks={sliderMarks()} onChange={(_, v) => setHelpedFix(v as number)} />
                </Box>
                <Box>
                    <Typography variant="subtitle2" gutterBottom>The automated feedback enhanced my understanding of the assessed concepts.</Typography>
                    <Slider value={improvedUnderstanding ?? 0} min={1} max={5} step={1} marks={sliderMarks()} onChange={(_, v) => setImprovedUnderstanding(v as number)} />
                </Box>
                <TextField label="Optional: Further comments?" value={comment} onChange={e => setComment(e.target.value)} multiline minRows={2} />
            </DialogContent>
            <DialogActions>
                <Button disabled={disabled || helpedFix == null || improvedUnderstanding == null} variant="contained" onClick={handleSubmit}>Submit</Button>
            </DialogActions>
        </Dialog>
    );
}
